/* Copyright © 2012, Intel Corporation
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

#include "fips-dispatch.h"

#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glx.h>

#include <EGL/egl.h>

#include "glwrap.h"
#include "publish.h"

bool fips_dispatch_initialized;
fips_api_t fips_dispatch_api;

void
fips_dispatch_init (fips_api_t api)
{
	fips_dispatch_api = api;

    if (!fips_dispatch_initialized)
        create_publishers();

	fips_dispatch_initialized = true;
}

typedef void (*generic_function_pointer)(void);

void *
fips_dispatch_lookup (const char *name)
{
	static PFNGLXGETPROCADDRESSPROC glx_gpa = NULL;
	static generic_function_pointer (*egl_gpa)(const char *name) = NULL;
	
	if (fips_dispatch_api == FIPS_API_GLX) {
		if (glx_gpa == NULL)
			glx_gpa = fips_lookup ("glXGetProcAddressARB");
		return glx_gpa ((const GLubyte *)name);
	} else {
		if (egl_gpa == NULL)
			egl_gpa = fips_lookup ("eglGetProcAddress");
		return egl_gpa (name);
	}
}
