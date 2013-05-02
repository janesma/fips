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

#include "dlwrap.h"

#include "glwrap.h"

void
glXSwapBuffers (Display *dpy, GLXDrawable drawable)
{
	GLWRAP_DEFER (glXSwapBuffers, dpy, drawable);

	glwrap_end_frame ();
}


typedef __GLXextFuncPtr (* fips_glXGetProcAddressARB_t)(const GLubyte *func);
__GLXextFuncPtr
glXGetProcAddressARB (const GLubyte *func)
{
	__GLXextFuncPtr ptr;
	static fips_glXGetProcAddressARB_t real_glXGetProcAddressARB = NULL;
	char *name = "glXGetProcAddressARB";

	if (! real_glXGetProcAddressARB) {
		real_glXGetProcAddressARB = glwrap_lookup (name);
		if (! real_glXGetProcAddressARB) {
			fprintf (stderr, "Error: Failed to find function %s.\n",
				 name);
			return NULL;
		}
	}

	/* If our library has this symbol, that's what we want to give. */
	ptr = dlwrap_real_dlsym (NULL, (const char *) func);
	if (ptr)
		return ptr;

	/* Otherwise, just defer to the real glXGetProcAddressARB. */
	return real_glXGetProcAddressARB (func);
}
