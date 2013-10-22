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

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>

#include <sys/time.h>

#include "fips-dispatch-gl.h"

#include "metrics.h"
#include "xmalloc.h"

typedef struct counter
{
	unsigned id;

	metrics_op_t op;
	struct counter *next;
} counter_t;

typedef struct op_metrics
{
	/* This happens to also be the index into the
	 * ctx->op_metrics array currently
	 */
	metrics_op_t op;
	double time_ns;
} op_metrics_t;

typedef struct context
{
	metrics_op_t op;

	counter_t *counter_head;
	counter_t *counter_tail;

	unsigned num_op_metrics;
	op_metrics_t *op_metrics;
} context_t;

/* FIXME: Need a map from integers to context objects and track the
 * current context with glXMakeContextCurrent, eglMakeCurrent, etc. */

context_t current_context;

int frames;
int verbose;

static const char *
metrics_op_string (metrics_op_t op)
{
	if (op >= METRICS_OP_SHADER)
		return "Shader program";

	switch (op)
	{
	case METRICS_OP_ACCUM:
		return "glAccum*(+)";
	case METRICS_OP_BUFFER_DATA:
		return "glBufferData(+)";
	case METRICS_OP_BUFFER_SUB_DATA:
		return "glCopyBufferSubData*";
	case METRICS_OP_BITMAP:
		return "glBitmap*";
	case METRICS_OP_BLIT_FRAMEBUFFER:
		return "glBlitFramebuffer*";
	case METRICS_OP_CLEAR:
		return "glClear(+)";
	case METRICS_OP_CLEAR_BUFFER_DATA:
		return "glCearBufferData(+)";
	case METRICS_OP_CLEAR_TEX_IMAGE:
		return "glClearTexImage(+)";
	case METRICS_OP_COPY_PIXELS:
		return "glCopyPixels";
	case METRICS_OP_COPY_TEX_IMAGE:
		return "glCopyTexImage(+)";
	case METRICS_OP_DRAW_PIXELS:
		return "glDrawPixels";
	case METRICS_OP_GET_TEX_IMAGE:
		return "glGetTexImage(+)";
	case METRICS_OP_READ_PIXELS:
		return "glReadPixels*";
	case METRICS_OP_TEX_IMAGE:
		return "glTexImage*(+)";
	default:
		fprintf (stderr, "Internal error: "
			 "Unknown metrics op value: %d\n", op);
		exit (1);
	}

	return "";
}

void
metrics_counter_start (void)
{
	counter_t *counter;

	counter = xmalloc (sizeof(counter_t));

	glGenQueries (1, &counter->id);

	counter->op = current_context.op;
	counter->next = NULL;

	if (current_context.counter_tail) {
		current_context.counter_tail->next = counter;
		current_context.counter_tail = counter;
	} else {
		current_context.counter_tail = counter;
		current_context.counter_head = counter;
	}

	glBeginQuery (GL_TIME_ELAPSED, counter->id);
}

void
metrics_counter_stop (void)
{
	glEndQuery (GL_TIME_ELAPSED);
}

void
metrics_set_current_op (metrics_op_t op)
{
	current_context.op = op;
}

metrics_op_t
metrics_get_current_op (void)
{
	return current_context.op;
}

static void
accumulate_program_time (metrics_op_t op, unsigned time_ns)
{
	context_t *ctx = &current_context;
	unsigned i;

	if (op >= ctx->num_op_metrics) {
		ctx->op_metrics = realloc (ctx->op_metrics,
					   (op + 1) * sizeof (op_metrics_t));
		for (i = ctx->num_op_metrics; i < op + 1; i++) {
			ctx->op_metrics[i].op = i;
			ctx->op_metrics[i].time_ns = 0.0;
		}

		ctx->num_op_metrics = op + 1;
	}

	ctx->op_metrics[op].time_ns += time_ns;
}

static int
time_compare(const void *in_a, const void *in_b, void *arg)
{
	int a = *(const int *)in_a;
	int b = *(const int *)in_b;
	struct op_metrics *metrics = arg;

	if (metrics[a].time_ns < metrics[b].time_ns)
		return -1;
	if (metrics[a].time_ns > metrics[b].time_ns)
		return 1;
	return 0;
}

static void
print_program_metrics (void)
{
	context_t *ctx = &current_context;
	unsigned i, j;
	int *sorted; /* Sorted indices into the ctx->op_metrics */
	double total = 0;

	/* Make a sorted list of the operations by time used, and figure
	 * out the total so we can print percentages.
	 */
	sorted = calloc(ctx->num_op_metrics, sizeof(*sorted));
	for (i = 0; i < ctx->num_op_metrics; i++) {
		sorted[i] = i;
		total += ctx->op_metrics[i].time_ns;
	}
	qsort_r(sorted, ctx->num_op_metrics, sizeof(*sorted),
		time_compare, ctx->op_metrics);

	for (i = 0; i < ctx->num_op_metrics; i++) {
		const char *op_string;
		op_metrics_t *metric =&ctx->op_metrics[sorted[i]];

		/* Since we sparsely fill the array based on program
		 * id, many "programs" have no time.
		 */
		if (metric->time_ns == 0.0)
			continue;

		op_string = metrics_op_string (metric->op);

		printf ("%s", op_string);
		if (metric->op >= METRICS_OP_SHADER) {
			printf (" %d:", metric->op - METRICS_OP_SHADER);
		} else {
			printf (":");
			for (j = strlen (op_string); j < 20; j++)
				printf (" ");
		}
		printf ("\t%7.2f ms (% 2.1f%%)\n",
			metric->time_ns / 1e6,
			metric->time_ns / total * 100);
	}
}

/* Called at program exit */
static void
metrics_exit (void)
{
	if (verbose)
		printf ("fips: terminating\n");
}


void
metrics_end_frame (void)
{
	static int initialized = 0;
	static struct timeval tv_start, tv_now;

	if (! initialized) {
		gettimeofday (&tv_start, NULL);
		atexit (metrics_exit);
		if (getenv ("FIPS_VERBOSE"))
			verbose = 1;
		initialized = 1;
	}

	if (verbose)
		printf ("fips: frame %d complete\n", frames);

	frames++;
	gettimeofday (&tv_now, NULL);

	/* Consume all counters that are ready. */
	counter_t *counter = current_context.counter_head;

	while (counter) {
		GLuint available, elapsed;

		glGetQueryObjectuiv (counter->id, GL_QUERY_RESULT_AVAILABLE,
				     &available);
		if (! available)
			break;

		glGetQueryObjectuiv (counter->id, GL_QUERY_RESULT, &elapsed);

		accumulate_program_time (counter->op, elapsed);

		current_context.counter_head = counter->next;
		if (current_context.counter_head == NULL)
			current_context.counter_tail = NULL;

		glDeleteQueries (1, &counter->id);

		free (counter);
		counter = current_context.counter_head;
	}

	if (frames % 60 == 0) {
		double fps;

		fps = (double) frames / (tv_now.tv_sec - tv_start.tv_sec +
					 (tv_now.tv_usec - tv_start.tv_usec) / 1.0e6);

		printf("FPS: %.3f\n", fps);

		print_program_metrics ();
	}
}
