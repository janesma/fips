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

#ifndef GLWRAP_H
#define GLWRAP_H

/* Lookup a function named 'name' in the underlying, real, libGL.so */
void *
glwrap_lookup (char *name);

/* Defer to the real 'function' (from libGL.so) to do the real work.
 * The symbol is looked up once and cached in a static variable for
 * future uses.
 */
#define GLWRAP_DEFER(function,...) do {				\
	static typeof(&function) real_ ## function;		\
	if (! real_ ## function)				\
		real_ ## function = glwrap_lookup (#function);	\
	real_ ## function(__VA_ARGS__); 			\
} while (0);

/* As GLWRAP_DEFER, but also set 'ret' to the return value */
#define GLWRAP_DEFER_WITH_RETURN(ret, function,...) do {	\
	static typeof(&function) real_ ## function;		\
	if (! real_ ## function)				\
		real_ ## function = glwrap_lookup (#function);	\
	(ret) = real_ ## function(__VA_ARGS__); 		\
} while (0);

#endif
