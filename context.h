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

#ifndef CONTEXT_H
#define CONTEXT_H

#include "metrics.h"
#include "metrics-info.h"

#include "fips-dispatch.h"

typedef struct context
{
	/* Pointer to the system's context ID, (such as a GLXContext) */
	void *system_id;

	metrics_info_t metrics_info;
	metrics_t *metrics;
} context_t;

/* Indicate that a new context has come into use.
 *
 * Here, 'system_context_id' is a pointer to a system context (such as
 * a GLXContext) which fips can use to map to persistent contex_t
 * objects if it cares to.
 */
void
context_enter (fips_api_t api, void *system_context_id);

/* Indicate that the application is done using the current context for now.
 *
 * The context_enter function should be called before any subsequent
 * OpenGL calls are made (other than glXMakeCurrent or similar).
 */
void
context_leave (void);

/* Get the current context. */
context_t *
context_get_current (void);

#endif
