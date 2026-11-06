/* swc: libswc/swc.h
 *
 * Copyright (c) 2013 Michael Forney
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef SWC_H
#define SWC_H

#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

struct libinput_device;
struct wl_display;
struct wl_event_loop;
struct wld_buffer;

/* Get the current cursor position */
bool
swc_cursor_position(int32_t *x, int32_t *y);

/* Set the cursor position */
bool
swc_cursor_set_position(int32_t x, int32_t y);

/* Send a pointer button event to the currently focused client */
void
swc_pointer_send_button(uint32_t time, uint32_t button, uint32_t state);

/* Send a pointer axis event to the currently focused client */
void
swc_pointer_send_axis(uint32_t time, uint32_t axis, int32_t value120);

enum swc_cursor_kind {
	SWC_CURSOR_DEFAULT = 0,
	SWC_CURSOR_BOX = 1,
	SWC_CURSOR_CROSS = 2,
	SWC_CURSOR_SIGHT = 3,
	SWC_CURSOR_UP = 4,
	SWC_CURSOR_DOWN = 5,
};

enum swc_cursor_mode {
	SWC_CURSOR_MODE_CLIENT = 0,
	SWC_CURSOR_MODE_COMPOSITOR = 1,
};

/* Override the compositor's internal cursor */
void
swc_set_cursor(enum swc_cursor_kind kind);

/* Control whether client cursor surfaces are honored */
void
swc_set_cursor_mode(enum swc_cursor_mode mode);

/* set a custom argb8888 cursor image for a given kind */
void
swc_set_cursor_image(enum swc_cursor_kind kind, const uint32_t *argb8888,
                     uint32_t width, uint32_t height, int32_t hotspot_x,
                     int32_t hotspot_y);

void
swc_clear_cursor_image(enum swc_cursor_kind kind);

/* draw [or update] a simple box overlay */
void
swc_overlay_set_box(int32_t x1, int32_t y1, int32_t x2, int32_t y2,
                    uint32_t color, uint32_t border_width);

/* Clear the current overlay, if any */
void
swc_overlay_clear(void);

/* Set the compositor zoom level */
void
swc_set_zoom(float level);

/* Set the color painted behind all windows (0xAARRGGBB, alpha is ignored) */
void
swc_set_background(uint32_t color);

/* Get the current zoom level */
float
swc_get_zoom(void);

struct swc_rectangle {
	int32_t x, y;
	uint32_t width, height;
};

struct swc_screen_handler {
	/* Called when the screen is about to be destroyed */
	void (*destroy)(void *data);

	/* Called when the total area of the screen has changed */
	void (*geometry_changed)(void *data);

	/* Called when the geometry of the screen available for laying out windows has changed */
	void (*usable_geometry_changed)(void *data);

	/* Called when the pointer enters the screen */
	void (*entered)(void *data);
};

struct swc_screen {
	/* The total area of the screen */
	struct swc_rectangle geometry;

	/* The area of the screen available for placing windows */
	struct swc_rectangle usable_geometry;
};

/* Set the handler associated with this screen */
void
swc_screen_set_handler(struct swc_screen *screen,
                       const struct swc_screen_handler *handler, void *data);

struct swc_window_handler {
	/* Called when the window is about to be destroyed */
	void (*destroy)(void *data);

	/* Called when the window's title changes */
	void (*title_changed)(void *data);

	/* Called when the window's application identifier changes */
	void (*app_id_changed)(void *data);

	/* Called when the window's parent changes */
	void (*parent_changed)(void *data);

	/* Called when the pointer enters the window */
	void (*entered)(void *data);

	/* Called when the window wants to initiate an interactive move, but the window is not in stacked mode */
	void (*move)(void *data);

	/* Called when the window wants to initiate an interactive resize, but the window is not in stacked mode */
	void (*resize)(void *data);
};

struct swc_window;

/* Called when a client asks to become fullscreen */
typedef void (*swc_window_fullscreen_request)(void *data,
                                              struct swc_screen *screen,
                                              bool fullscreen);

