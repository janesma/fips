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
#include "context.h"
#include "metrics-info.h"
#include "xmalloc.h"

int frames;
int verbose;

#define MAX_MONITORS_IN_FLIGHT 1000

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
	 * metrics->op_metrics array currently
	 */
	metrics_op_t op;
	double time_ns;

	double **counters;
} op_metrics_t;

struct metrics
{
	/* Description of all available peformance counters, counter
	 * groups, their names and IDs, etc. */
	metrics_info_t *info;

	/* The current operation being measured. */
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
};

metrics_t *
metrics_create (metrics_info_t *info)
{
	metrics_t *metrics;

	metrics = xmalloc (sizeof (metrics_t));

	metrics->info = info;

	metrics->op = 0;

	metrics->timer_begun_id = 0;

	metrics->timer_head = NULL;
	metrics->timer_tail = NULL;

	metrics->monitor_begun_id = 0;

	metrics->monitor_head = NULL;
	metrics->monitor_tail = NULL;

	metrics->monitors_in_flight = 0;

	metrics->num_op_metrics = 0;
	metrics->op_metrics = NULL;

	return metrics;
}

void
metrics_fini (metrics_t *metrics)
{
	timer_query_t *timer, *timer_next;
	monitor_t *monitor, *monitor_next;

	/* Discard and cleanup any outstanding queries. */
	if (metrics->timer_begun_id) {
		glEndQuery (GL_TIME_ELAPSED);
		glDeleteQueries (1, &metrics->timer_begun_id);
		metrics->timer_begun_id = 0;
	}

	for (timer = metrics->timer_head;
	     timer;
	     timer = timer_next)
	{
		glDeleteQueries (1, &timer->id);
		timer_next = timer->next;
		free (timer);
	}
	metrics->timer_head = NULL;
	metrics->timer_tail = NULL;

	if (metrics->info->have_perfmon) {

		if (metrics->monitor_begun_id) {
			glEndPerfMonitorAMD (metrics->monitor_begun_id);
			glDeletePerfMonitorsAMD (1, &metrics->monitor_begun_id);
			metrics->monitor_begun_id = 0;
		}

		for (monitor = metrics->monitor_head;
		     monitor;
		     monitor = monitor_next)
		{
			glDeletePerfMonitorsAMD (1, &monitor->id);
			monitor_next = monitor->next;
			free (monitor);
		}
		metrics->monitor_head = NULL;
		metrics->monitor_tail = NULL;

	}

	metrics->monitors_in_flight = 0;
}

