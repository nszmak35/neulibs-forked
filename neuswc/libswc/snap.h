#ifndef SWC_SNAP_H
#define SWC_SNAP_H

#include <stdint.h>

struct wl_display;
struct wl_global;
struct screen;

struct wl_global *
snap_manager_create(struct wl_display *display);

void
snap_overlay_cursor(void *dst_pixels, uint32_t dst_width, uint32_t dst_height,
                    uint32_t dst_pitch, struct screen *screen);

#endif
