/* Copyright © 2014, Intel Corporation
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
#include <dlfcn.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <fcntl.h>

void *libgl_handle = NULL;
void *libegl_handle = NULL;
void *libfips_handle = NULL;

#define STRNCMP_LITERAL(str, literal) strncmp (str, literal, sizeof (literal) - 1)

static void
open_lib_handles (void)
{
	const char *libgl_path = NULL, *libegl_path = NULL, *libfips_path = NULL;

	/* only execute once */
	if (libgl_handle)
		return;

	/* Open libGL.so handle */
	libgl_path = getenv ("FIPS_GL");
	assert(libgl_path != NULL && strlen (libgl_path) > 0);
	if (libgl_path == NULL || strlen (libgl_path) == 0) {
		fprintf (stderr,
			 "fips: Error: Failed to detect OpenGL library.\n"
			 "FIPS_GL not set.\n");
		exit (1);
	}

	dlerror();
	libgl_handle = dlopen (libgl_path, RTLD_LAZY | RTLD_GLOBAL);
	if (libgl_handle == NULL) {
		fprintf (stderr, "fips: Error: Failed to dlopen %s: %s\n",
			 libgl_path, dlerror());
		exit (1);
	}

	/* Open libEGL.so handle */
	libegl_path = getenv ("FIPS_EGL");
	assert(libegl_path != NULL && strlen (libegl_path) > 0);
	if (libegl_path == NULL || strlen (libegl_path) == 0) {
		fprintf (stderr,
			 "fips: Error: Failed to detect EGL library.\n"
			 "FIPS_EGL not set.\n");
		exit (1);
	}

	dlerror();
	libegl_handle = dlopen (libegl_path, RTLD_LAZY | RTLD_GLOBAL);
	if (libegl_handle == NULL) {
		fprintf (stderr, "fips: Error: Failed to dlopen %s: %s\n",
			 libegl_path, dlerror());
		exit (1);
	}

	/* Open libfips.so handle */
	libfips_path = getenv ("FIPS_LIBFIPS");
	assert(libfips_path != NULL && strlen (libfips_path) > 0);
	if (libfips_path == NULL || strlen (libfips_path) == 0) {
		fprintf (stderr,
			 "fips: Error: Failed to detect fips library.\n"
			 "FIPS_LIBFIPS not set.\n");
		exit (1);
	}

	dlerror();
	libfips_handle = dlopen (libfips_path, RTLD_LAZY | RTLD_GLOBAL);
	if (libfips_handle == NULL) {
		fprintf (stderr, "fips: Error: Failed to dlopen %s: %s\n",
			 libfips_path, dlerror());
		exit (1);
	}
}

static void
fips_init (void) __attribute__((constructor));

static void
fips_init (void)
{
	open_lib_handles ();
}

static void *
resolve (const char *name)
{
	void *symbol;

	/* anything in libfips has priority on all symbols. */
	symbol = dlsym (libfips_handle, name);
	if (symbol)
		return symbol;

	/* Search for "real" the function implementation in libGL or libEGL */
	symbol = dlsym (libgl_handle, name);
	if (symbol)
		return symbol;
	symbol = dlsym (libegl_handle, name);
	if (symbol)
		return symbol;
	return NULL;
}

void
(*glXGetProcAddress (const unsigned char *name))(void);

void
(*glXGetProcAddress (const unsigned char *name))(void)
{
	static int first_call = 1;
	static typeof (&glXGetProcAddress) libgl_glXGetProcAddress;
	void *symbol;

	/* On the first call, check if the wrapper provides an
	 * implementation of this function. */
	if (first_call) {
		libgl_glXGetProcAddress = dlsym (libgl_handle,
						 "glXGetProcAddress");
		first_call = 0;
	}

	/* Give the fips version if it exists. */
	symbol = dlsym (libfips_handle, (char *)name);
	if (symbol)
		return symbol;

	/* otherwize defer to the underlying
	 * glXGetProcAddress */
	return libgl_glXGetProcAddress (name);

}

void
(*glXGetProcAddressARB (const unsigned char *name))(void);

void
(*glXGetProcAddressARB (const unsigned char *name))(void)
{
	static int first_call = 1;
	static typeof (&glXGetProcAddressARB) libgl_glXGetProcAddressARB;
	void *symbol;

	/* On the first call, check if the wrapper provides an
	 * implementation of this function. */
	if (first_call) {
		libgl_glXGetProcAddressARB = dlsym (libgl_handle,
						    "glXGetProcAddressARB");
		first_call = 0;
	}

	/* Give the fips version if it exists. */
	symbol = dlsym (libfips_handle, (char *)name);
	if (symbol)
		return symbol;

	/* otherwize defer to the underlying
	 * glXGetProcAddressARB */
	return libgl_glXGetProcAddressARB (name);

}

void *
(*eglGetProcAddress (char const *func))(void);

void *
(*eglGetProcAddress (char const *func))(void)
{
	static int first_call = 1;
	static typeof (&eglGetProcAddress) libegl_eglGetProcAddress;
	void *symbol;

	/* On the first call, check if the wrapper provides an
	 * implementation of this function. */
	if (first_call) {
		libegl_eglGetProcAddress = dlsym(libegl_handle, "eglGetProcAddress");
		first_call = 0;
	}

	/* Give the fips version if it exists. */
	symbol = dlsym (libfips_handle, func);
	if (symbol)
		return symbol;

	/* otherwize defer to the underlying
	 * eglGetProcAddress */
	return libegl_eglGetProcAddress(func);
}

#define FIPS_API(name)							\
	void * name() __attribute__((ifunc(#name "_resolver")));	\
	static void *							\
	name ## _resolver (void) { return resolve (#name); }

#include "specs/gl.def"
#include "specs/glx.def"
#include "specs/egl.def"
