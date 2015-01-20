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
 * Context-specific resoures (eg Queries) must be cleaned up before
 * the context changes.  No action should be taken if the context is
 * unchanged.
 * 
 * The context_enter function should be called
 * before any subsequent OpenGL calls are made (other than
 * glXMakeCurrent or similar).
 */
void
context_leave (void *next_system_context_id);

/* Start accumulating GPU time.
 *
 * The time accumulated will be accounted against the
 * current operation (as set with context_set_current_op).
 */
void
context_counter_start (void);

/* Stop accumulating GPU time (stops the most-recently started counter) */
void
context_counter_stop (void);

/* Set a metrics_op_t value to indicate what kind of operation is
 * being performed.
 *
 * The metrics-tracking code will account for timings by accumulating
 * measured counter values into a separate counter for each
 * metrics_op_t value, (so that the report can describe which
 * operations are the most expensive).
 *
 * In addition, for the value METRICS_OP_SHADER, each specific shader
 * program can be distinguished. To accomplish this, pass a value of
 * METRICS_OP_SHADER + shader_program_number to this function.
 */
void
context_set_current_op (metrics_op_t op);

/* Return the current metrics_op_t value, (the value most-recently-set
 * with a call to context_set_current_op).
 */
metrics_op_t
context_get_current_op (void);

/* Should be called at the end of every function wrapper for a
 * function that ends a frame, (glXSwapBuffers and similar).
 *
 * This function performs whatever bookkeeping is necessary to
 * generate a timing report, then emits that report.
 */
void
context_end_frame (void);

#endif
