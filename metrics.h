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

/* Set the ID of the currently executing shader program.
 *
 * The metrics-tracking code will account for per-shader-program
 * timings by accumulating counter values measured while each porogram
 * is active (see metrics_add_counter).
 */
void
metrics_set_current_program (unsigned program);

/* Should be called at the end of every function wrapper for a
 * function that ends a frame, (glXSwapBuffers and similar).
 *
 * This function performs whatever bookkeeping is necessary to
 * generate a timing report, then emits that report.
 */
void
metrics_end_frame (void);

#endif
