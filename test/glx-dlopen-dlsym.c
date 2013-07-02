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
 *	2. By using dlopen to dynamically load libGL.so
 *	3. By using dlsym to lookup OpenGL functions
 */

#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glx.h>

#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>

#include "util-x11.h"

void (*my_glClear) (GLbitfield);
void (*my_glClearColor) (GLclampf, GLclampf, GLclampf, GLclampf);
void (*my_glLoadIdentity) (void);
void (*my_glMatrixMode) (GLenum);
void (*my_glOrtho) (GLdouble, GLdouble, GLdouble, GLdouble, GLdouble, GLdouble);
void (*my_glViewport) (GLint, GLint, GLsizei, GLsizei);
XVisualInfo * (*my_glXChooseVisual) (Display *, int, int *);
GLXContext (*my_glXCreateContext) (Display *, XVisualInfo *, GLXContext, Bool);
void (*my_glXDestroyContext) (Display *, GLXContext);
Bool (*my_glXMakeCurrent) (Display *, GLXDrawable, GLXContext);
void (*my_glXSwapBuffers) (Display *, GLXDrawable);

#define COMMON_GL_PREFIX my_
#include "common.c"

static void
resolve_symbols (void)
{
	void *gl_handle;
	char *error;

	gl_handle = dlopen ("libGL.so", RTLD_LAZY);
	if (gl_handle == NULL) {
		fprintf (stderr, "Error: Failed to open libGL.so\n");
		exit (1);
	}

	/* Clear errors. */
	dlerror();

	my_glClear = dlsym (gl_handle, "glClear");
	error = dlerror ();
	if (error) {
		fprintf (stderr, "Failed to dlsym glClear: %s\n", error);
		exit (1);
	}

	my_glClearColor = dlsym (gl_handle, "glClearColor");
	error = dlerror ();
	if (error) {
		fprintf (stderr, "Failed to dlsym glClearColor: %s\n", error);
		exit (1);
	}

	my_glLoadIdentity = dlsym (gl_handle, "glLoadIdentity");
	error = dlerror ();
	if (error) {
		fprintf (stderr, "Failed to dlsym glLoadIdentity: %s\n", error);
		exit (1);
	}

	my_glMatrixMode = dlsym (gl_handle, "glMatrixMode");
	error = dlerror ();
	if (error) {
		fprintf (stderr, "Failed to dlsym glMatrixMode: %s\n", error);
		exit (1);
	}

	my_glOrtho = dlsym (gl_handle, "glOrtho");
	error = dlerror ();
	if (error) {
		fprintf (stderr, "Failed to dlsym glOrtho: %s\n", error);
		exit (1);
	}

	my_glViewport = dlsym (gl_handle, "glViewport");
	error = dlerror ();
	if (error) {
		fprintf (stderr, "Failed to dlsym glViewport: %s\n", error);
		exit (1);
	}

	my_glXChooseVisual = dlsym (gl_handle, "glXChooseVisual");
	error = dlerror ();
	if (error) {
		fprintf (stderr, "Failed to dlsym glXChooseVisual: %s\n", error);
		exit (1);
	}

	my_glXCreateContext = dlsym (gl_handle, "glXCreateContext");
	error = dlerror ();
	if (error) {
		fprintf (stderr, "Failed to dlsym glXCreateContext: %s\n", error);
		exit (1);
	}

	my_glXDestroyContext = dlsym (gl_handle, "glXDestroyContext");
	error = dlerror ();
	if (error) {
		fprintf (stderr, "Failed to dlsym glXDestroyContext: %s\n", error);
		exit (1);
	}

	my_glXMakeCurrent = dlsym (gl_handle, "glXMakeCurrent");
	error = dlerror ();
	if (error) {
		fprintf (stderr, "Failed to dlsym glXMakeCurrent: %s\n", error);
		exit (1);
	}

	my_glXSwapBuffers = dlsym (gl_handle, "glXSwapBuffers");
	error = dlerror ();
	if (error) {
		fprintf (stderr, "Failed to dlsym glXSwapBuffers: %s\n", error);
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
