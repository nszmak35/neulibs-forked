#ifndef SWC_FOREIGN_TOPLEVEL_H
#define SWC_FOREIGN_TOPLEVEL_H

struct window;
struct wl_display;
struct wl_global;
struct wl_listener;

struct wl_global *
foreign_toplevel_manager_create(struct wl_display *display);

void
foreign_toplevel_window_add(struct window *window);
void
foreign_toplevel_window_remove(struct window *window);

void
foreign_toplevel_window_update(struct window *window);

extern struct wl_listener foreign_toplevel_focus_listener;

#endif
