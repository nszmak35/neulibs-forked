#ifndef SWC_SCREENCOPY_H
#define SWC_SCREENCOPY_H

struct wl_display;
struct wl_global;

struct wl_global *
screencopy_manager_create(struct wl_display *display);

#endif
