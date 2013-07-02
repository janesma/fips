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
 *	1. Using EGL to construct an OpenGLESv2 context
 *	2. By directly linking with libEGL.so and libGLESv2.so
 *	3. By using eglGetProcAddress to lookup OpenGL functions
 */

#define GL_GLEXT_PROTOTYPES
#include <GLES2/gl2.h>
#include <EGL/egl.h>

#include <stdio.h>
#include <stdlib.h>

#include "util-x11.h"

/* The OpenGL header files give typedefs for the types of all
 * functions provided by etensisons. Here, though, we're using
 * glxGetProcAddress to lookup core functions, so we provide our own
 * typedefs. */
typedef void (*FIPS_GLCLEAR_FN)(GLbitfield);
FIPS_GLCLEAR_FN my_glClear;

typedef void (*FIPS_GLCLEARCOLOR_FN)(GLclampf, GLclampf, GLclampf, GLclampf);
FIPS_GLCLEARCOLOR_FN my_glClearColor;

typedef void (*FIPS_GLVIEWPORT_FN)(GLint, GLint, GLsizei, GLsizei);
FIPS_GLVIEWPORT_FN my_glViewport;

#define COMMON_USE_EGL
#define COMMON_GL_PREFIX my_
#define COMMON_EGL_PREFIX
#include "common.c"

static void
resolve_symbols (void)
{
	my_glClear = (FIPS_GLCLEAR_FN) eglGetProcAddress ("glClear");
	if (my_glClear == NULL) {
		fprintf (stderr, "Failed to eglGetProcAddress glClear\n");
		exit (1);
	}

	my_glClearColor = (FIPS_GLCLEARCOLOR_FN) eglGetProcAddress ("glClearColor");
	if (my_glClearColor == NULL) {
		fprintf (stderr, "Failed to eglGetProcAddress glClearColor\n");
		exit (1);
	}

	my_glViewport = (FIPS_GLVIEWPORT_FN) eglGetProcAddress ("glViewport");
	if (my_glViewport == NULL) {
		fprintf (stderr, "Failed to eglGetProcAddress glViewport\n");
		exit (1);
	}
}

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

	resolve_symbols ();

	dpy = util_x11_init_display ();

	common_create_egl_context (dpy, EGL_OPENGL_ES_API, &egl_dpy,
				   &ctx, &config, &visual_info);

	window = util_x11_init_window (dpy, visual_info);

	surface = eglCreateWindowSurface (egl_dpy, config, window, NULL);

	common_make_current (egl_dpy, ctx, surface);

        common_handle_events (dpy, egl_dpy, surface);

	util_x11_fini_window (dpy, window);

	util_x11_fini_display (dpy);

        return 0;
}
