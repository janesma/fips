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

#ifndef METRICS_INFO_H
#define METRICS_INFO_H

#include "fips-dispatch-gl.h"

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

	bool have_perfmon;

	unsigned num_groups;
	metrics_group_info_t *groups;

	unsigned num_shader_stages;
	shader_stage_info_t *stages;

} metrics_info_t;

/* Initialize metrics info
 *
 * This queries the names and ranges for all available performance counters.
 *
 * This should be called before any other metrics functions.
 *
 * The Boolean have_perfmon must be set to correctly indicate whether
 * the current OpenGL context has the AMD_performance_monitor
 * extension.
 */
void
metrics_info_init (metrics_info_t *info, bool have_perfmon);

/* Finalize metrics info state.
 *
 * The function should be called just before setting a new, current,
 * OpenGL context.
 */
void
metrics_info_fini (metrics_info_t *info);

#endif
