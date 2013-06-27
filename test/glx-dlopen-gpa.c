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

#include "util.h"

void* (*my_glXGetProcAddress) (char *);
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

static void
set_2d_projection (int width, int height)
{
	my_glMatrixMode (GL_PROJECTION);
	my_glLoadIdentity ();
	my_glOrtho (0, width, height, 0, 0, 1);
	my_glMatrixMode (GL_MODELVIEW);
}

static void
paint_rgb_using_clear (double r, double g, double b)
{
        my_glClearColor(r, g, b, 1.0);
        my_glClear(GL_COLOR_BUFFER_BIT);
}

static void
draw (Display *dpy, Window window, int width, int height)
{
	int i;
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

	/* Window and context setup. */
        XVisualInfo *visual_info = my_glXChooseVisual(dpy, 0, visual_attr);
        GLXContext ctx = my_glXCreateContext(dpy, visual_info, NULL, True);
        my_glXMakeCurrent(dpy, window, ctx);

        my_glViewport(0, 0, width, height);

	set_2d_projection (width, height);

/* Simply count through some colors, frame by frame. */
#define RGB(frame) (((frame+1)/4) % 2), (((frame+1)/2) % 2), ((frame+1) % 2)

	int frame = 0;
	for (i = 0; i < 2; i++) {
		/* Frame: Draw a solid (magenta) frame */
		paint_rgb_using_clear (RGB(frame));
		my_glXSwapBuffers (dpy, window);
		frame++;

		/* Frame: Draw a solid (yellow) frame */
		paint_rgb_using_clear (RGB(frame));
		my_glXSwapBuffers (dpy, window);
		frame++;

		/* Frame: Draw a solid (cyan) frame */
		paint_rgb_using_clear (RGB(frame));
		my_glXSwapBuffers (dpy, window);
		frame++;
	}

	/* Cleanup */
        my_glXDestroyContext (dpy, ctx);
}

static void
handle_events(Display *dpy, Window window)
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
                                draw (dpy, window, width, height);
                                return;
                        }
                        break;
                }
        }
}

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

	my_glXGetProcAddress = dlsym (gl_handle, "glXGetProcAddress");
	error = dlerror ();
	if (error) {
		fprintf (stderr, "Failed to dlsym glXGetProcAddress: %s\n", error);
		exit (1);
	}

	my_glClear = my_glXGetProcAddress ("glClear");
	if (my_glClear == NULL) {
		fprintf (stderr, "Failed to glXGetProcAddress glClear\n");
		exit (1);
	}

	my_glClearColor = my_glXGetProcAddress ("glClearColor");
	if (my_glClearColor == NULL) {
		fprintf (stderr, "Failed to glXGetProcAddress glClearColor\n");
		exit (1);
	}

	my_glLoadIdentity = my_glXGetProcAddress ("glLoadIdentity");
	if (my_glLoadIdentity == NULL) {
		fprintf (stderr, "Failed to glXGetProcAddress glLoadIdentity\n");
		exit (1);
	}

	my_glMatrixMode = my_glXGetProcAddress ("glMatrixMode");
	if (my_glMatrixMode == NULL) {
		fprintf (stderr, "Failed to glXGetProcAddress glMatrixMode\n");
		exit (1);
	}

	my_glOrtho = my_glXGetProcAddress ("glOrtho");
	if (my_glOrtho == NULL) {
		fprintf (stderr, "Failed to glXGetProcAddress glOrtho\n");
		exit (1);
	}

	my_glViewport = my_glXGetProcAddress ("glViewport");
	if (my_glViewport == NULL) {
		fprintf (stderr, "Failed to glXGetProcAddress glViewport\n");
		exit (1);
	}

	my_glXChooseVisual = my_glXGetProcAddress ("glXChooseVisual");
	if (my_glXChooseVisual == NULL) {
		fprintf (stderr, "Failed to glXGetProcAddress glXChooseVisual\n");
		exit (1);
	}

	my_glXCreateContext = my_glXGetProcAddress ("glXCreateContext");
	if (my_glXCreateContext == NULL) {
		fprintf (stderr, "Failed to glXGetProcAddress glXCreateContext\n");
		exit (1);
	}

	my_glXDestroyContext = my_glXGetProcAddress ("glXDestroyContext");
	if (my_glXDestroyContext == NULL) {
		fprintf (stderr, "Failed to glXGetProcAddress glXDestroyContext\n");
		exit (1);
	}

	my_glXMakeCurrent = my_glXGetProcAddress ("glXMakeCurrent");
	if (my_glXMakeCurrent == NULL) {
		fprintf (stderr, "Failed to glXGetProcAddress glXMakeCurrent\n");
		exit (1);
	}

	my_glXSwapBuffers = my_glXGetProcAddress ("glXSwapBuffers");
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

	util_init_display_window (&dpy, &window);

	resolve_symbols ();

        handle_events (dpy, window);

	util_fini_display_window (dpy, window);

        return 0;
}
