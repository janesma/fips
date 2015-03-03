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
#include "publish.h"

typedef struct context
{
	/* Pointer to the system's context ID, (such as a GLXContext) */
	void *system_id;

	/* Does this context have the AMD_performance_monitor extension? */
	bool have_perfmon;

	metrics_info_t metrics_info;
	metrics_t *metrics;
} context_t;

static context_t *current_context = NULL;

/* static bool */
/* check_extension (const char *extension); */

static context_t *
context_create (fips_api_t api, void *system_context_id)
{
	context_t *ctx;

	ctx = xcalloc (1, sizeof (*ctx));

	ctx->system_id = system_context_id;

	fips_dispatch_init (api);

	ctx->have_perfmon = false;

	metrics_info_init (&ctx->metrics_info, ctx->have_perfmon);
	ctx->metrics = metrics_create (&ctx->metrics_info);

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
	publish_context(system_context_id);

	if (current_context && current_context->system_id == system_context_id)
		return;

	if (current_context)
		context_destroy (current_context);

	current_context = context_create (api, system_context_id);

	metrics_set_current_op (current_context->metrics,
				METRICS_OP_SHADER + 0);
	//metrics_counter_start (current_context->metrics);
}

void
context_leave (void *next_system_context_id)
{
	context_t *ctx = current_context;

	if (ctx == NULL)
		return;

	if (next_system_context_id == ctx->system_id)
		// glXMakeCurrent called twice with the same context
		return;

	// clean up metrics resources allocated to the current
	// context, before it changes.
	metrics_destroy (ctx->metrics);
}

void
context_counter_start (void)
{
	//metrics_counter_start (current_context->metrics);
}

void
context_counter_stop (void)
{
	//metrics_counter_stop (current_context->metrics);
}

void
context_set_current_op (metrics_op_t op)
{
	metrics_set_current_op (current_context->metrics, op);
}

metrics_op_t
context_get_current_op (void)
{
	return metrics_get_current_op (current_context->metrics);
}

void
context_end_frame (void)
{
	return metrics_end_frame (current_context->metrics);
}

/* Is the given extension available? */
/* static bool */
/* check_extension (const char *extension) */
/* { */
/* 	int i, num_extensions; */
/* 	const char *available; */

/* 	glGetIntegerv (GL_NUM_EXTENSIONS, &num_extensions); */

/* 	for (i = 0; i < num_extensions; i++) { */
/* 		available = (char *) glGetStringi (GL_EXTENSIONS, i); */
/* 		if (strcmp (extension, available) == 0) { */
/* 			return true; */
/* 		} */
/* 	} */

/* 	return false; */
/* } */
