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

#include "fips-dispatch.h"

#include <EGL/egl.h>

#include "context.h"
#include "eglwrap.h"
#include "dlwrap.h"
#include "metrics.h"

/* Defer to the real 'function' (from libEGL.so.1) to do the real work.
 * The symbol is looked up once and cached in a static variable for
 * future uses.
 */
#define EGLWRAP_DEFER(function,...) do {			\
	static typeof(&function) real_ ## function;		\
	if (! real_ ## function)				\
		real_ ## function = eglwrap_lookup (#function);	\
	real_ ## function(__VA_ARGS__); 			\
} while (0);

/* As EGLWRAP_DEFER, but also set 'ret' to the return value */
#define EGLWRAP_DEFER_WITH_RETURN(ret, function,...) do {	\
	static typeof(&function) real_ ## function;		\
	if (! real_ ## function)				\
		real_ ## function = eglwrap_lookup (#function);	\
	(ret) = real_ ## function(__VA_ARGS__); 		\
} while (0);


/* Note: We only need to perform a lookup in libEGL.so.1, (not
 * libGLESv2.so.2). This is because the functions we wrap, (currently
 * eglSwapBufers, eglGetProcAddress, and eglMakeCurrent), exist only
 * in libEGL.so.1.
 */
void *
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

	EGLWRAP_DEFER_WITH_RETURN (ret, eglSwapBuffers, dpy, surface);

	context_counter_stop ();

	context_end_frame ();

	context_counter_start ();

	return ret;
}

void (*eglGetProcAddress (char const *func))(void)
{
	void *ret;

	/* If our library has this symbol, that's what we want to give. */
	ret = dlwrap_real_dlsym (NULL, (const char *) func);
	if (ret)
		return ret;

	/* Otherwise, just defer to the real eglGetProcAddress */
	EGLWRAP_DEFER_WITH_RETURN (ret, eglGetProcAddress, func);

	return ret;
}

EGLBoolean
eglMakeCurrent (EGLDisplay display, EGLSurface draw, EGLSurface read,
		EGLContext context)
{
	EGLBoolean ret;

	context_leave ();

	EGLWRAP_DEFER_WITH_RETURN (ret, eglMakeCurrent, display, draw, read, context);

	context_enter (FIPS_API_EGL, context);

	return ret;
}