/* Called when something other than the window manager */
typedef void (*swc_window_activate_request)(void *data);

/* Be told about the activation requests of a window */
void
swc_window_set_activate_handler(struct swc_window *window,
                                swc_window_activate_request request, void *data);

/* Be told about the fullscreen requests of a window */
void
swc_window_set_fullscreen_handler(struct swc_window *window,
                                  swc_window_fullscreen_request request,
                                  void *data);

struct swc_window {
	char *title;
	char *app_id;

	struct swc_window *parent;
	uint32_t motion_throttle_ms;
	uint32_t min_width;
	uint32_t min_height;
	uint32_t max_width;
	uint32_t max_height;
};

/* Set the handler associated with this window */
void
swc_window_set_handler(struct swc_window *window,
                       const struct swc_window_handler *handler, void *data);

/* Request that the specified window close */
void
swc_window_close(struct swc_window *window);

/* Make the specified window visible */
void
swc_window_show(struct swc_window *window);

/* Make the specified window hidden */
void
swc_window_hide(struct swc_window *window);

/* Set the keyboard focus to the specified window */
void
swc_window_focus(struct swc_window *window);

/* Sets the window to stacked mode */
void
swc_window_set_stacked(struct swc_window *window);

/* Sets the window to tiled mode */
void
swc_window_set_tiled(struct swc_window *window);

/* Sets the window to fullscreen mode */
void
swc_window_set_fullscreen(struct swc_window *window, struct swc_screen *screen);

/* Set the window's position */
void
swc_window_set_position(struct swc_window *window, int32_t x, int32_t y);

/* Set the window's size */
void
swc_window_set_size(struct swc_window *window, uint32_t width, uint32_t height);

/* Set the window's size and position */
void
swc_window_set_geometry(struct swc_window *window,
                        const struct swc_rectangle *geometry);

/* Get the window's current geometry in compositor-global coordinates */
bool
swc_window_get_geometry(const struct swc_window *window,
                        struct swc_rectangle *geometry);

/* Get the pid of the client that owns this window */
pid_t
swc_window_get_pid(struct swc_window *window);

/* Set the window's border color and width */
void
swc_window_set_border(struct swc_window *window, uint32_t inner_border_color,
                      uint32_t inner_border_width, uint32_t outer_border_color,
                      uint32_t outer_border_width);

/* Select which decor edge a text slot is rendered */
enum swc_decor_edge {
	SWC_DECOR_EDGE_TOP,
	SWC_DECOR_EDGE_RIGHT,
	SWC_DECOR_EDGE_BOTTOM,
	SWC_DECOR_EDGE_LEFT,
};

/* Choose text alignment within the selected decor edge */
enum swc_decor_align {
	SWC_DECOR_ALIGN_START,
	SWC_DECOR_ALIGN_CENTER,
	SWC_DECOR_ALIGN_END,
};

/* Describe a window decor text slot */
struct swc_decor_text {
	bool enabled;
	enum swc_decor_edge edge;
	enum swc_decor_align align;
	const char *string;
	uint32_t color;
	uint32_t padding;
	int32_t offset_x, offset_y;
	const char *font;
};

/* Describes one wm-supplied decor pixel block */
struct swc_decor_part {
	uint32_t width, height;
	uint32_t stride;
	const void *data;
};

/* Describes optional pixel blocks for the outer frame of a decor */
struct swc_decor_parts {
	struct swc_decor_part top_left;
	struct swc_decor_part top;
	struct swc_decor_part top_right;
	struct swc_decor_part left;
	struct swc_decor_part right;
	struct swc_decor_part bottom_left;
	struct swc_decor_part bottom;
	struct swc_decor_part bottom_right;
};

/* Describe decor around a window's edges */
struct swc_decor {
	uint32_t color;
	uint32_t top, right, bottom, left;
	const struct swc_decor_parts *parts;
	struct swc_decor_text title;
};

/* Set window decor around the window */
void
swc_window_set_decor(struct swc_window *window, const struct swc_decor *decor);

