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
 *	2. By using dlopen to dynamically load libGLESv2.so and libEGL.so
 *	3. By using dlsym to lookup OpenGL functions
 */

#define GL_GLEXT_PROTOTYPES
#include <GLES2/gl2.h>
#include <EGL/egl.h>

#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>

#include "util-x11.h"

EGLBoolean (*my_eglBindAPI)(EGLenum);
EGLBoolean (*my_eglChooseConfig)(EGLDisplay, EGLint const *, EGLConfig *, EGLint, EGLint *);
EGLContext (*my_eglCreateContext)(EGLDisplay, EGLConfig, EGLContext, EGLint const *);
EGLSurface (*my_eglCreateWindowSurface)(EGLDisplay, EGLConfig, NativeWindowType, EGLint const *);
EGLBoolean (*my_eglGetConfigAttrib)(EGLDisplay, EGLConfig, EGLint, EGLint *);
EGLDisplay (*my_eglGetDisplay)(NativeDisplayType);
EGLBoolean (*my_eglInitialize)(EGLDisplay, EGLint *, EGLint *);
EGLBoolean (*my_eglMakeCurrent)(EGLDisplay, EGLSurface, EGLSurface, EGLContext);
EGLBoolean (*my_eglSwapBuffers)(EGLDisplay, EGLSurface);
void (*my_glClear)(GLbitfield);
void (*my_glClearColor)(GLclampf, GLclampf, GLclampf, GLclampf);
void (*my_glViewport)(GLint, GLint, GLsizei, GLsizei);

#define COMMON_USE_EGL
#define COMMON_GL_PREFIX my_
#define COMMON_EGL_PREFIX my_
#include "common.c"

static void
resolve_symbols (void)
{
	void *glesv2_handle;
	void *egl_handle;
	char *error;

	glesv2_handle = dlopen ("libGLESv2.so", RTLD_LAZY);
	if (glesv2_handle == NULL) {
		fprintf (stderr, "Error: Failed to open libGL.so\n");
		exit (1);
	}

	egl_handle = dlopen ("libEGL.so", RTLD_LAZY);
	if (egl_handle == NULL) {
		fprintf (stderr, "Error: Failed to open libEGL.so\n");
		exit (1);
	}

	/* Clear errors. */
	dlerror();

	my_eglBindAPI = dlsym (egl_handle, "eglBindAPI");
	error = dlerror ();
	if (error) {
		fprintf (stderr, "Failed to dlsym eglBindAPI: %s\n", error);
		exit (1);
	}

	my_eglChooseConfig = dlsym (egl_handle, "eglChooseConfig");
	error = dlerror ();
	if (error) {
		fprintf (stderr, "Failed to dlsym eglChooseConfig: %s\n", error);
		exit (1);
	}

	my_eglCreateContext = dlsym (egl_handle, "eglCreateContext");
	error = dlerror ();
	if (error) {
		fprintf (stderr, "Failed to dlsym eglCreateContext: %s\n", error);
		exit (1);
	}

	my_eglCreateWindowSurface = dlsym (egl_handle, "eglCreateWindowSurface");
	error = dlerror ();
	if (error) {
		fprintf (stderr, "Failed to dlsym eglCreateWindowSurface: %s\n", error);
		exit (1);
	}

	my_eglGetConfigAttrib = dlsym (egl_handle, "eglGetConfigAttrib");
	error = dlerror ();
	if (error) {
		fprintf (stderr, "Failed to dlsym eglGetConfigAttrib: %s\n", error);
		exit (1);
	}

	my_eglGetDisplay = dlsym (egl_handle, "eglGetDisplay");
	error = dlerror ();
	if (error) {
		fprintf (stderr, "Failed to dlsym eglGetDisplay: %s\n", error);
		exit (1);
	}

	my_eglInitialize = dlsym (egl_handle, "eglInitialize");
	error = dlerror ();
	if (error) {
		fprintf (stderr, "Failed to dlsym eglInitialize: %s\n", error);
		exit (1);
	}

	my_eglMakeCurrent = dlsym (egl_handle, "eglMakeCurrent");
	error = dlerror ();
	if (error) {
		fprintf (stderr, "Failed to dlsym eglMakeCurrent: %s\n", error);
		exit (1);
	}

	my_eglSwapBuffers = dlsym (egl_handle, "eglSwapBuffers");
	error = dlerror ();
	if (error) {
		fprintf (stderr, "Failed to dlsym eglSwapBuffers: %s\n", error);
		exit (1);
	}

	my_glClear = dlsym (glesv2_handle, "glClear");
	error = dlerror ();
	if (error) {
		fprintf (stderr, "Failed to dlsym glClear: %s\n", error);
		exit (1);
	}

	my_glClearColor = dlsym (glesv2_handle, "glClearColor");
	error = dlerror ();
	if (error) {
		fprintf (stderr, "Failed to dlsym glClearColor: %s\n", error);
		exit (1);
	}

	my_glViewport = dlsym (glesv2_handle, "glViewport");
	error = dlerror ();
	if (error) {
		fprintf (stderr, "Failed to dlsym glViewport: %s\n", error);
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

	surface = my_eglCreateWindowSurface (egl_dpy, config, window, NULL);

	common_make_current (egl_dpy, ctx, surface);

        common_handle_events (dpy, egl_dpy, surface);

	util_x11_fini_window (dpy, window);

	util_x11_fini_display (dpy);

        return 0;
}
