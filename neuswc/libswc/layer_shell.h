#ifndef SWC_LAYER_SHELL_H
#define SWC_LAYER_SHELL_H

#include <stdbool.h>

struct wl_display;
struct wl_global;

struct wl_global *
layer_shell_create(struct wl_display *display);

bool
layer_shell_keyboard_grabbed(void);

void
layer_shell_finalize(void);

#endif