void
metrics_destroy (metrics_t *metrics)
{
	metrics_fini (metrics);

	free (metrics);
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
metrics_counter_start (metrics_t *metrics)
{
	unsigned i;

	/* Initialize the timer_query object. */
	glGenQueries (1, &metrics->timer_begun_id);

	/* Most everything else in this function is
	 * performance-monitor related. If we don't have that
	 * extension, just start the timer query and be done. */
	if (! metrics->info->have_perfmon) {
		glBeginQuery (GL_TIME_ELAPSED, metrics->timer_begun_id);
		return;
	}

	/* Initialize the performance-monitor object */
	glGenPerfMonitorsAMD (1, &metrics->monitor_begun_id);

	for (i = 0; i < metrics->info->num_groups; i++)
	{
		metrics_group_info_t *group;
		int num_counters;

		group = &metrics->info->groups[i];

		num_counters = group->num_counters;
		if (group->max_active_counters < group->num_counters)
		{
			fprintf (stderr, "Warning: Only monitoring %d/%d counters from group %d\n",
				 group->max_active_counters,
				 group->num_counters, i);
			num_counters = group->max_active_counters;

		}

		glSelectPerfMonitorCountersAMD(metrics->monitor_begun_id,
					       GL_TRUE, group->id,
					       num_counters,
					       group->counter_ids);
	}

	/* Start the queries */
	glBeginQuery (GL_TIME_ELAPSED, metrics->timer_begun_id);

	glBeginPerfMonitorAMD (metrics->monitor_begun_id);
}

void
metrics_counter_stop (metrics_t *metrics)
{
	timer_query_t *timer;
	monitor_t *monitor;

	/* Stop the current timer and monitor. */
	glEndQuery (GL_TIME_ELAPSED);

	if (metrics->info->have_perfmon)
		glEndPerfMonitorAMD (metrics->monitor_begun_id);

	/* Add these IDs to our lists of outstanding queries and
	 * monitors so the results can be collected later. */
	timer = xmalloc (sizeof (timer_query_t));

	timer->op = metrics->op;
	timer->id = metrics->timer_begun_id;
	timer->next = NULL;

	if (metrics->timer_tail) {
		metrics->timer_tail->next = timer;
		metrics->timer_tail = timer;
	} else {
		metrics->timer_tail = timer;
		metrics->timer_head = timer;
	}

	if (metrics->info->have_perfmon) {
		/* Create a new performance-monitor query */
		monitor = xmalloc (sizeof (monitor_t));

		monitor->op = metrics->op;
		monitor->id = metrics->monitor_begun_id;
		monitor->next = NULL;

		if (metrics->monitor_tail) {
			metrics->monitor_tail->next = monitor;
			metrics->monitor_tail = monitor;
		} else {
			metrics->monitor_tail = monitor;
			metrics->monitor_head = monitor;
		}
	}

	metrics->monitors_in_flight++;

	/* Avoid being a resource hog and collect outstanding results
	 * once we have sent off a large number of
	 * queries. (Presumably, many of the outstanding queries are
	 * available by now.)
	 */
	if (metrics->monitors_in_flight > MAX_MONITORS_IN_FLIGHT)
		metrics_collect_available (metrics);
}

void
metrics_set_current_op (metrics_t *metrics, metrics_op_t op)
{
	metrics->op = op;
}

metrics_op_t
metrics_get_current_op (metrics_t *metrics)
{
	return metrics->op;
}

static void
op_metrics_init (metrics_info_t *info, op_metrics_t *metrics, metrics_op_t op)
{
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
_get_op_metrics (metrics_t *metrics, metrics_op_t op)
{
	unsigned i;

	if (op >= metrics->num_op_metrics)
	{
		metrics->op_metrics = realloc (metrics->op_metrics,
					       (op + 1) * sizeof (op_metrics_t));
		for (i = metrics->num_op_metrics; i < op + 1; i++)
			op_metrics_init (metrics->info, &metrics->op_metrics[i], i);

		metrics->num_op_metrics = op + 1;
	}

	return &metrics->op_metrics[op];
}

static void
accumulate_program_metrics (metrics_t *metrics, metrics_op_t op,
			    GLuint *result, GLuint size)
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

	metrics_info_t *info = metrics->info;
	op_metrics_t *op_metrics = _get_op_metrics (metrics, op);
	unsigned char *p = (unsigned char *) result;

	while (p < ((unsigned char *) result) + size)
	{
		GLuint group_id, group_index;
		GLuint counter_id, counter_index;
		metrics_group_info_t *group;
		double value = 0.0;
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

		op_metrics->counters[group_index][counter_index] += value;
	}
}

static void
accumulate_program_time (metrics_t *metrics, metrics_op_t op, unsigned time_ns)
{
	op_metrics_t *op_metrics;

	op_metrics = _get_op_metrics (metrics, op);

	op_metrics->time_ns += time_ns;
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
print_per_stage_metrics (metrics_t *metrics,
			 per_stage_metrics_t *per_stage,
			 double total)
{
	metrics_info_t *info = metrics->info;
	op_metrics_t *op_metrics = per_stage->metrics;
	metrics_group_info_t *group;
	const char *op_string;
	unsigned group_index, counter;
	double value;

	/* Don't print anything for stages with no alloted time. */
	if (per_stage->time_ns == 0.0)
		return;

	op_string = metrics_op_string (op_metrics->op);

	printf ("%21s", op_string);

	if (op_metrics->op >= METRICS_OP_SHADER) {
		printf (" %3d", op_metrics->op - METRICS_OP_SHADER);
	} else {
		printf ("    ");

	}

	if (per_stage->stage)
		printf (" %cS:", per_stage->stage->name[0]);
	else
		printf ("   :");

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

			value = op_metrics->counters[group_index][counter];
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
print_program_metrics (metrics_t *metrics)
{
	metrics_info_t *info = metrics->info;
	unsigned num_shader_stages = info->num_shader_stages;
	per_stage_metrics_t *sorted, *per_stage;
	double total_time, op_cycles;
	op_metrics_t *op;
	unsigned group_index, counter_index;
	unsigned i, j, num_sorted;

	/* Make a sorted list of the per-stage operations by time
	 * used, and figure out the total so we can print percentages.
	 */
	if (num_shader_stages)
		num_sorted = metrics->num_op_metrics * num_shader_stages;
	else
		num_sorted = metrics->num_op_metrics;

	sorted = xmalloc (sizeof (*sorted) * num_sorted);

	total_time = 0.0;

	for (i = 0; i < metrics->num_op_metrics; i++) {

		op = &metrics->op_metrics[i];

		/* Accumulate total time across all ops. */
		total_time += op->time_ns;

		/* Also, find total cycles in all stages of this op. */
		op_cycles = 0.0;

		if (num_shader_stages == 0) {
			per_stage = &sorted[i];
			per_stage->metrics = op;
			per_stage->stage = NULL;
			per_stage->time_ns = op->time_ns;
			per_stage->active = 0.0;
		}

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

			if (op_cycles) {
				per_stage->stage = &info->stages[j];
				per_stage->time_ns = op->time_ns * (stage_cycles / op_cycles);
			} else {
				/* If we don't have any per-stage cycle counts
				 * for this operation, then use the first
				 * stage as a placeholder for all the time,
				 * but NULL-ify the stage info so that the
				 * report doesn't lie about this time being
				 * from any particular stage. */
				per_stage->stage = NULL;
				if (j == 0) {
					per_stage->time_ns = op->time_ns;
				} else {
					per_stage->time_ns = 0.0;
				}
			}

			if (stage_cycles) {
				per_stage->active = active_cycles / stage_cycles;
			} else {
				per_stage->active = 0.0;
			}
		}
	}

	qsort_r (sorted, num_sorted, sizeof (*sorted),
		 time_compare, metrics->op_metrics);

	for (i = 0; i < num_sorted; i++)
		print_per_stage_metrics (metrics, &sorted[i], total_time);

	free (sorted);
}

void
metrics_collect_available (metrics_t *metrics)
{
	/* Consume all timer queries that are ready. */
	timer_query_t *timer = metrics->timer_head;

	while (timer) {
		GLuint available, elapsed;

		glGetQueryObjectuiv (timer->id,
				     GL_QUERY_RESULT_AVAILABLE, &available);
		if (! available)
			break;

		glGetQueryObjectuiv (timer->id,
				     GL_QUERY_RESULT, &elapsed);

		accumulate_program_time (metrics, timer->op, elapsed);

		metrics->timer_head = timer->next;
		if (metrics->timer_head == NULL)
			metrics->timer_tail = NULL;

		glDeleteQueries (1, &timer->id);

		free (timer);
		timer = metrics->timer_head;
	}

	if (! metrics->info->have_perfmon)
		return;

	/* And similarly for all performance monitors that are ready. */
	monitor_t *monitor = metrics->monitor_head;

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

		accumulate_program_metrics (metrics, monitor->op, result, result_size);

		free (result);

		metrics->monitor_head = monitor->next;
		if (metrics->monitor_head == NULL)
			metrics->monitor_tail = NULL;

		glDeletePerfMonitorsAMD (1, &monitor->id);

		free (monitor);

		metrics->monitors_in_flight--;

		monitor = metrics->monitor_head;
	}
}

static void
metrics_exit (void)
{
	if (verbose)
		printf ("fips: terminating\n");
}

void
metrics_end_frame (metrics_t *metrics)
{
	return;
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

	metrics_collect_available (metrics);

	if (frames % 15 == 0) {
		double fps;

		gettimeofday (&tv_now, NULL);

		fps = (double) frames / (tv_now.tv_sec - tv_start.tv_sec +
					 (tv_now.tv_usec - tv_start.tv_usec) / 1.0e6);

		printf("FPS: %.3f\n", fps);

		print_program_metrics (metrics);
	}
}
