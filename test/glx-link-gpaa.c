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
 *	3. By using glXGetProcAddressARB to lookup OpenGL functions
 */

#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glx.h>

#include <stdio.h>
#include <stdlib.h>

#include "util-x11.h"

/* The OpenGL header files give typedefs for the types of all
 * functions provided by etensisons. Here, though, we're using
 * glxGetProcAddressARB to lookup core functions, so we provide our
 * own typedefs. */
typedef void (*FIPS_GLCLEAR_FN)(GLbitfield);
FIPS_GLCLEAR_FN my_glClear;

typedef void (*FIPS_GLCLEARCOLOR_FN)(GLclampf, GLclampf, GLclampf, GLclampf);
FIPS_GLCLEARCOLOR_FN my_glClearColor;

typedef void (*FIPS_GLLOADIDENTITY_FN)(void);
FIPS_GLLOADIDENTITY_FN my_glLoadIdentity;

typedef void (*FIPS_GLMATRIXMODE_FN)(GLenum);
FIPS_GLMATRIXMODE_FN my_glMatrixMode;

typedef void (*FIPS_GLORTHO_FN)(GLdouble, GLdouble, GLdouble, GLdouble, GLdouble, GLdouble);
FIPS_GLORTHO_FN my_glOrtho;

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
	my_glClear = (FIPS_GLCLEAR_FN) glXGetProcAddressARB ((GLubyte*) "glClear");
	if (my_glClear == NULL) {
		fprintf (stderr, "Failed to glXGetProcAddressARB glClear\n");
		exit (1);
	}

	my_glClearColor = (FIPS_GLCLEARCOLOR_FN) glXGetProcAddressARB ((GLubyte*) "glClearColor");
	if (my_glClearColor == NULL) {
		fprintf (stderr, "Failed to glXGetProcAddressARB glClearColor\n");
		exit (1);
	}

	my_glLoadIdentity = (FIPS_GLLOADIDENTITY_FN) glXGetProcAddressARB ((GLubyte*) "glLoadIdentity");
	if (my_glLoadIdentity == NULL) {
		fprintf (stderr, "Failed to glXGetProcAddressARB glLoadIdentity\n");
		exit (1);
	}

	my_glMatrixMode = (FIPS_GLMATRIXMODE_FN) glXGetProcAddressARB ((GLubyte*) "glMatrixMode");
	if (my_glMatrixMode == NULL) {
		fprintf (stderr, "Failed to glXGetProcAddressARB glMatrixMode\n");
		exit (1);
	}

	my_glOrtho = (FIPS_GLORTHO_FN) glXGetProcAddressARB ((GLubyte*) "glOrtho");
	if (my_glOrtho == NULL) {
		fprintf (stderr, "Failed to glXGetProcAddressARB glOrtho\n");
		exit (1);
	}

	my_glViewport = (FIPS_GLVIEWPORT_FN) glXGetProcAddressARB ((GLubyte*) "glViewport");
	if (my_glViewport == NULL) {
		fprintf (stderr, "Failed to glXGetProcAddressARB glViewport\n");
		exit (1);
	}

	my_glXChooseVisual = (FIPS_GLXCHOOSEVISUAL_FN) glXGetProcAddressARB ((GLubyte*) "glXChooseVisual");
	if (my_glXChooseVisual == NULL) {
		fprintf (stderr, "Failed to glXGetProcAddressARB glXChooseVisual\n");
		exit (1);
	}

	my_glXCreateContext = (FIPS_GLXCREATECONTEXT_FN) glXGetProcAddressARB ((GLubyte*) "glXCreateContext");
	if (my_glXCreateContext == NULL) {
		fprintf (stderr, "Failed to glXGetProcAddressARB glXCreateContext\n");
		exit (1);
	}

	my_glXDestroyContext = (FIPS_GLXDESTROYCONTEXT_FN) glXGetProcAddressARB ((GLubyte*) "glXDestroyContext");
	if (my_glXDestroyContext == NULL) {
		fprintf (stderr, "Failed to glXGetProcAddressARB glXDestroyContext\n");
		exit (1);
	}

	my_glXMakeCurrent = (FIPS_GLXMAKECURRENT_FN) glXGetProcAddressARB ((GLubyte*) "glXMakeCurrent");
	if (my_glXMakeCurrent == NULL) {
		fprintf (stderr, "Failed to glXGetProcAddressARB glXMakeCurrent\n");
		exit (1);
	}

	my_glXSwapBuffers = (FIPS_GLXSWAPBUFFERS_FN) glXGetProcAddressARB ((GLubyte*) "glXSwapBuffers");
	if (my_glXSwapBuffers == NULL) {
		fprintf (stderr, "Failed to glXGetProcAddressARB glXSwapBuffers\n");
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

	common_create_context (dpy, &ctx, &visual_info);

	window = util_x11_init_window (dpy, visual_info);

        common_handle_events (dpy, ctx, window);

	util_x11_fini_window (dpy, window);

	util_x11_fini_display (dpy);

        return 0;
}
