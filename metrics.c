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

	GLuint *counter_ids;
	char **counter_names;
	GLuint *counter_types;

} metrics_group_info_t;

typedef struct shader_stage_info
{
	char *name;

	GLuint active_group_index;
	GLuint active_counter_index;

	GLuint stall_group_index;
	GLuint stall_counter_index;

} shader_stage_info_t;

typedef struct metrics_info
{
	int initialized;

	unsigned num_groups;
	metrics_group_info_t *groups;

	unsigned num_shader_stages;
	shader_stage_info_t *stages;

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
	group->counter_ids = xmalloc (group->num_counters * sizeof (GLuint));

	glGetPerfMonitorCountersAMD (group->id, NULL, NULL,
				     group->num_counters,
				     group->counter_ids);

	/* Get counter names */
	group->counter_names = xmalloc (group->num_counters * sizeof (char *));
	group->counter_types = xmalloc (group->num_counters * sizeof (GLuint));

	for (i = 0; i < group->num_counters; i++) {
		glGetPerfMonitorCounterInfoAMD (group->id,
						group->counter_ids[i],
						GL_COUNTER_TYPE_AMD,
						&group->counter_types[i]);

		glGetPerfMonitorCounterStringAMD (group->id,
						  group->counter_ids[i],
						  0, &length, NULL);

		group->counter_names[i] = xmalloc (length + 1);

		glGetPerfMonitorCounterStringAMD (group->id,
						  group->counter_ids[i],
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
	free (group->counter_ids);

	free (group->name);
}

/* A helper function, part of metrics_info_init below. */

typedef enum {
	SHADER_ACTIVE,
	SHADER_STALL
} shader_phase_t;

static void
_add_shader_stage (metrics_info_t *info, const char *name,
		   GLuint group_index, GLuint counter_index,
		   shader_phase_t phase)
{
	shader_stage_info_t *stage;
	char *stage_name, *space;
	unsigned i;

	stage_name = xstrdup (name);

	/* Terminate the stage name at the first space.
	 *
	 * This is valid for counter names such as:
	 *
	 *	"Vertex Shader Active Time"
	 * or
	 *	"Vertex Shader Stall Time - Core Stall"
	 */
	space = strchr (stage_name, ' ');
	if (space)
		*space = '\0';

	/* Look for an existing stage of the given name. */
	stage = NULL;

	for (i = 0; i < info->num_shader_stages; i++) {
		if (strcmp (info->stages[i].name, stage_name) == 0) {
			stage = &info->stages[i];
			break;
		}
	}

	if (stage == NULL) {
		info->num_shader_stages++;
		info->stages = xrealloc (info->stages,
					 info->num_shader_stages *
					 sizeof (shader_stage_info_t));
		stage = &info->stages[info->num_shader_stages - 1];
		stage->name = xstrdup (stage_name);
		stage->active_group_index = 0;
		stage->active_counter_index = 0;
		stage->stall_group_index = 0;
		stage->stall_counter_index = 0;
	}

	if (phase == SHADER_ACTIVE) {
		stage->active_group_index = group_index;
		stage->active_counter_index = counter_index;
	} else {
		stage->stall_group_index = group_index;
		stage->stall_counter_index = counter_index;
	}

	free (stage_name);
}

void
metrics_info_init (void)
{
	unsigned i, j;
	GLuint *group_ids;
	metrics_info_t *info = &current_context.metrics_info;

	glGetPerfMonitorGroupsAMD ((int *) &info->num_groups, 0, NULL);

	group_ids = xmalloc (info->num_groups * sizeof (GLuint));

	glGetPerfMonitorGroupsAMD (NULL, info->num_groups, group_ids);

	info->groups = xmalloc (info->num_groups * sizeof (metrics_group_info_t));

	for (i = 0; i < info->num_groups; i++)
		metrics_group_info_init (&info->groups[i], group_ids[i]);

	free (group_ids);

	/* Identify each shader stage (by looking at
	 * performance-counter names for specific patterns) and
	 * initialize structures referring to the corresponding
	 * counter numbers for each stage. */
	info->num_shader_stages = 0;
	info->stages = NULL;

	for (i = 0; i < info->num_groups; i++) {
		metrics_group_info_t *group = &info->groups[i];
		for (j = 0; j < group->num_counters; j++) {
			char *name = group->counter_names[j];
			if (strstr (name, "Shader Active Time")) {
				_add_shader_stage (info, name, i, j,
						   SHADER_ACTIVE);
			}
			if (strstr (name, "Shader Stall Time")) {
				_add_shader_stage (info, name, i, j,
						   SHADER_STALL);
			}
		}
	}

	info->initialized = 1;
}

void
metrics_info_fini (void)
{
	metrics_info_t *info = &current_context.metrics_info;
	unsigned i;
	timer_query_t *timer, *timer_next;
	monitor_t *monitor, *monitor_next;

	if (! info->initialized)
		return;

	for (timer = current_context.timer_head;
	     timer;
	     timer = timer_next)
	{
		timer_next = timer->next;
		free (timer);
	}
	current_context.timer_head = NULL;
	current_context.timer_tail = NULL;

	for (monitor = current_context.monitor_head;
	     monitor;
	     monitor = monitor_next)
	{
		monitor_next = monitor->next;
		free (monitor);
	}
	current_context.monitor_head = NULL;
	current_context.monitor_tail = NULL;

	for (i = 0; i < info->num_groups; i++)
		metrics_group_info_fini (&info->groups[i]);

	free (info->groups);
	info->groups = NULL;

	for (i = 0; i < info->num_shader_stages; i++)
		free (info->stages[i].name);

	free (info->stages);
	info->stages = NULL;

	info->initialized = 0;
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
					       group->counter_ids);
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
	metrics_info_t *info = &ctx->metrics_info;
	unsigned char *p = (unsigned char *) result;

	while (p < ((unsigned char *) result) + size)
	{
		GLuint group_id, group_index;
		GLuint counter_id, counter_index;
		metrics_group_info_t *group;
		double value;
		unsigned i;

		CONSUME (group_id);
		CONSUME (counter_id);

		for (i = 0; i < info->num_groups; i++) {
			if (info->groups[i].id == group_id)
				break;
		}
		group_index = i;
		assert (group_index < info->num_groups);
		group = &info->groups[group_index];

		for (i = 0; i < group->num_counters; i++) {
			if (group->counter_ids[i] == counter_id)
				break;
		}
		counter_index = i;
		assert (counter_index < group->num_counters);

		switch (group->counter_types[counter_index])
		{
			uint uint_value;
			uint64_t uint64_value;
			float float_value;
		case GL_UNSIGNED_INT:
			CONSUME (uint_value);
			value = uint_value;
			break;
		case GL_UNSIGNED_INT64_AMD:
			CONSUME (uint64_value);
			value = uint64_value;
			break;
		case GL_PERCENTAGE_AMD:
		case GL_FLOAT:
			CONSUME (float_value);
			value = float_value;
			break;
		default:
			fprintf (stderr, "fips: Warning: Unknown counter value type (%d)\n",
				 group->counter_types[counter_index]);
			value = 0.0;
			break;
		}

		ctx->op_metrics[op].counters[group_index][counter_index] += value;
	}
}

static void
accumulate_program_time (metrics_op_t op, unsigned time_ns)
{
	op_metrics_t *metrics;

	metrics = ctx_get_op_metrics (&current_context, op);

	metrics->time_ns += time_ns;
}

typedef struct per_stage_metrics
{
	op_metrics_t *metrics;
	shader_stage_info_t *stage;
	double time_ns;
	double active;
} per_stage_metrics_t;

static int
_is_shader_stage_counter (metrics_info_t *info,
			  unsigned group_index,
			  unsigned counter_index)
{
	shader_stage_info_t *stage;
	unsigned i;

	for (i = 0; i < info->num_shader_stages; i++) {
		stage = &info->stages[i];

		if (stage->active_group_index == group_index &&
		    stage->active_counter_index == counter_index)
		{
			return 1;
		}

		if (stage->stall_group_index == group_index &&
		    stage->stall_counter_index == counter_index)
		{
			return 1;
		}
	}

	return 0;
}

static void
print_per_stage_metrics (context_t *ctx,
			 per_stage_metrics_t *per_stage,
			 double total)
{
	metrics_info_t *info = &ctx->metrics_info;
	op_metrics_t *metric = per_stage->metrics;
	metrics_group_info_t *group;
	const char *op_string;
	unsigned group_index, counter;
	double value;

	/* Don't print anything for stages with no alloted time. */
	if (per_stage->time_ns == 0.0)
		return;

	op_string = metrics_op_string (metric->op);

	printf ("%21s", op_string);

	if (metric->op >= METRICS_OP_SHADER) {
		printf (" %3d", metric->op - METRICS_OP_SHADER);
	} else {
		printf ("    ");

	}
	printf (" %cS:", per_stage->stage->name[0]);

	printf ("\t%7.2f ms (%4.1f%%)",
		per_stage->time_ns / 1e6,
		per_stage->time_ns / total * 100);

	if (per_stage->active)
		printf (", %4.1f%% active", per_stage->active * 100);

	printf ("\n");

	/* I'm not seeing a lot of value printing the rest of these
	 * performance counters by default yet. Use --verbose to get
	 * them for now. */
	if (! verbose)
		return;

	printf ("[");
	for (group_index = 0; group_index < info->num_groups; group_index++) {
		group = &info->groups[group_index];
		for (counter = 0; counter < group->num_counters; counter++) {

			/* Don't print this counter value if it's a
			 * per-stage cycle counter, (which we have
			 * already accounted for). */
			if (_is_shader_stage_counter (info, group_index, counter))
				continue;

			value = metric->counters[group_index][counter];
			if (value == 0.0)
				continue;
			printf ("%s: %.2f ", group->counter_names[counter],
				value / 1e6);
		}
	}
	printf ("]\n");
}

static int
time_compare(const void *in_a, const void *in_b, void *arg unused)
{
	const per_stage_metrics_t *a = in_a;
	const per_stage_metrics_t *b = in_b;


	if (a->time_ns < b->time_ns)
		return -1;
	if (a->time_ns > b->time_ns)
		return 1;
	return 0;
}

static void
print_program_metrics (void)
{
	context_t *ctx = &current_context;
	metrics_info_t *info = &ctx->metrics_info;
	unsigned num_shader_stages = info->num_shader_stages;
	per_stage_metrics_t *sorted, *per_stage;
	double total_time, op_cycles;
	op_metrics_t *op;
	unsigned group_index, counter_index;
	unsigned i, j, num_sorted;

	/* Make a sorted list of the per-stage operations by time
	 * used, and figure out the total so we can print percentages.
	 */
	num_sorted = ctx->num_op_metrics * num_shader_stages;

	sorted = xmalloc (sizeof (*sorted) * num_sorted);

	total_time = 0.0;

	for (i = 0; i < ctx->num_op_metrics; i++) {

		op = &ctx->op_metrics[i];

		/* Accumulate total time across all ops. */
		total_time += op->time_ns;

		/* Also, find total cycles in all stages of this op. */
		op_cycles = 0.0;

		for (j = 0; j < num_shader_stages; j++) {
			/* Active cycles */
			group_index = info->stages[j].active_group_index;
			counter_index = info->stages[j].active_counter_index;
			op_cycles += op->counters[group_index][counter_index];

			/* Stall cycles */
			group_index = info->stages[j].stall_group_index;
			counter_index = info->stages[j].stall_counter_index;
			op_cycles += op->counters[group_index][counter_index];
		}

		for (j = 0; j < num_shader_stages; j++) {
			double active_cycles, stall_cycles, stage_cycles;

			/* Active cycles */
			group_index = info->stages[j].active_group_index;
			counter_index = info->stages[j].active_counter_index;
			active_cycles = op->counters[group_index][counter_index];

			/* Stall cycles */
			group_index = info->stages[j].stall_group_index;
			counter_index = info->stages[j].stall_counter_index;
			stall_cycles = op->counters[group_index][counter_index];

			stage_cycles = active_cycles + stall_cycles;

			per_stage = &sorted[i * num_shader_stages + j];
			per_stage->metrics = op;
			per_stage->stage = &info->stages[j];
			if (op_cycles)
				per_stage->time_ns = op->time_ns * (stage_cycles / op_cycles);
			else
				per_stage->time_ns = 0.0;
			if (stage_cycles)
				per_stage->active = active_cycles / stage_cycles;
			else
				per_stage->active = 0.0;
		}
	}

	qsort_r (sorted, num_sorted, sizeof (*sorted),
		 time_compare, ctx->op_metrics);

	for (i = 0; i < num_sorted; i++)
		print_per_stage_metrics (ctx, &sorted[i], total_time);

	free (sorted);
}

/* Called at program exit */
static void
metrics_exit (void)
{
	if (verbose)
		printf ("fips: terminating\n");

	metrics_info_fini ();
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
