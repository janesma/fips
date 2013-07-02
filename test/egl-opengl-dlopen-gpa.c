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
 *	2. By using dlopen to dynamically load libGL.so
 *	3. By using eglGetProcAddress to lookup OpenGL functions
 */

#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
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
void * (*my_eglGetProcAddress)(char const *);
EGLBoolean (*my_eglInitialize)(EGLDisplay, EGLint *, EGLint *);
EGLBoolean (*my_eglMakeCurrent)(EGLDisplay, EGLSurface, EGLSurface, EGLContext);
EGLBoolean (*my_eglSwapBuffers)(EGLDisplay, EGLSurface);

/* The OpenGL header files give typedefs for the types of all
 * functions provided by etensisons. Here, though, we're using
 * glxGetProcAddress to lookup core functions, so we provide our own
 * typedefs. */
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

#define COMMON_USE_EGL
#define COMMON_GL_PREFIX my_
#define COMMON_EGL_PREFIX my_
#include "common.c"

static void
resolve_symbols (void)
{
	void *egl_handle, *gl_handle;
	char *error;

	egl_handle = dlopen ("libEGL.so", RTLD_LAZY);
	if (egl_handle == NULL) {
		fprintf (stderr, "Error: Failed to open libEGL.so\n");
		exit (1);
	}

	gl_handle = dlopen ("libGL.so", RTLD_LAZY);
	if (gl_handle == NULL) {
		fprintf (stderr, "Error: Failed to open libGL.so\n");
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

	my_eglGetProcAddress = dlsym (egl_handle, "eglGetProcAddress");
	error = dlerror ();
	if (error) {
		fprintf (stderr, "Failed to dlsym eglGetProcAddress: %s\n", error);
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

	my_glClear = (FIPS_GLCLEAR_FN) my_eglGetProcAddress ("glClear");
	if (my_glClear == NULL) {
		fprintf (stderr, "Failed to eglGetProcAddress glClear\n");
		exit (1);
	}

	my_glClearColor = (FIPS_GLCLEARCOLOR_FN) my_eglGetProcAddress ("glClearColor");
	if (my_glClearColor == NULL) {
		fprintf (stderr, "Failed to eglGetProcAddress glClearColor\n");
		exit (1);
	}

	my_glLoadIdentity = (FIPS_GLLOADIDENTITY_FN) my_eglGetProcAddress ("glLoadIdentity");
	if (my_glLoadIdentity == NULL) {
		fprintf (stderr, "Failed to eglGetProcAddress glLoadIdentity\n");
		exit (1);
	}

	my_glMatrixMode = (FIPS_GLMATRIXMODE_FN) my_eglGetProcAddress ("glMatrixMode");
	if (my_glMatrixMode == NULL) {
		fprintf (stderr, "Failed to eglGetProcAddress glMatrixMode\n");
		exit (1);
	}

	my_glOrtho = (FIPS_GLORTHO_FN) my_eglGetProcAddress ("glOrtho");
	if (my_glOrtho == NULL) {
		fprintf (stderr, "Failed to eglGetProcAddress glOrtho\n");
		exit (1);
	}

	my_glViewport = (FIPS_GLVIEWPORT_FN) my_eglGetProcAddress ("glViewport");
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

	common_create_egl_context (dpy, EGL_OPENGL_API, &egl_dpy,
				   &ctx, &config, &visual_info);

	window = util_x11_init_window (dpy, visual_info);

	surface = my_eglCreateWindowSurface (egl_dpy, config, window, NULL);

	common_make_current (egl_dpy, ctx, surface);

        common_handle_events (dpy, egl_dpy, surface);

	util_x11_fini_window (dpy, window);

	util_x11_fini_display (dpy);

        return 0;
}