/* Let the decor reach over the window content */
void
swc_window_set_decor_inset(struct swc_window *window, uint32_t inset);

/* Begin an interactive move of the specified window */
void
swc_window_begin_move(struct swc_window *window);

/* End an interactive move of the specified window */
void
swc_window_end_move(struct swc_window *window);

enum {
	SWC_WINDOW_EDGE_AUTO = 0,
	SWC_WINDOW_EDGE_TOP = (1 << 0),
	SWC_WINDOW_EDGE_BOTTOM = (1 << 1),
	SWC_WINDOW_EDGE_LEFT = (1 << 2),
	SWC_WINDOW_EDGE_RIGHT = (1 << 3)
};

/* Begin an interactive resize of the specified window */
void
swc_window_begin_resize(struct swc_window *window, uint32_t edges);

/* End an interactive resize of the specified window */
void
swc_window_end_resize(struct swc_window *window);

/* returns the topmost window at any given compositor global coordinates */
struct swc_window *
swc_window_at(int32_t x, int32_t y);

/* move a window in the stacking order by one step */
void
swc_window_stack(struct swc_window *window, int32_t direction);

struct wl_resource;

/* The screen of a wl_output resource a client bound, or NULL */
struct swc_screen *
swc_screen_from_output(struct wl_resource *output);

/* Index of the active keyboard layout (xkb group) */
uint32_t
swc_keyboard_get_layout(void);

/* Switch to the keyboard layout (xkb group) with this index */
bool
swc_keyboard_set_layout(uint32_t index);

/* Build the keyboard keymap again from these xkb names */
#define SWC_HAS_KEYBOARD_SET_KEYMAP 1
bool
swc_keyboard_set_keymap(const char *rules, const char *model,
                        const char *layout, const char *variant,
                        const char *options);

typedef void (*swc_keyboard_layout_handler)(void *data, uint32_t index);

/* Be told when the active keyboard layout changes */
void
swc_keyboard_set_layout_handler(swc_keyboard_layout_handler handler, void *data);

extern int32_t swc_repeat_rate, swc_repeat_delay;

enum {
	SWC_MOD_CTRL = 1 << 0,
	SWC_MOD_ALT = 1 << 1,
	SWC_MOD_LOGO = 1 << 2,
	SWC_MOD_SHIFT = 1 << 3,
	SWC_MOD_ANY = ~0
};

enum swc_binding_type {
	SWC_BINDING_KEY,
	SWC_BINDING_BUTTON,
};

typedef void (*swc_binding_handler)(void *data, uint32_t time, uint32_t value,
                                    uint32_t state);
typedef void (*swc_axis_binding_handler)(void *data, uint32_t time,
                                         uint32_t axis, int32_t value120);

/* Register a new input binding */
int
swc_add_binding(enum swc_binding_type type, uint32_t modifiers, uint32_t value,
                swc_binding_handler handler, void *data);

/* Unregister a registered input binding */
void
swc_remove_binding(enum swc_binding_type type, uint32_t modifiers,
                   uint32_t value);

/* register a new pointer axis binding */
int
swc_add_axis_binding(uint32_t modifiers, uint32_t axis,
                     swc_axis_binding_handler handler, void *data);

/* This is a user-provided structure that swc will use to notify the display server of new windows */
struct swc_manager {
	/* Called when a new screen is created */
	void (*new_screen)(struct swc_screen *screen);

	/* Called when a new window is created */
	void (*new_window)(struct swc_window *window);

	/* Called when a new input device is detected */
	void (*new_device)(struct libinput_device *device);

	/* Called when the session gets activated (for example, startup or VT switch) */
	void (*activate)(void);

	/* Called when the session gets deactivated */
	void (*deactivate)(void);
};

/* Initializes the compositor using the specified display, event_loop, and manager */
bool
swc_initialize(struct wl_display *display, struct wl_event_loop *event_loop,
               const struct swc_manager *manager);

/* Stops the compositor, releasing any used resources */
void
swc_finalize(void);

#ifdef __cplusplus
}
#endif

#endif
