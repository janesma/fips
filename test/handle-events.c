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
 * It defines a function:
 *
 *	static void
 *	handle_events(Display *dpy, Window window);
 *
 * which handles X events, responding by drawing a few frames using
 * OpenGL.
 *
 * Before including this file, the test program must define a macro
 * HANDLE_EVENTS_GL_PREFIX to some prefix. This macro can be empty (to
 * directly call functions in an OpenGL-library directly linked) or
 * can be some custom prefix to call through symbols defined in the
 * test program.
 */

#ifndef HANDLE_EVENTS_GL_PREFIX
#error Code including handle-events.c must define HANDLE_EVENTS_GL_PREFIX
#endif

#define concat_(a,b) a ## b
#define concat(a,b) concat_(a,b)
#define _(func) concat(HANDLE_EVENTS_GL_PREFIX, func)

static void
set_2d_projection (int width, int height)
{
	_(glMatrixMode) (GL_PROJECTION);
	_(glLoadIdentity) ();
	_(glOrtho) (0, width, height, 0, 0, 1);
	_(glMatrixMode) (GL_MODELVIEW);
}

static void
paint_rgb_using_clear (double r, double g, double b)
{
        _(glClearColor) (r, g, b, 1.0);
        _(glClear) (GL_COLOR_BUFFER_BIT);
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
        XVisualInfo *visual_info = _(glXChooseVisual) (dpy, 0, visual_attr);
        GLXContext ctx = _(glXCreateContext) (dpy, visual_info, NULL, True);
        _(glXMakeCurrent) (dpy, window, ctx);

        _(glViewport) (0, 0, width, height);

	set_2d_projection (width, height);

/* Simply count through some colors, frame by frame. */
#define RGB(frame) (((frame+1)/4) % 2), (((frame+1)/2) % 2), ((frame+1) % 2)

	int frame = 0;
	for (i = 0; i < 2; i++) {
		/* Frame: Draw a solid (magenta) frame */
		paint_rgb_using_clear (RGB(frame));
		_(glXSwapBuffers) (dpy, window);
		frame++;

		/* Frame: Draw a solid (yellow) frame */
		paint_rgb_using_clear (RGB(frame));
		_(glXSwapBuffers) (dpy, window);
		frame++;

		/* Frame: Draw a solid (cyan) frame */
		paint_rgb_using_clear (RGB(frame));
		_(glXSwapBuffers) (dpy, window);
		frame++;
	}

	/* Cleanup */
        _(glXDestroyContext) (dpy, ctx);
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
