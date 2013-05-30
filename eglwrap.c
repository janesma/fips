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

#include <EGL/egl.h>

#include "dlwrap.h"
#include "metrics.h"

static void *
eglwrap_lookup (char *name)
{
	const char *libegl_filename = "libEGL.so.1";
	static void *libegl_handle = NULL;

	if (! libegl_handle) {
		libegl_handle = dlwrap_real_dlopen (libegl_filename, RTLD_NOW | RTLD_DEEPBIND);
		if (! libegl_handle) {
			fprintf (stderr, "Error: Failed to dlopen %s\n",
				 libegl_filename);
			exit (1);
		}
	}

	return dlwrap_real_dlsym (libegl_handle, name);
}

EGLBoolean
eglSwapBuffers (EGLDisplay dpy, EGLSurface surface)
{
	EGLBoolean ret;
	static typeof(&eglSwapBuffers) eglwrap_real_eglSwapBuffers;

	if (! eglwrap_real_eglSwapBuffers)
		eglwrap_real_eglSwapBuffers = eglwrap_lookup ("eglSwapBuffers");

	ret = eglwrap_real_eglSwapBuffers (dpy, surface);

	metrics_end_frame ();

	return ret;
}
