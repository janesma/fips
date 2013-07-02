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

/* This is C code intended to be included (yes, included) by test
 * programs in the fips test suite.
 *
 * Before including this file, the test program must define a macro
 * COMMON_GL_PREFIX to some prefix. This macro can be empty (to
 * directly call functions in an OpenGL-library directly linked) or
 * can be some custom prefix to call through symbols defined in the
 * test program.
 *
 * Optionally, the test program may define the macro COMMON_USE_EGL to
 * instruct the common code to use EGL (rather than GLX which it will
 * use by default if COMMON_USE_EGL is not defined).
 *
 * If COMMON_USE_EGL is not defined, this file provides three
 * functions for use by the test program:
 *
 *	static void
 *	common_create_glx_context (Display *dpy,
 *				   GLXContext *ctx, XVisualInfo **vi);
 *
 *	static void
 *	common_make_current (Display *dpy, GLXContext ctx, Window window)
 *
 *	static void
 *	common_handle_events (Display *dpy, GLXContext ctx, Window window);
 *
 * If COMMON_USE_EGL is defined, this file instead provides three
 * similar functions:
 *
 *	static void
 *	common_create_egl_context (Display *dpy, EGLenum api,
 *				   EGLDisplay *egl_dpy, EGLContext *ctx,
 *				   EGLConfig *config, XVisualInfo **vi);
 *
 *	static void
 *	common_make_current (EGLDisplay egl_dpy, EGLContext ctx,
 *			     EGLSurface surface)
 *
 *	static void
 *	common_handle_event (Display *dpy, EGLDisplay egl_dpy,
 *			     EGLSurface surface);
 */

#include <stdio.h>
#include <stdlib.h>

#ifndef COMMON_GL_PREFIX
#error Code including common.c must define COMMON_GL_PREFIX
#endif

#ifdef COMMON_USE_EGL
#ifndef COMMON_EGL_PREFIX
#error Code including common.c with COMMON_USE_EGL must define COMMON_EGL_PREFIX
#endif
#define COMMON_CONTEXT EGLContext
#define COMMON_SWAP_DISPLAY EGLDisplay
#define COMMON_SWAP_WINDOW EGLSurface
#define COMMON_SWAP_FUNCTION _E(eglSwapBuffers)
#else
#define COMMON_CONTEXT GLXContext
#define COMMON_SWAP_DISPLAY Display*
#define COMMON_SWAP_WINDOW Window
#define COMMON_SWAP_FUNCTION _(glXSwapBuffers)
#endif

#define concat_(a,b) a ## b
#define concat(a,b) concat_(a,b)
#define _(func) concat(COMMON_GL_PREFIX, func)
#define _E(func) concat(COMMON_EGL_PREFIX, func)

static void
paint_rgb_using_clear (double r, double g, double b)
{
        _(glClearColor) (r, g, b, 1.0);
        _(glClear) (GL_COLOR_BUFFER_BIT);
}

