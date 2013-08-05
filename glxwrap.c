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

#include <X11/Xlib.h>
#include <GL/gl.h>
#include <GL/glx.h>

#include "dlwrap.h"
#include "glwrap.h"
#include "metrics.h"

void
glXSwapBuffers (Display *dpy, GLXDrawable drawable)
{
	GLWRAP_DEFER (glXSwapBuffers, dpy, drawable);

	metrics_end_frame ();
}

/* glXGetProcAddressARB is a function which accepts a string and
 * returns a generic function pointer (which nominally accepts void and
 * has void return type). Of course, the user is expected to cast the
 * returned function pointer to a function pointer of the expected
 * type.
 */
void (*glXGetProcAddressARB (const GLubyte *func))(void)
{
	static void *libfips_handle = NULL;
	void *ret;

	if (libfips_handle == NULL)
		libfips_handle = dlwrap_dlopen_libfips ();

	/* If our library has this symbol, that's what we want to give. */
	ret = dlwrap_real_dlsym (libfips_handle, (const char *) func);
	if (ret)
		return ret;

	/* Otherwise, just defer to the real glXGetProcAddressARB. */
	GLWRAP_DEFER_WITH_RETURN (ret, glXGetProcAddressARB, func);

	return ret;
}

void (*glXGetProcAddress (const GLubyte *func))(void)
{
	/* This comment must not be removed. It ensures that the
	 * glXGetProcAddress function ends up in our exported symbol
	 * list even though there's not otherwise any code saying:
	 *
	 * GLWRAP_DEFER_WITH_RETURN (ret, glXGetProcAddress, func);
	 */
	return glXGetProcAddressARB(func);
}

Bool
glXMakeCurrent (Display *dpy, GLXDrawable drawable, GLXContext ctx)
{
	Bool ret;

	fips_dispatch_init (FIPS_API_GLX);

	GLWRAP_DEFER_WITH_RETURN (ret, glXMakeCurrent, dpy, drawable, ctx);

	return ret;
}
