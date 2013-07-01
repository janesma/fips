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

#include <stdio.h>
#include <stdlib.h>

#include "util-x11.h"

void
util_x11_init_display (Display **dpy)
{
        *dpy = XOpenDisplay (NULL);

        if (*dpy == NULL) {
                fprintf(stderr, "Failed to open display %s\n",
                        XDisplayName(NULL));
                exit (1);
        }
}

void
util_x11_fini_display (Display *dpy)
{
        XCloseDisplay (dpy);
}

void
util_x11_init_window (Display *dpy, Window *window)
{
	int width = 64;
	int height = 64;



        *window = XCreateSimpleWindow(dpy, DefaultRootWindow (dpy),
				      0, 0, width, height, 0,
				      BlackPixel (dpy, DefaultScreen (dpy)),
				      BlackPixel (dpy, DefaultScreen (dpy)));

        XSelectInput(dpy, *window,
                     KeyPressMask | StructureNotifyMask | ExposureMask);

        XMapWindow (dpy, *window);

}

void
util_x11_fini_window (Display *dpy, Window window)
{
        XDestroyWindow (dpy, window);
}
