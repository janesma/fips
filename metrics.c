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
#include <assert.h>
#include <sys/time.h>

#include "fips-dispatch-gl.h"

#include "metrics.h"
#include "xmalloc.h"

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

typedef struct metrics_group_info
{
	GLuint id;
	char *name;

	GLuint num_counters;
	GLuint max_active_counters;

	GLuint *counters;
	char **counter_names;
	GLuint *counter_types;

} metrics_group_info_t;

typedef struct metrics_info
{
	int initialized;

	unsigned num_groups;
	metrics_group_info_t *groups;
} metrics_info_t;

typedef struct context
{
	metrics_info_t metrics_info;

	metrics_op_t op;

	timer_query_t *timer_head;
	timer_query_t *timer_tail;

	monitor_t *monitor_head;
	monitor_t *monitor_tail;

	unsigned num_op_metrics;
	op_metrics_t *op_metrics;
} context_t;

/* FIXME: Need a map from integers to context objects and track the
 * current context with glXMakeContextCurrent, eglMakeCurrent, etc. */

context_t current_context;

int frames;
int verbose;

static void
metrics_group_info_init (metrics_group_info_t *group, GLuint id)
{
	GLsizei length;
	unsigned i;

	group->id = id;

	/* Get name */
	glGetPerfMonitorGroupStringAMD (id, 0, &length, NULL);

	group->name = xmalloc (length + 1);

	glGetPerfMonitorGroupStringAMD (id, length + 1, NULL, group->name);

	/* Get number of counters */
	group->num_counters = 0;
	group->max_active_counters = 0;
	glGetPerfMonitorCountersAMD (group->id,
				     (int *) &group->num_counters,
				     (int *) &group->max_active_counters,
				     0, NULL);

	/* Get counter numbers */
	group->counters = xmalloc (group->num_counters * sizeof (GLuint));

	glGetPerfMonitorCountersAMD (group->id, NULL, NULL,
				     group->num_counters,
				     group->counters);

	/* Get counter names */
	group->counter_names = xmalloc (group->num_counters * sizeof (char *));
	group->counter_types = xmalloc (group->num_counters * sizeof (GLuint));

	for (i = 0; i < group->num_counters; i++) {
		glGetPerfMonitorCounterInfoAMD (group->id,
						group->counters[i],
						GL_COUNTER_TYPE_AMD,
						&group->counter_types[i]);

		/* We assume that all peformance counters are made
		 * available as uint32 values. The code calling
		 * CONSUME in accumulate_program_metrics will need to
		 * be extended to accomodate other counter values. */
		if (group->counter_types[i] != GL_UNSIGNED_INT) {
			fprintf (stderr, "fips: Internal error: No support for non-uint counter values\n");
			exit (1);
		}

		glGetPerfMonitorCounterStringAMD (group->id,
						  group->counters[i],
						  0, &length, NULL);

		group->counter_names[i] = xmalloc (length + 1);

		glGetPerfMonitorCounterStringAMD (group->id,
						  group->counters[i],
						  length + 1, NULL,
						  group->counter_names[i]);
	}
}

static void
metrics_group_info_fini (metrics_group_info_t *group)
{
	unsigned i;

	for (i = 0; i < group->num_counters; i++)
		free (group->counter_names[i]);

	free (group->counter_types);
	free (group->counter_names);
	free (group->counters);

	free (group->name);
}

static void
metrics_info_fini (metrics_info_t *info);

void
metrics_info_init (void)
{
	unsigned i;
	GLuint *group_ids;
	metrics_info_t *metrics_info = &current_context.metrics_info;

	if (metrics_info->initialized)
		metrics_info_fini (metrics_info);

	glGetPerfMonitorGroupsAMD ((int *) &metrics_info->num_groups, 0, NULL);

	group_ids = xmalloc (metrics_info->num_groups * sizeof (GLuint));

	glGetPerfMonitorGroupsAMD (NULL, metrics_info->num_groups, group_ids);

	metrics_info->groups = xmalloc (metrics_info->num_groups * sizeof (metrics_group_info_t));

	for (i = 0; i < metrics_info->num_groups; i++)
		metrics_group_info_init (&metrics_info->groups[i], i);

	free (group_ids);

	metrics_info->initialized = 1;
}

