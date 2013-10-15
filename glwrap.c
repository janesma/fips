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

#include "dlwrap.h"

/* The prototypes for some OpenGL functions changed at one point from:
 *
 *	const void* *indices
 * to:
 *	const void* const coist *indices
 *
 * This makes it difficult for us to provide an implementation of
 * these functions that is consistent with the locally-available gl.h
 * since we don't know if the extra const will be present or not.
 *
 * To workaround this problem, we simply #define away const altogether
 * before including gl.h.
 *
 * Kudos to Keith Packard for suggesting this kludge.
 */
#define const

#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>

#include "fips.h"

#include "glwrap.h"

#include "metrics.h"

static void *gl_handle;

void
glwrap_set_gl_handle (void *handle)
{
	if (gl_handle == NULL)
		gl_handle = handle;
}

void *
glwrap_lookup (char *name)
{
	void *ret;

	/* We don't call dlopen here to find the library in which to
	 * perform a dlsym lookup. That's because the application may
	 * be loading libGL.so or libGLESv2.so for its OpenGL symbols.
	 *
	 * So we instead watch for one of those filenames to go by in
	 * our dlopen wrapper, which will then call
	 * glwrap_set_gl_handle to give us the handle to use here.
	 *
	 * If the application hasn't called dlopen on a "libGL"
	 * library, then presumably the application is linked directly
	 * to an OpenGL implementation. In this case, we can use
	 * RTLD_NEXT to find the symbol.
	 *
	 * But just in case, we also let the user override that by
	 * specifying the FIPS_LIBGL environment variable to the path
	 * of the real libGL.so library that fips should dlopen here.
	 */
	if (gl_handle == NULL) {
		const char *path;

		path = getenv ("FIPS_LIBGL");
		if (path) {
			gl_handle = dlopen (path, RTLD_LAZY);

			if (gl_handle == NULL) {
				fprintf (stderr, "Failed to dlopen FIPS_LIBGL: "
					 "%s\n", path);
				exit (1);
			}
		} else {
			gl_handle = RTLD_NEXT;
		}
	}

	ret = dlwrap_real_dlsym (gl_handle, name);

	if (ret == NULL) {
		fprintf (stderr, "Error: glwrap_lookup failed to dlsym %s\n",
			 name);
		exit (1);
	}

	return ret;
}

/* With a program change, we stop the counter, update the
 * active program, then start the counter up again. */
void
glUseProgram (GLuint program)
{
	metrics_counter_stop ();

	metrics_set_current_program (program);

	metrics_counter_start ();

	GLWRAP_DEFER(glUseProgram, program);
}

void
glUseProgramObjectARB (GLhandleARB programObj)
{
	metrics_counter_stop ();

	metrics_set_current_program (programObj);

	metrics_counter_start ();

	GLWRAP_DEFER(glUseProgramObjectARB, programObj);
}
