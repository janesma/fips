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

#include "metrics-info.h"

#include "xmalloc.h"

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
metrics_info_init (metrics_info_t *info)
{
	unsigned i, j;
	GLuint *group_ids;

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
metrics_info_fini (metrics_info_t *info)
{
	unsigned i;
	if (! info->initialized)
		return;

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
