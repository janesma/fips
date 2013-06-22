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

#ifndef FIPS_DISPATCH_H
#define FIPS_DISPATCH_H

#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>

extern PFNGLGENQUERIESPROC fips_dispatch_glGenQueries;
#define glGenQueries fips_dispatch_glGenQueries

extern PFNGLDELETEQUERIESPROC fips_dispatch_glDeleteQueries;
#define glDeleteQueries fips_dispatch_glDeleteQueries

extern PFNGLBEGINQUERYPROC fips_dispatch_glBeginQuery;
#define glBeginQuery fips_dispatch_glBeginQuery

extern PFNGLENDQUERYPROC fips_dispatch_glEndQuery;
#define glEndQuery fips_dispatch_glEndQuery

extern PFNGLGETQUERYOBJECTUIVPROC fips_dispatch_glGetQueryObjectuiv;
#define glGetQueryObjectuiv fips_dispatch_glGetQueryObjectuiv

typedef enum {
	FIPS_API_GLX,
	FIPS_API_EGL
} fips_api_t;

void
fips_dispatch_init (fips_api_t api);

#endif /* FIPS_DISPATCH_H */
