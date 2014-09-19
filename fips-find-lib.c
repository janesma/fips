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
#include <string.h>

/* The purpose of this program is to locate the libEGL or libGL which would
 * ordinarily be found by fips's target application.  It checks the sole
 * parameter and dynamically loads libEGL or libGL.  The file name of the lib
 * that was loaded is printed.
 *
 * We need to use an external program for this purpose to handle the case where
 * fips does not have the same arch as the target program.  Calling dlopen from
 * a 64-bit fips would obtain an incompatible library for a 32-bit target
 * application.  This program is compiled in 32 and 64 bit variants, and fips
 * calls the right one based on the arch of the target program.
 *
 * Using a program like this allows us to directly inspect what the
 * dynamic loader actually does. This is much more reliable than
 * trying to recreate its search-path logic which would requrie at
 * least:
 *
 *	* Parsing LD_LIBRARY_PATH
 *
 *	* Correctly interpreting $LIB within LD_LIBRARY_PATH
 *
 *	* Knowing the canonical name for the current architecture for
 *        *$LIB
 *
 *	* Parsing /etc/ld.so.conf entries
 *
 *	* etc. etc.
 */

void usage();
void usage()
{
	printf ("USAGE: fips-find-lib < GL | EGL >\n"
		"       pass either EGL or GL as the sole parameter\n");
}

int
main (int argc, const char **argv)
{
	Dl_info info;
	void *handle;
	void *function;

	if (argc != 2)
	{
		usage();
		return 1;
	}

	if (strncmp(argv[1], "EGL", 3) == 0)
	{        
		handle = dlopen ("libEGL.so.1", RTLD_NOW);
		if (handle == NULL) {
			fprintf (stderr, "fips-find-lib: Failed to dlopen libEGL.so.1\n");
			return 1;
		}

		function = dlsym (handle, "eglSwapBuffers");
		if (function == NULL) {
			fprintf (stderr, "fips-find-lib: Failed to dlsym eglSwapBuffers\n");
			return 1;
		}

		if (dladdr (function, &info) == 0) {
			fprintf (stderr, "fips-find-lib: Failed to dladdr eglSwapBuffers\n");
			return 1;
		}
		printf ("%s\n", info.dli_fname);

		return 0;
	}

	if (strncmp(argv[1], "GL", 2) != 0)
	{
		usage();
		return 1;
	}
    
	handle = dlopen ("libGL.so.1", RTLD_NOW);
	if (handle == NULL) {
		fprintf (stderr, "fips-find-lib: Failed to dlopen libGL.so.1\n");
		return 1;
	}

	function = dlsym (handle, "glClear");
	if (function == NULL) {
		fprintf (stderr, "fips-find-lib: Failed to dlsym glClear\n");
		return 1;
	}

	if (dladdr (function, &info) == 0) {
		fprintf (stderr, "fips-find-lib: Failed to dladdr glClear\n");
		return 1;
	}
	printf ("%s\n", info.dli_fname);

	return 0;
}

