/* Copyright © 2013, Intel Corporation
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
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/* Perform some simple drawing via OpenGL as follows:
 *
 *	1. Using EGL to construct an OpenGL context
 *	2. By directly linking with libEGL.so and libGL.so
 *	3. By directly calling OpenGL functions
 */

#include <EGL/egl.h>
#include <GL/gl.h>

#include "util-x11.h"

#define COMMON_USE_EGL
#define COMMON_GL_PREFIX
#define COMMON_EGL_PREFIX
#include "common.c"

int
main (void)
{
        Display *dpy;
	Window window;
	EGLDisplay egl_dpy;
	EGLContext ctx;
	EGLConfig config;
        EGLSurface surface;
	XVisualInfo *visual_info;

	dpy = util_x11_init_display ();

	common_create_egl_context (dpy, EGL_OPENGL_API, &egl_dpy,
				   &ctx, &config, &visual_info);

	window = util_x11_init_window (dpy, visual_info);

	surface = eglCreateWindowSurface (egl_dpy, config, window, NULL);

	common_make_current (egl_dpy, ctx, surface);

        common_handle_events (dpy, egl_dpy, surface);

	util_x11_fini_window (dpy, window);

	util_x11_fini_display (dpy);

        return 0;
}
