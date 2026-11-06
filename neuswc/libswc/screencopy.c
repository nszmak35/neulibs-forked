#include "screencopy.h"
#include "compositor.h"
#include "internal.h"
#include "output.h"
#include "screen.h"
#include "shm.h"
#include "snap.h"
#include "util.h"

#include "wlr-screencopy-unstable-v1-server-protocol.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <wayland-server.h>
#include <wld/wld.h>

struct frame {
	struct wl_resource *resource;
	struct screen *screen;
	struct swc_rectangle region;
	bool overlay_cursor;
	bool copied;
};

static bool
screen_alive(const struct screen *screen)
{
	struct screen *s;

	wl_list_for_each (s, &swc.screens, link) {
		if (s == screen) {
			return true;
		}
	}
	return false;
}

static void
frame_free(struct wl_resource *resource)
{
	free(wl_resource_get_user_data(resource));
}

static void
frame_copy_buffer(struct wl_resource *resource,
                  struct wl_resource *buffer_resource, bool with_damage)
{
	struct frame *frame = wl_resource_get_user_data(resource);
	const struct swc_rectangle *r = &frame->region;
	struct swc_shm_buffer_info target;
	struct wld_buffer *shot;
	struct timespec ts;
	uint32_t y, x;

	if (frame->copied) {
		wl_resource_post_error(resource,
		                       ZWLR_SCREENCOPY_FRAME_V1_ERROR_ALREADY_USED,
		                       "the frame was already copied");
		return;
	}
	frame->copied = true;

	if (!shm_buffer_get_info(buffer_resource, &target) || !target.writable ||
	    (target.format != WL_SHM_FORMAT_XRGB8888 &&
	     target.format != WL_SHM_FORMAT_ARGB8888) ||
	    target.width != (int32_t)r->width ||
	    target.height != (int32_t)r->height ||
	    target.stride < (int32_t)(r->width * 4)) {
		wl_resource_post_error(resource,
		                       ZWLR_SCREENCOPY_FRAME_V1_ERROR_INVALID_BUFFER,
		                       "the buffer does not match the frame");
		return;
	}

	if (!screen_alive(frame->screen) || !(shot = compositor_render_to_shm(frame->screen))) {
		zwlr_screencopy_frame_v1_send_failed(resource);
		return;
	}
	if (!wld_map(shot) || !shot->map ||
	    (uint64_t)r->x + r->width > shot->width ||
	    (uint64_t)r->y + r->height > shot->height) {
		wld_buffer_unreference(shot);
		zwlr_screencopy_frame_v1_send_failed(resource);
		return;
	}

	if (frame->overlay_cursor) {
		snap_overlay_cursor(shot->map, shot->width, shot->height, shot->pitch,
		                    frame->screen);
	}

	for (y = 0; y < r->height; y++) {
		const uint32_t *src =
		    (const uint32_t *)((const uint8_t *)shot->map +
		                       (size_t)(r->y + y) * shot->pitch) +
		    r->x;
		uint32_t *dst =
		    (uint32_t *)((uint8_t *)target.data + (size_t)y * target.stride);

		for (x = 0; x < r->width; x++) {
			dst[x] = src[x] | 0xff000000u;
		}
	}
	wld_unmap(shot);
	wld_buffer_unreference(shot);

	clock_gettime(CLOCK_MONOTONIC, &ts);
	zwlr_screencopy_frame_v1_send_flags(resource, 0);
	if (with_damage) {
		zwlr_screencopy_frame_v1_send_damage(resource, 0, 0, r->width, r->height);
	}
	zwlr_screencopy_frame_v1_send_ready(resource, (uint32_t)((uint64_t)ts.tv_sec >> 32),
	                                    (uint32_t)ts.tv_sec, (uint32_t)ts.tv_nsec);
}

static void
frame_copy(struct wl_client *client, struct wl_resource *resource,
           struct wl_resource *buffer_resource)
{
	(void)client;
	frame_copy_buffer(resource, buffer_resource, false);
}

static void
frame_copy_with_damage(struct wl_client *client, struct wl_resource *resource,
                       struct wl_resource *buffer_resource)
{
	(void)client;
	frame_copy_buffer(resource, buffer_resource, true);
}

