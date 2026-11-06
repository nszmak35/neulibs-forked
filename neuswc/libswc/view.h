/* swc: libswc/view.h
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

#ifndef SWC_VIEW_H
#define SWC_VIEW_H

#include "swc.h"

#include <wayland-util.h>

struct view {
	const struct view_impl *impl;
	struct wl_list handlers;

	struct swc_rectangle geometry;
	uint32_t screens;

	struct wld_buffer *buffer;
};

struct view_handler {
	const struct view_handler_impl *impl;
	struct wl_list link;
};

struct view_impl {
	bool (*update)(struct view *view);
	int (*attach)(struct view *view, struct wld_buffer *buffer);
	bool (*move)(struct view *view, int32_t x, int32_t y);
};

struct view_handler_impl {
	void (*frame)(struct view_handler *handler, uint32_t time);
	void (*attach)(struct view_handler *handler);
	void (*move)(struct view_handler *handler);
	void (*resize)(struct view_handler *handler, uint32_t old_width,
	               uint32_t old_height);
	void (*screens)(struct view_handler *handler, uint32_t left,
	                uint32_t entered);
};

/* Attach a new buffer to the view */
int
view_attach(struct view *view, struct wld_buffer *buffer);

bool
view_update(struct view *view);

bool
view_move(struct view *view, int32_t x, int32_t y);

void
view_initialize(struct view *view, const struct view_impl *impl);

void
view_finalize(struct view *view);

bool
view_set_position(struct view *view, int32_t x, int32_t y);
bool
view_set_size(struct view *view, uint32_t width, uint32_t height);
bool
view_set_size_from_buffer(struct view *view, struct wld_buffer *bufer);
void
view_set_screens(struct view *view, uint32_t screens);
void
view_update_screens(struct view *view);

void
view_frame(struct view *view, uint32_t time);

#endif