static void
metrics_info_fini (metrics_info_t *info)
{
	unsigned i;

	for (i = 0; i < info->num_groups; i++)
		metrics_group_info_fini (&info->groups[i]);

	free (info->groups);
}

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
		fprintf (stderr, "fips: Internal error: "
			 "Unknown metrics op value: %d\n", op);
		exit (1);
	}

	return "";
}

void
metrics_counter_start (void)
{
	context_t *ctx = &current_context;
	timer_query_t *timer;
	monitor_t *monitor;
	unsigned i;

	/* Create new timer query, add to list */
	timer = xmalloc (sizeof (timer_query_t));

	timer->op = ctx->op;
	timer->next = NULL;

	if (ctx->timer_tail) {
		ctx->timer_tail->next = timer;
		ctx->timer_tail = timer;
	} else {
		ctx->timer_tail = timer;
		ctx->timer_head = timer;
	}

	/* Create a new performance-monitor query */
	monitor = xmalloc (sizeof (monitor_t));

	monitor->op = ctx->op;
	monitor->next = NULL;

	if (ctx->monitor_tail) {
		ctx->monitor_tail->next = monitor;
		ctx->monitor_tail = monitor;
	} else {
		ctx->monitor_tail = monitor;
		ctx->monitor_head = monitor;
	}

	/* Initialize the timer_query and monitor objects */
	glGenQueries (1, &timer->id);

	glGenPerfMonitorsAMD (1, &monitor->id);

	for (i = 0; i < ctx->metrics_info.num_groups; i++)
	{
		metrics_group_info_t *group;
		int num_counters;

		group = &ctx->metrics_info.groups[i];

		num_counters = group->num_counters;
		if (group->max_active_counters < group->num_counters)
		{
			fprintf (stderr, "Warning: Only monitoring %d/%d counters from group %d\n",
				 group->max_active_counters,
				 group->num_counters, i);
			num_counters = group->max_active_counters;

		}

		glSelectPerfMonitorCountersAMD(monitor->id,
					       GL_TRUE, group->id,
					       num_counters,
					       group->counters);
	}

	/* Start the queries */
	glBeginQuery (GL_TIME_ELAPSED, timer->id);

	glBeginPerfMonitorAMD (monitor->id);
}

