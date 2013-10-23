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
#include <string.h>

#include "xmalloc.h"

void *
xmalloc (size_t size)
{
	void *ret;

	ret = malloc (size);
	if (size != 0 && ret == NULL) {
		fprintf (stderr, "Out of memory\n");
		exit (1);
	}

	return ret;
}

void *
xrealloc (void *ptr, size_t size)
{
	void *ret;

	ret = realloc (ptr, size);
	if (size != 0 && ret == NULL) {
		fprintf (stderr, "Out of memory\n");
		exit (1);
	}

	return ret;
}

char *
xstrdup (const char *s)
{
	void *ret;

	if (s == NULL)
		return NULL;

	ret = strdup (s);

	if (ret == NULL) {
		fprintf (stderr, "Out of memory\n");
		exit (1);
	}

	return ret;
}

