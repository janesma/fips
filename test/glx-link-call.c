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
 *	3. By directly calling OpenGL functions
 */

#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glx.h>

#include "util.h"

static void
set_2d_projection (int width, int height)
{
	glMatrixMode (GL_PROJECTION);
	glLoadIdentity ();
	glOrtho (0, width, height, 0, 0, 1);
	glMatrixMode (GL_MODELVIEW);
}

static void
paint_rgb_using_clear (double r, double g, double b)
{
        glClearColor(r, g, b, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);
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
        XVisualInfo *visual_info = glXChooseVisual(dpy, 0, visual_attr);
        GLXContext ctx = glXCreateContext(dpy, visual_info, NULL, True);
        glXMakeCurrent(dpy, window, ctx);

        glViewport(0, 0, width, height);

	set_2d_projection (width, height);

/* Simply count through some colors, frame by frame. */
#define RGB(frame) (((frame+1)/4) % 2), (((frame+1)/2) % 2), ((frame+1) % 2)

	int frame = 0;
	for (i = 0; i < 2; i++) {
		/* Frame: Draw a solid (magenta) frame */
		paint_rgb_using_clear (RGB(frame));
		glXSwapBuffers (dpy, window);
		frame++;

		/* Frame: Draw a solid (yellow) frame */
		paint_rgb_using_clear (RGB(frame));
		glXSwapBuffers (dpy, window);
		frame++;

		/* Frame: Draw a solid (cyan) frame */
		paint_rgb_using_clear (RGB(frame));
		glXSwapBuffers (dpy, window);
		frame++;
	}

	/* Cleanup */
        glXDestroyContext (dpy, ctx);
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

int
main (void)
{
        Display *dpy;
        Window window;

	util_init_display_window (&dpy, &window);

        handle_events (dpy, window);

	util_fini_display_window (dpy, window);

        return 0;
}