void
metrics_counter_stop (void)
{
	glEndQuery (GL_TIME_ELAPSED);
	glEndPerfMonitorAMD (current_context.monitor_tail->id);
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
op_metrics_init (context_t *ctx, op_metrics_t *metrics, metrics_op_t op)
{
	metrics_info_t *info = &ctx->metrics_info;
	unsigned i, j;

	metrics->op = op;
	metrics->time_ns = 0.0;

	metrics->counters = xmalloc (sizeof(double *) * info->num_groups);

	for (i = 0; i < info->num_groups; i++) {
		metrics->counters[i] = xmalloc (sizeof (double) *
						info->groups[i].num_counters);
		for (j = 0; j < info->groups[i].num_counters; j++)
			metrics->counters[i][j] = 0.0;
	}
}

static op_metrics_t *
ctx_get_op_metrics (context_t *ctx, metrics_op_t op)
{
	unsigned i;

	if (op >= ctx->num_op_metrics)
	{
		ctx->op_metrics = realloc (ctx->op_metrics,
					   (op + 1) * sizeof (op_metrics_t));
		for (i = ctx->num_op_metrics; i < op + 1; i++)
			op_metrics_init (ctx, &ctx->op_metrics[i], i);

		ctx->num_op_metrics = op + 1;
	}

	return &ctx->op_metrics[op];
}

static void
accumulate_program_metrics (metrics_op_t op, GLuint *result, GLuint size)
{
#define CONSUME(var)							\
	if (p + sizeof(var) > ((unsigned char *) result) + size)	\
	{								\
		fprintf (stderr, "Unexpected end-of-buffer while "	\
			 "parsing results\n");				\
		break;							\
	}								\
	(var) = *((typeof(var) *) p);					\
	p += sizeof(var);

	context_t *ctx = &current_context;
	unsigned char *p = (unsigned char *) result;

	while (p < ((unsigned char *) result) + size)
	{
		GLuint group_id, counter_id, counter_index;
		metrics_group_info_t *group;
		uint32_t value;
		unsigned i;

		CONSUME (group_id);
		CONSUME (counter_id);
		CONSUME (value);

		assert (group_id < ctx->metrics_info.num_groups);
		group = &ctx->metrics_info.groups[group_id];

		for (i = 0; i < group->num_counters; i++) {
			if (group->counters[i] == counter_id)
				break;
		}
		counter_index = i;
		assert (counter_index < group->num_counters);

		ctx->op_metrics[op].counters[group_id][counter_index] += value;
	}
}

static void
accumulate_program_time (metrics_op_t op, unsigned time_ns)
{
	op_metrics_t *metrics;

	metrics = ctx_get_op_metrics (&current_context, op);

	metrics->time_ns += time_ns;
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
print_op_metrics (context_t *ctx, op_metrics_t *metric, double total)
{
	metrics_info_t *info = &ctx->metrics_info;
	metrics_group_info_t *group;
	const char *op_string;
	unsigned i, group_id, counter;
	double value;

	/* Since we sparsely fill the array based on program
	 * id, many "programs" have no time.
	 */
	if (metric->time_ns == 0.0)
		return;

	op_string = metrics_op_string (metric->op);

	printf ("%s", op_string);
	if (metric->op >= METRICS_OP_SHADER) {
		printf (" %d:", metric->op - METRICS_OP_SHADER);
	} else {
		printf (":");
		for (i = strlen (op_string); i < 20; i++)
			printf (" ");
	}

	printf ("\t%7.2f ms (% 2.1f%%)",
		metric->time_ns / 1e6,
		metric->time_ns / total * 100);

	printf ("[");
	for (group_id = 0; group_id < info->num_groups; group_id++) {
		group = &info->groups[group_id];
		for (counter = 0; counter < group->num_counters; counter++) {
			value = metric->counters[group_id][counter];
			if (value == 0.0)
				continue;
			printf ("%s: %.2f ", group->counter_names[counter],
				value / 1e6);
		}
	}
	printf ("]\n");
}

static void
print_program_metrics (void)
{
	context_t *ctx = &current_context;
	int *sorted; /* Sorted indices into the ctx->op_metrics */
	double total = 0;
	unsigned i;

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

	for (i = 0; i < ctx->num_op_metrics; i++)
		print_op_metrics (ctx, &ctx->op_metrics[sorted[i]], total);

	free (sorted);
}

/* Called at program exit */
static void
metrics_exit (void)
{
	if (verbose)
		printf ("fips: terminating\n");

	metrics_info_fini (&current_context.metrics_info);
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

	/* Consume all timer queries that are ready. */
	timer_query_t *timer = current_context.timer_head;

	while (timer) {
		GLuint available, elapsed;

		glGetQueryObjectuiv (timer->id,
				     GL_QUERY_RESULT_AVAILABLE, &available);
		if (! available)
			break;

		glGetQueryObjectuiv (timer->id,
				     GL_QUERY_RESULT, &elapsed);

		accumulate_program_time (timer->op, elapsed);

		current_context.timer_head = timer->next;
		if (current_context.timer_head == NULL)
			current_context.timer_tail = NULL;

		glDeleteQueries (1, &timer->id);

		free (timer);
		timer = current_context.timer_head;
	}

	/* And similarly for all performance monitors that are ready. */
	monitor_t *monitor = current_context.monitor_head;

	while (monitor) {
		GLuint available, result_size, *result;
		GLint bytes_written;

		glGetPerfMonitorCounterDataAMD (monitor->id,
						GL_PERFMON_RESULT_AVAILABLE_AMD,
						sizeof (available), &available,
						NULL);
		if (! available)
			break;

		glGetPerfMonitorCounterDataAMD (monitor->id,
						GL_PERFMON_RESULT_SIZE_AMD,
						sizeof (result_size),
						&result_size, NULL);

		result = xmalloc (result_size);

		glGetPerfMonitorCounterDataAMD (monitor->id,
						GL_PERFMON_RESULT_AMD,
						result_size, result,
						&bytes_written);

		accumulate_program_metrics (monitor->op, result, result_size);

		free (result);

		current_context.monitor_head = monitor->next;
		if (current_context.monitor_head == NULL)
			current_context.monitor_tail = NULL;

		glDeletePerfMonitorsAMD (1, &monitor->id);

		free (monitor);
		monitor = current_context.monitor_head;
	}

	if (frames % 60 == 0) {
		double fps;

		fps = (double) frames / (tv_now.tv_sec - tv_start.tv_sec +
					 (tv_now.tv_usec - tv_start.tv_usec) / 1.0e6);

		printf("FPS: %.3f\n", fps);

		print_program_metrics ();
	}
}
