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
 *	1. Using GLX to construct an OpenGL context
 *	2. By directly linking with libGL.so
 *	3. By using glXGetProcAddress to lookup OpenGL functions
 */

#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glx.h>

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

typedef XVisualInfo* (*FIPS_GLXCHOOSEVISUAL_FN)(Display *, int, int *);
FIPS_GLXCHOOSEVISUAL_FN my_glXChooseVisual;

typedef GLXContext (*FIPS_GLXCREATECONTEXT_FN)(Display *, XVisualInfo *, GLXContext, Bool);
FIPS_GLXCREATECONTEXT_FN my_glXCreateContext;

typedef void (*FIPS_GLXDESTROYCONTEXT_FN)(Display *, GLXContext);
FIPS_GLXDESTROYCONTEXT_FN my_glXDestroyContext;

typedef Bool (*FIPS_GLXMAKECURRENT_FN)(Display *, GLXDrawable, GLXContext);
FIPS_GLXMAKECURRENT_FN my_glXMakeCurrent;

typedef void (*FIPS_GLXSWAPBUFFERS_FN)(Display *, GLXDrawable);
FIPS_GLXSWAPBUFFERS_FN my_glXSwapBuffers;

#define COMMON_GL_PREFIX my_
#include "common.c"

static void
resolve_symbols (void)
{
	my_glClear = (FIPS_GLCLEAR_FN) glXGetProcAddress ((GLubyte*) "glClear");
	if (my_glClear == NULL) {
		fprintf (stderr, "Failed to glXGetProcAddress glClear\n");
		exit (1);
	}

	my_glClearColor = (FIPS_GLCLEARCOLOR_FN) glXGetProcAddress ((GLubyte*) "glClearColor");
	if (my_glClearColor == NULL) {
		fprintf (stderr, "Failed to glXGetProcAddress glClearColor\n");
		exit (1);
	}

	my_glViewport = (FIPS_GLVIEWPORT_FN) glXGetProcAddress ((GLubyte*) "glViewport");
	if (my_glViewport == NULL) {
		fprintf (stderr, "Failed to glXGetProcAddress glViewport\n");
		exit (1);
	}

	my_glXChooseVisual = (FIPS_GLXCHOOSEVISUAL_FN) glXGetProcAddress ((GLubyte*) "glXChooseVisual");
	if (my_glXChooseVisual == NULL) {
		fprintf (stderr, "Failed to glXGetProcAddress glXChooseVisual\n");
		exit (1);
	}

	my_glXCreateContext = (FIPS_GLXCREATECONTEXT_FN) glXGetProcAddress ((GLubyte*) "glXCreateContext");
	if (my_glXCreateContext == NULL) {
		fprintf (stderr, "Failed to glXGetProcAddress glXCreateContext\n");
		exit (1);
	}

	my_glXDestroyContext = (FIPS_GLXDESTROYCONTEXT_FN) glXGetProcAddress ((GLubyte*) "glXDestroyContext");
	if (my_glXDestroyContext == NULL) {
		fprintf (stderr, "Failed to glXGetProcAddress glXDestroyContext\n");
		exit (1);
	}

	my_glXMakeCurrent = (FIPS_GLXMAKECURRENT_FN) glXGetProcAddress ((GLubyte*) "glXMakeCurrent");
	if (my_glXMakeCurrent == NULL) {
		fprintf (stderr, "Failed to glXGetProcAddress glXMakeCurrent\n");
		exit (1);
	}

	my_glXSwapBuffers = (FIPS_GLXSWAPBUFFERS_FN) glXGetProcAddress ((GLubyte*) "glXSwapBuffers");
	if (my_glXSwapBuffers == NULL) {
		fprintf (stderr, "Failed to glXGetProcAddress glXSwapBuffers\n");
		exit (1);
	}
}

int
main (void)
{
        Display *dpy;
        Window window;
	GLXContext ctx;
	XVisualInfo *visual_info;

	resolve_symbols ();

	dpy = util_x11_init_display ();

	common_create_glx_context (dpy, &ctx, &visual_info);

	window = util_x11_init_window (dpy, visual_info);

	common_make_current (dpy, ctx, window);

        common_handle_events (dpy, dpy, window);

	util_x11_fini_window (dpy, window);

	util_x11_fini_display (dpy);

        return 0;
}