static const struct zwlr_screencopy_frame_v1_interface frame_impl = {
    .copy = frame_copy,
    .destroy = destroy_resource,
    .copy_with_damage = frame_copy_with_damage,
};

static void
create_frame(struct wl_client *client, struct wl_resource *manager,
             uint32_t id, int32_t overlay_cursor,
             struct wl_resource *output_resource, bool has_region, int32_t x,
             int32_t y, int32_t width, int32_t height)
{
	struct output *output = wl_resource_get_user_data(output_resource);
	struct screen *screen = output ? output->screen : NULL;
	struct wl_resource *resource;
	struct frame *frame;
	int64_t x1, y1, x2, y2, screen_w, screen_h;

	frame = calloc(1, sizeof(*frame));
	resource = wl_resource_create(client, &zwlr_screencopy_frame_v1_interface,
	                              wl_resource_get_version(manager), id);
	if (!frame || !resource) {
		free(frame);
		if (resource) {
			wl_resource_destroy(resource);
		}
		wl_client_post_no_memory(client);
		return;
	}
	frame->resource = resource;
	frame->screen = screen;
	frame->overlay_cursor = overlay_cursor != 0;
	wl_resource_set_implementation(resource, &frame_impl, frame, &frame_free);

	if (!screen || !screen_alive(screen)) {
		zwlr_screencopy_frame_v1_send_failed(resource);
		return;
	}

	screen_w = screen->base.geometry.width;
	screen_h = screen->base.geometry.height;
	if (has_region) {
		x1 = x;
		y1 = y;
		x2 = (int64_t)x + width;
		y2 = (int64_t)y + height;
		x1 = x1 < 0 ? 0 : x1;
		y1 = y1 < 0 ? 0 : y1;
		x2 = x2 > screen_w ? screen_w : x2;
		y2 = y2 > screen_h ? screen_h : y2;
	} else {
		x1 = y1 = 0;
		x2 = screen_w;
		y2 = screen_h;
	}
	if (width < 0 || height < 0 || x2 <= x1 || y2 <= y1) {
		zwlr_screencopy_frame_v1_send_failed(resource);
		return;
	}
	frame->region.x = (int32_t)x1;
	frame->region.y = (int32_t)y1;
	frame->region.width = (uint32_t)(x2 - x1);
	frame->region.height = (uint32_t)(y2 - y1);

	zwlr_screencopy_frame_v1_send_buffer(resource, WL_SHM_FORMAT_XRGB8888,
	                                     frame->region.width, frame->region.height,
	                                     frame->region.width * 4);
	if (wl_resource_get_version(resource) >= ZWLR_SCREENCOPY_FRAME_V1_BUFFER_DONE_SINCE_VERSION) {
		zwlr_screencopy_frame_v1_send_buffer_done(resource);
	}
}

static void
capture_output(struct wl_client *client, struct wl_resource *resource,
               uint32_t id, int32_t overlay_cursor,
               struct wl_resource *output_resource)
{
	create_frame(client, resource, id, overlay_cursor, output_resource, false, 0,
	             0, 0, 0);
}

static void
capture_output_region(struct wl_client *client, struct wl_resource *resource,
                      uint32_t id, int32_t overlay_cursor,
                      struct wl_resource *output_resource, int32_t x, int32_t y,
                      int32_t width, int32_t height)
{
	create_frame(client, resource, id, overlay_cursor, output_resource, true, x, y,
	             width, height);
}

static const struct zwlr_screencopy_manager_v1_interface manager_impl = {
    .capture_output = capture_output,
    .capture_output_region = capture_output_region,
    .destroy = destroy_resource,
};

static void
bind_manager(struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
	struct wl_resource *resource;

	resource = wl_resource_create(client, &zwlr_screencopy_manager_v1_interface,
	                              version, id);
	if (!resource) {
		wl_client_post_no_memory(client);
		return;
	}
	wl_resource_set_implementation(resource, &manager_impl, NULL, NULL);
}

struct wl_global *
screencopy_manager_create(struct wl_display *display)
{
	return wl_global_create(display, &zwlr_screencopy_manager_v1_interface, 3,
	                        NULL, &bind_manager);
}
