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

/* This header file provides prototypes to iniatialize the
 * fips-dispatch system (fips_dispatch_init) but does not provide the
 * prototypes for dynamically-dipatched OpenGL functions.
 * See fips-dispatch-gl.h for those.
 */

#ifndef FIPS_DISPATCH_H
#define FIPS_DISPATCH_H

#include "fips.h"

typedef enum {
	FIPS_API_GLX,
	FIPS_API_EGL
} fips_api_t;

extern bool fips_dispatch_initialized;
extern fips_api_t fips_dispatch_api;

/* Initialize fips-dispatch with the appropriate API, (whether GLX or
 * EGL, etc.). */
void
fips_dispatch_init (fips_api_t api);

/* Lookup a named symbol, (via either glXGetProcAddressARB or
 * eglGetProcAddress).
 *
 * Note: No user of fips-dispatch need ever call this
 * function. Instead, users should simply include fips-dispatch-gl.h,
 * then make direct calls to GL functions and through some
 * C-preprocessor magic, the lookup will magically happen at
 * runtime.
 */
void *
fips_dispatch_lookup (const char *name);

#endif /* FIPS_DISPATCH_H */
