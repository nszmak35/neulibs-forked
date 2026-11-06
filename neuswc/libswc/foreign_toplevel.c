#include "foreign_toplevel.h"

#include "event.h"
#include "input.h"
#include "internal.h"
#include "keyboard.h"
#include "seat.h"
#include "swc.h"
#include "util.h"
#include "view.h"
#include "compositor.h"
#include "window.h"

#include "wlr-foreign-toplevel-management-unstable-v1-server-protocol.h"
#include <stdlib.h>
#include <string.h>
#include <wayland-server.h>

#define VERSION 3

static struct wl_list managers = {&managers, &managers};
static struct wl_list windows = {&windows, &windows};

static void
send_state(struct wl_resource *handle, const struct window *window)
{
	struct wl_array state;
	uint32_t *entry;

	wl_array_init(&state);
	if (window->view && window->view == swc.seat->keyboard->focus.view) {
		if ((entry = wl_array_add(&state, sizeof(*entry)))) {
			*entry = ZWLR_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_ACTIVATED;
		}
	}
	if (window->mode == WINDOW_MODE_FULLSCREEN &&
	    wl_resource_get_version(handle) >= 2) {
		if ((entry = wl_array_add(&state, sizeof(*entry)))) {
			*entry = ZWLR_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_FULLSCREEN;
		}
	}
	zwlr_foreign_toplevel_handle_v1_send_state(handle, &state);
	wl_array_release(&state);
}

static void
send_all(struct wl_resource *handle, const struct window *window)
{
	if (window->base.title) {
		zwlr_foreign_toplevel_handle_v1_send_title(handle, window->base.title);
	}
	if (window->base.app_id) {
		zwlr_foreign_toplevel_handle_v1_send_app_id(handle, window->base.app_id);
	}
	send_state(handle, window);
	zwlr_foreign_toplevel_handle_v1_send_done(handle);
}

static void
handle_unlink(struct wl_resource *handle)
{
	struct wl_list *link = wl_resource_get_link(handle);

	wl_list_remove(link);
	wl_list_init(link);
}

static void
handle_destroy(struct wl_resource *handle)
{
	handle_unlink(handle);
}

static void
handle_noop(struct wl_client *client, struct wl_resource *handle)
{
	(void)client;
	(void)handle;
}

static void
handle_activate(struct wl_client *client, struct wl_resource *handle,
                struct wl_resource *seat)
{
	struct window *window = wl_resource_get_user_data(handle);

	(void)client;
	(void)seat;

	if (window && window->managed && window->activate_request) {
		window->activate_request(window->activate_request_data);
	}
}

static void
handle_close(struct wl_client *client, struct wl_resource *handle)
{
	struct window *window = wl_resource_get_user_data(handle);

	(void)client;

	if (window && window->managed && window->impl->close) {
		window->impl->close(window);
	}
}

static void
handle_set_rectangle(struct wl_client *client, struct wl_resource *handle,
                     struct wl_resource *surface, int32_t x, int32_t y,
                     int32_t width, int32_t height)
{
	(void)client;
	(void)surface;
	(void)x;
	(void)y;

	if (width < 0 || height < 0) {
		wl_resource_post_error(
		    handle, ZWLR_FOREIGN_TOPLEVEL_HANDLE_V1_ERROR_INVALID_RECTANGLE,
		    "invalid rectangle");
	}
}

static void
handle_set_fullscreen(struct wl_client *client, struct wl_resource *handle,
                      struct wl_resource *output)
{
	struct window *window = wl_resource_get_user_data(handle);

	(void)client;
	(void)output;

	if (window && window->managed && window->fullscreen_request) {
		window->fullscreen_request(window->fullscreen_request_data, NULL, true);
	}
}

static void
handle_unset_fullscreen(struct wl_client *client, struct wl_resource *handle)
{
	struct window *window = wl_resource_get_user_data(handle);

	(void)client;

	if (window && window->managed && window->fullscreen_request) {
		window->fullscreen_request(window->fullscreen_request_data, NULL, false);
	}
}

