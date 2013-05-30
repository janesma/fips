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

#include <stdio.h>
#include <stdlib.h>

#include <sys/time.h>

#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>

#include "metrics.h"

typedef struct counter
{
	unsigned id;
	unsigned program;
	struct counter *next;
} counter_t;

typedef struct program_metrics
{
	unsigned id;
	double ticks;
} program_metrics_t;

typedef struct context
{
	unsigned int program;

	counter_t *counter_head;
	counter_t *counter_tail;

	unsigned num_program_metrics;
	program_metrics_t *program_metrics;
} context_t;

/* FIXME: Need a map from integers to context objects and track the
 * current context with glXMakeContextCurrent, eglMakeCurrent, etc. */

context_t current_context;

unsigned
metrics_add_counter (void)
{
	counter_t *counter;

	counter = malloc (sizeof(counter_t));
	if (counter == NULL) {
		fprintf (stderr, "Out of memory\n");
		exit (1);
	}

	glGenQueries (1, &counter->id);

	counter->program = current_context.program;
	counter->next = NULL;

	if (current_context.counter_tail) {
		current_context.counter_tail->next = counter;
		current_context.counter_tail = counter;
	} else {
		current_context.counter_tail = counter;
		current_context.counter_head = counter;
	}

	return counter->id;
}

void
metrics_set_current_program (unsigned program)
{
	current_context.program = program;
}

static void
accumulate_program_ticks (unsigned program_id, unsigned ticks)
{
	context_t *ctx = &current_context;
	unsigned i;

	if (program_id >= ctx->num_program_metrics) {
		ctx->program_metrics = realloc (ctx->program_metrics,
						(program_id + 1) * sizeof (program_metrics_t));
		for (i = ctx->num_program_metrics; i < program_id + 1; i++) {
			ctx->program_metrics[i].id = i;
			ctx->program_metrics[i].ticks = 0.0;
		}

		ctx->num_program_metrics = program_id + 1;
	}

	ctx->program_metrics[program_id].ticks += ticks;
}

/* FIXME: Should sort the metrics, print out percentages, etc. */
static void
print_program_metrics (void)
{
	context_t *ctx = &current_context;
	unsigned i;

	for (i = 0; i < ctx->num_program_metrics; i++) {
		if (ctx->program_metrics[i].ticks == 0.0)
			continue;
		printf ("Program %d:\t%7.2f mega-ticks\n",
			i, ctx->program_metrics[i].ticks / 1e6);
	}
}

void
metrics_end_frame (void)
{
	static int initialized = 0;
	static int frames;
	static struct timeval tv_start, tv_now;

	if (! initialized) {
		frames = 0;
		gettimeofday (&tv_start, NULL);
		initialized = 1;
	}


	frames++;
	gettimeofday (&tv_now, NULL);

	/* Consume all counters that are ready. */
	counter_t *counter = current_context.counter_head;

	while (counter) {
		GLint available;
		GLuint elapsed;

		glGetQueryObjectiv (counter->id, GL_QUERY_RESULT_AVAILABLE,
				    &available);
		if (! available)
			break;

		glGetQueryObjectuiv (counter->id, GL_QUERY_RESULT, &elapsed);

		accumulate_program_ticks (counter->program, elapsed);

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
