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

#ifndef METRICS_H
#define METRICS_H

typedef enum
{
	METRICS_OP_ACCUM,
	METRICS_OP_BUFFER_DATA,
	METRICS_OP_BUFFER_SUB_DATA,
	METRICS_OP_BITMAP,
	METRICS_OP_BLIT_FRAMEBUFFER,
	METRICS_OP_CLEAR,
	METRICS_OP_CLEAR_BUFFER_DATA,
	METRICS_OP_CLEAR_TEX_IMAGE,
	METRICS_OP_COPY_PIXELS,
	METRICS_OP_COPY_TEX_IMAGE,
	METRICS_OP_DRAW_PIXELS,
	METRICS_OP_GET_TEX_IMAGE,
	METRICS_OP_READ_PIXELS,
	METRICS_OP_TEX_IMAGE,

	/* METRICS_OP_SHADER must be last.
	 * 
	 * All larger values for metrics_op_t are interpreted as:
	 *
	 *	METRICS_OP_SHADER + shader_program_number
	 *
	 * to indicate a specific shader program.
	 */
	METRICS_OP_SHADER
} metrics_op_t;

/* Start accumulating GPU time.
 *
 * The time accumulated will be accounted against the
 * current program (as set with metrics_set_current_program).
 */
void
metrics_counter_start (void);

/* Stop accumulating GPU time (stops the most-recently started counter) */
void
metrics_counter_stop (void);

/* Set a metrics_op_t value to indicate what kind of operation is
 * being performed.
 *
 * The metrics-tracking code will account for timings by accumulating
 * measured counter values into a separate counter for each
 * metrics_op_t value, (so that the report can describe which
 * operations are the most expensive).
 *
 * In addition, for the value METRICS_OP_SHADER, each specific shader
 * program can be distinguished. To accomplish this, pass a value of
 * METRICS_OP_SHADER + shader_program_number to this function.
 */
void
metrics_set_current_op (metrics_op_t op);

/* Return the current metrics_op_t value, (the value most-recently-set
 * with a call to metrics_set_current_op).
 */
metrics_op_t
metrics_get_current_op (void);

/* Should be called at the end of every function wrapper for a
 * function that ends a frame, (glXSwapBuffers and similar).
 *
 * This function performs whatever bookkeeping is necessary to
 * generate a timing report, then emits that report.
 */
void
metrics_end_frame (void);

#endif