static const struct zwlr_foreign_toplevel_handle_v1_interface handle_impl = {
    .set_maximized = handle_noop,
    .unset_maximized = handle_noop,
    .set_minimized = handle_noop,
    .unset_minimized = handle_noop,
    .activate = handle_activate,
    .close = handle_close,
    .set_rectangle = handle_set_rectangle,
    .destroy = destroy_resource,
    .set_fullscreen = handle_set_fullscreen,
    .unset_fullscreen = handle_unset_fullscreen,
};

static void
handle_create(struct wl_resource *manager, struct window *window)
{
	struct wl_resource *handle;

	handle = wl_resource_create(wl_resource_get_client(manager),
	                            &zwlr_foreign_toplevel_handle_v1_interface,
	                            wl_resource_get_version(manager), 0);
	if (!handle) {
		wl_client_post_no_memory(wl_resource_get_client(manager));
		return;
	}
	wl_resource_set_implementation(handle, &handle_impl, window, handle_destroy);
	wl_list_insert(&window->toplevel_handles, wl_resource_get_link(handle));
	zwlr_foreign_toplevel_manager_v1_send_toplevel(manager, handle);
	send_all(handle, window);
}

static void
manager_stop(struct wl_client *client, struct wl_resource *resource)
{
	struct wl_list *link = wl_resource_get_link(resource);

	(void)client;

	wl_list_remove(link);
	wl_list_init(link);
	zwlr_foreign_toplevel_manager_v1_send_finished(resource);
}

static const struct zwlr_foreign_toplevel_manager_v1_interface manager_impl = {
    .stop = manager_stop,
};

static void
manager_destroy(struct wl_resource *resource)
{
	struct wl_list *link = wl_resource_get_link(resource);

	wl_list_remove(link);
	wl_list_init(link);
}

static void
bind_manager(struct wl_client *client, void *data, uint32_t version,
             uint32_t id)
{
	struct wl_resource *resource;
	struct window *window;

	(void)data;

	resource = wl_resource_create(
	    client, &zwlr_foreign_toplevel_manager_v1_interface, version, id);
	if (!resource) {
		wl_client_post_no_memory(client);
		return;
	}
	wl_resource_set_implementation(resource, &manager_impl, NULL,
	                               manager_destroy);
	wl_list_insert(&managers, wl_resource_get_link(resource));

	wl_list_for_each (window, &windows, toplevel_link) {
		handle_create(resource, window);
	}
}

struct wl_global *
foreign_toplevel_manager_create(struct wl_display *display)
{
	return wl_global_create(display,
	                        &zwlr_foreign_toplevel_manager_v1_interface, VERSION,
	                        NULL, bind_manager);
}

void
foreign_toplevel_window_add(struct window *window)
{
	struct wl_resource *manager;
	struct wl_list *link;

	wl_list_insert(&windows, &window->toplevel_link);
	for (link = managers.next; link != &managers; link = link->next) {
		manager = wl_resource_from_link(link);
		handle_create(manager, window);
	}
}

void
foreign_toplevel_window_remove(struct window *window)
{
	struct wl_list *link, *next;

	for (link = window->toplevel_handles.next; link != &window->toplevel_handles;
	     link = next) {
		struct wl_resource *handle = wl_resource_from_link(link);

		next = link->next;
		wl_resource_set_user_data(handle, NULL);
		zwlr_foreign_toplevel_handle_v1_send_closed(handle);
		wl_list_remove(link);
		wl_list_init(link);
	}
	wl_list_remove(&window->toplevel_link);
	wl_list_init(&window->toplevel_link);
}

void
foreign_toplevel_window_update(struct window *window)
{
	struct wl_list *link;

	if (!window)
		return;
	for (link = window->toplevel_handles.next; link != &window->toplevel_handles;
	     link = link->next) {
		send_all(wl_resource_from_link(link), window);
	}
}

static void
handle_focus_changed(struct wl_listener *listener, void *data)
{
	struct event *event = data;
	struct input_focus_event_data *event_data = event->data;

	(void)listener;

	if (event->type != INPUT_FOCUS_EVENT_CHANGED) {
		return;
	}
	if (event_data->old && event_data->old->window) {
		foreign_toplevel_window_update(event_data->old->window);
	}
	if (event_data->new && event_data->new->window) {
		foreign_toplevel_window_update(event_data->new->window);
	}
}

struct wl_listener foreign_toplevel_focus_listener = {
    .notify = handle_focus_changed,
};
