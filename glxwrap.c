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

#include "fips.h"

#include <X11/Xlib.h>
#include <GL/gl.h>
#include <GL/glx.h>

#include <sys/time.h>

#include "dlwrap.h"

typedef void (* fips_glXSwapBuffers_t)(Display *dpy, GLXDrawable drawable);

static void *
lookup (const char *name)
{
	const char *libgl_filename = "libGL.so.1";
	static void *libgl_handle = NULL;

	if (! libgl_handle) {
		libgl_handle = dlwrap_real_dlopen (libgl_filename, RTLD_NOW | RTLD_DEEPBIND);
		if (! libgl_handle) {
			fprintf (stderr, "Error: Failed to dlopen %s\n",
				 libgl_filename);
			exit (1);
		}
	}

	return dlwrap_real_dlsym (libgl_handle, name);
}

static void
call_glXSwapBuffers (Display *dpy, GLXDrawable drawable)
{
	static fips_glXSwapBuffers_t real_glXSwapBuffers = NULL;
	const char *name = "glXSwapBuffers";

	if (! real_glXSwapBuffers) {
		real_glXSwapBuffers = (fips_glXSwapBuffers_t) lookup (name);
		if (! real_glXSwapBuffers) {
			fprintf (stderr, "Error: Failed to find function %s.\n",
				 name);
			return;
		}
	}
	real_glXSwapBuffers (dpy, drawable);
}	

void
glXSwapBuffers (Display *dpy, GLXDrawable drawable)
{
	static int initialized = 0;
	static int frames;
	static struct timeval tv_start, tv_now;

	if (! initialized) {
		frames = 0;
		gettimeofday (&tv_start, NULL);
		initialized = 1;
	}

	call_glXSwapBuffers (dpy, drawable);

	frames++;
	if (frames % 60 == 0) {
		double fps;
		gettimeofday (&tv_now, NULL);

		fps = (double) frames / (tv_now.tv_sec - tv_start.tv_sec +
					 (tv_now.tv_usec - tv_start.tv_usec) / 1.0e6);

		printf("FPS: %.3f\n", fps);
	}
}


typedef __GLXextFuncPtr (* fips_glXGetProcAddressARB_t)(const GLubyte *func);
__GLXextFuncPtr
glXGetProcAddressARB (const GLubyte *func)
{
	static fips_glXGetProcAddressARB_t real_glXGetProcAddressARB = NULL;
	const char *name = "glXGetProcAddressARB";

	if (! real_glXGetProcAddressARB) {
		real_glXGetProcAddressARB = (fips_glXGetProcAddressARB_t) lookup (name);
		if (! real_glXGetProcAddressARB) {
			fprintf (stderr, "Error: Failed to find function %s.\n",
				 name);
			return NULL;
		}
	}

	if (strcmp ((const char *)func, "glXSwapBuffers") == 0)
		return (__GLXextFuncPtr) glXSwapBuffers;
	else
		return real_glXGetProcAddressARB (func);
}