#ifdef COMMON_USE_EGL
static void
common_create_egl_context (Display *dpy, EGLenum api, EGLDisplay *egl_dpy_ret,
			   EGLContext *ctx_ret, EGLConfig *config_ret,
			   XVisualInfo **visual_info_ret)
{
	EGLDisplay egl_dpy;
	int config_attr[] = {
                EGL_RED_SIZE,		8,
                EGL_GREEN_SIZE, 	8,
                EGL_BLUE_SIZE,		8,
                EGL_ALPHA_SIZE, 	8,
		EGL_SURFACE_TYPE,	EGL_WINDOW_BIT,
		EGL_RENDERABLE_TYPE,	EGL_OPENGL_BIT,
                EGL_NONE
        };
	EGLConfig config;
	int num_configs;
	EGLint major, minor;
	EGLBoolean success;
	int opengl_context_attr[] = {
		EGL_NONE
	};
	int glesv2_context_attr[] = {
		EGL_CONTEXT_CLIENT_VERSION, 2,
		EGL_NONE
	};
	int *context_attr;
	EGLContext ctx;
	XVisualInfo *visual_info, visual_attr;
	int visualid, num_visuals;

	egl_dpy = _E(eglGetDisplay) (dpy);

	success = _E(eglInitialize) (egl_dpy, &major, &minor);
	if (!success) {
		fprintf (stderr, "Error: Failed to initialized EGL\n");
		exit (1);
	}

	success = _E(eglChooseConfig) (egl_dpy, config_attr, &config, 1, &num_configs);
	if (!success || num_configs == 0) {
		fprintf (stderr, "Error: Failed to find EGL config\n");
		exit (1);
	}

	success = _E(eglGetConfigAttrib) (egl_dpy, config, EGL_NATIVE_VISUAL_ID, &visualid);
	if (!success) {
		fprintf (stderr, "Error: Failed to find native Visual ID\n");
		exit (1);
	}

	visual_attr.visualid = visualid;
	visual_info = XGetVisualInfo (dpy, VisualIDMask, &visual_attr, &num_visuals);
	if (!visual_info) {
		fprintf (stderr, "Error: Failed to get XVisualInfo\n");
		exit (1);
	}

	_E(eglBindAPI) (api);

	if (api == EGL_OPENGL_ES_API)
		context_attr = glesv2_context_attr;
	else
		context_attr = opengl_context_attr;
	ctx = _E(eglCreateContext) (egl_dpy, config, NULL, context_attr);
	if (!ctx) {
		fprintf (stderr, "Error: Failed to create EGL context\n");
		exit (1);
	}

	*egl_dpy_ret = egl_dpy;
	*ctx_ret = ctx;
	*config_ret = config;
	*visual_info_ret = visual_info;
	
}
#else
static void
common_create_glx_context (Display *dpy,
			   GLXContext *ctx_ret, XVisualInfo **visual_info_ret)
{
        int visual_attr[] = {
                GLX_RGBA,
                GLX_RED_SIZE,		8,
                GLX_GREEN_SIZE, 	8,
                GLX_BLUE_SIZE,		8,
                GLX_ALPHA_SIZE, 	8,
                GLX_DOUBLEBUFFER,
                GLX_DEPTH_SIZE,		24,
                GLX_STENCIL_SIZE,	8,
                GLX_X_VISUAL_TYPE,	GLX_DIRECT_COLOR,
                None
        };

        *visual_info_ret = _(glXChooseVisual) (dpy, 0, visual_attr);
        *ctx_ret = _(glXCreateContext) (dpy, *visual_info_ret, NULL, True);
}
#endif

#ifdef COMMON_USE_EGL
static void
common_make_current (EGLDisplay egl_dpy, EGLContext ctx, EGLSurface surface)
{
	_E(eglMakeCurrent) (egl_dpy, surface, surface, ctx);
}
#else
static void
common_make_current (Display *dpy, GLXContext ctx, Window window)
{
	_(glXMakeCurrent) (dpy, window, ctx);
}
#endif

static void
draw (COMMON_SWAP_DISPLAY dpy, COMMON_SWAP_WINDOW window, int width, int height)
{
	int i;
        _(glViewport) (0, 0, width, height);

/* Simply count through some colors, frame by frame. */
#define RGB(frame) (((frame+1)/4) % 2), (((frame+1)/2) % 2), ((frame+1) % 2)

	int frame = 0;
	for (i = 0; i < 2; i++) {
		/* Frame: Draw a solid (magenta) frame */
		paint_rgb_using_clear (RGB(frame));
		COMMON_SWAP_FUNCTION (dpy, window);
		frame++;

		/* Frame: Draw a solid (yellow) frame */
		paint_rgb_using_clear (RGB(frame));
		COMMON_SWAP_FUNCTION (dpy, window);
		frame++;

		/* Frame: Draw a solid (cyan) frame */
		paint_rgb_using_clear (RGB(frame));
		COMMON_SWAP_FUNCTION (dpy, window);
		frame++;
	}
}

static void
common_handle_events(Display *dpy, COMMON_SWAP_DISPLAY swap_dpy, COMMON_SWAP_WINDOW window)
{
        XEvent xev;
	int width = 0;
	int height = 0;

        XNextEvent (dpy, &xev);

        while (1) {
                XNextEvent (dpy, &xev);
                switch (xev.type) {
                case ConfigureNotify:
                        width = xev.xconfigure.width;
                        height = xev.xconfigure.height;
                        break;
                case Expose:
                        if (xev.xexpose.count == 0) {
                                draw (swap_dpy, window, width, height);
                                return;
                        }
                        break;
                }
        }
}
