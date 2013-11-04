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

#include "context.h"
#include "metrics.h"
#include "xmalloc.h"

context_t *current_context;

static context_t *
context_create (fips_api_t api, void *system_context_id)
{
	context_t *ctx;

	ctx = xcalloc (1, sizeof (*ctx));

	ctx->system_id = system_context_id;

	fips_dispatch_init (api);

	metrics_info_init (&ctx->metrics_info);

	return ctx;
}

static void
context_destroy (context_t *ctx)
{
	metrics_info_fini (&ctx->metrics_info);
}

void
context_enter (fips_api_t api, void *system_context_id)
{
	/* Do nothing if the application is setting the same context
	 * as is already current. */
	if (current_context && current_context->system_id == system_context_id)
		return;

	if (current_context)
		context_destroy (current_context);

	current_context = context_create (api, system_context_id);

	metrics_set_current_op (METRICS_OP_SHADER + 0);
	metrics_counter_start ();
}

void
context_leave (void)
{
	context_t *ctx = current_context;
	timer_query_t *timer, *timer_next;
	monitor_t *monitor, *monitor_next;

	if (ctx == NULL)
		return;

	metrics_collect_available ();

	if (ctx->timer_begun_id) {
		glEndQuery (GL_TIME_ELAPSED);
		glDeleteQueries (1, &ctx->timer_begun_id);
		ctx->timer_begun_id = 0;
	}

	for (timer = ctx->timer_head;
	     timer;
	     timer = timer_next)
	{
		glDeleteQueries (1, &timer->id);
		timer_next = timer->next;
		free (timer);
	}
	ctx->timer_head = NULL;
	ctx->timer_tail = NULL;

	if (ctx->monitor_begun_id) {
		glEndPerfMonitorAMD (ctx->monitor_begun_id);
		glDeletePerfMonitorsAMD (1, &ctx->monitor_begun_id);
		ctx->monitor_begun_id = 0;
	}

	for (monitor = ctx->monitor_head;
	     monitor;
	     monitor = monitor_next)
	{
		glDeletePerfMonitorsAMD (1, &monitor->id);
		monitor_next = monitor->next;
		free (monitor);
	}
	ctx->monitor_head = NULL;
	ctx->monitor_tail = NULL;

	ctx->monitors_in_flight = 0;
}

context_t *
context_get_current (void)
{
	return current_context;
}
