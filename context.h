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

/* Timer query */
typedef struct timer_query
{
	unsigned id;

	metrics_op_t op;
	struct timer_query *next;
} timer_query_t;

/* Performance-monitor query */
typedef struct monitor
{
	unsigned id;

	metrics_op_t op;
	struct monitor *next;
} monitor_t;

typedef struct op_metrics
{
	/* This happens to also be the index into the
	 * ctx->op_metrics array currently
	 */
	metrics_op_t op;
	double time_ns;

	double **counters;
} op_metrics_t;

typedef struct context
{
	/* Pointer to the system's context ID, (such as a GLXContext) */
	void *system_id;

	metrics_info_t metrics_info;

	metrics_op_t op;

	/* GL_TIME_ELAPSED query for which glEndQuery has not yet
	 * been called. */
	unsigned timer_begun_id;

	/* GL_TIME_ELAPSED queries for which glEndQuery has been
	 * called, (but results have not yet been queried). */
	timer_query_t *timer_head;
	timer_query_t *timer_tail;

	/* Performance monitor for which glEndPerfMonitorAMD has not
	 * yet been called. */
	unsigned monitor_begun_id;

	/* Performance monitors for which glEndPerfMonitorAMD has
	 * been called, (but results have not yet been queried). */
	monitor_t *monitor_head;
	monitor_t *monitor_tail;

	int monitors_in_flight;

	unsigned num_op_metrics;
	op_metrics_t *op_metrics;
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
