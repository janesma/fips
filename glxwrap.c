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

#include "context.h"
#include "glwrap.h"
#include "metrics.h"
#include "publish.h"

void
glXSwapBuffers (Display *dpy, GLXDrawable drawable)
{
	FIPS_DEFER (glXSwapBuffers, dpy, drawable);

	context_counter_stop ();

	context_end_frame ();

	context_counter_start ();
	publish();
}

Bool
glXMakeCurrent (Display *dpy, GLXDrawable drawable, GLXContext ctx)
{
	Bool ret;

	context_leave (ctx);

	FIPS_DEFER_WITH_RETURN (ret, glXMakeCurrent, dpy, drawable, ctx);

	context_enter (FIPS_API_GLX, ctx);
	grafips_context_init();
	return ret;
}

Bool
glXMakeContextCurrent (Display *dpy, GLXDrawable drawable, GLXDrawable read, GLXContext ctx)
{
	Bool ret;

	context_leave (ctx);

	FIPS_DEFER_WITH_RETURN (ret, glXMakeContextCurrent, dpy, drawable, read, ctx);

	context_enter (FIPS_API_GLX, ctx);
	grafips_context_init();
	return ret;
}

