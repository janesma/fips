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

/* dladdr is a glibc extension */
#define _GNU_SOURCE
#include <dlfcn.h>

#include <stdio.h>
#include <stdlib.h>

#include <string.h>

#include "dlwrap.h"

#define STRNCMP_LITERAL(var, literal) \
    strncmp ((var), (literal), sizeof (literal) - 1)

void *libfips_handle;

typedef void * (* fips_dlopen_t)(const char * filename, int flag);
typedef void * (* fips_dlsym_t)(void *handle, const char *symbol);

/* Many (most?) OpenGL programs dlopen libGL.so.1 rather than linking
 * against it directly, which means they would not be seeing our
 * wrapped GL symbols via LD_PRELOAD. So we catch the dlopen in a
 * wrapper here and redirect it to our library.
 */
void *
dlopen (const char *filename, int flag)
{
	Dl_info info;

	/* Not libGL, so just return real dlopen */
	if (STRNCMP_LITERAL (filename, "libGL.so"))
		return dlwrap_real_dlopen (filename, flag);

	/* Redirect all dlopens to libGL to our own wrapper library.
	 * We find our own filename by looking up this very function
	 * (that is, this "dlopen"), with dladdr).*/
	if (dladdr (dlopen, &info)) {
		libfips_handle = dlwrap_real_dlopen (info.dli_fname, flag);
		return libfips_handle;
	} else {
		fprintf (stderr, "Error: Failed to redirect dlopen of %s:\n",
			 filename);
		exit (1);
	}
}

void *
dlwrap_real_dlopen (const char *filename, int flag)
{
	fips_dlopen_t real_dlopen = NULL;

	if (! real_dlopen) {
		real_dlopen = (fips_dlopen_t) dlwrap_real_dlsym (RTLD_NEXT, "dlopen");
		if (! real_dlopen) {
			fprintf (stderr, "Error: Failed to find symbol for dlopen.\n");
			exit (1);
		}
	}

	return real_dlopen (filename, flag);
}

/* Since we redirect a dlopen of libGL.so to libfips we need to ensure
 * that dlysm succeeds for all functions that might be defined in the
 * real, underlying libGL library. But we're far too lazy to implement
 * wrappers for function that would simply pass-through, so instead we
 * also wrap dlysm and arrange for it to pass things through with
 * RTLD_next if libfips does not have the function desired.
*/
void *
dlsym (void *handle, const char *name)
{
	static void *libgl_handle = NULL;
	static void *symbol;

	/* All gl symbols are preferentially looked up in libfips. */
	if (STRNCMP_LITERAL (name, "gl") == 0) {
		symbol = dlwrap_real_dlsym (libfips_handle, name);
		if (symbol)
			return symbol;
	}

	/* Failing that, anything specifically requested from the
	 * libfips library should be redirected to a real GL
	 * library. */
	if (handle == libfips_handle) {
		if (! libgl_handle)
			libgl_handle = dlwrap_real_dlopen ("libGL.so.1", RTLD_LAZY);
		return dlwrap_real_dlsym (libgl_handle, name);
	}

	/* And anything else is some unrelated dlsym. Just pass it through. */
	return dlwrap_real_dlsym (handle, name);
}

void *
dlwrap_real_dlsym (void *handle, const char *name)
{
	static fips_dlsym_t real_dlsym = NULL;

	if (! real_dlsym) {
		/* FIXME: This brute-force, hard-coded searching for a versioned
		 * symbol is really ugly. The only reason I'm doing this is because
		 * I need some way to lookup the "dlsym" function in libdl, but
		 * I can't use 'dlsym' to do it. So dlvsym works, but forces me
		 * to guess what the right version is.
		 *
		 * Potential fixes here:
		 *
		 *   1. Use libelf to actually inspect libdl.so and
		 *      find the right version, (finding the right
		 *      libdl.so can be made easier with
		 *      dl_iterate_phdr).
		 *
		 *   2. Use libelf to find the offset of the 'dlsym'
		 *      symbol within libdl.so, (and then add this to
		 *      the base address at which libdl.so is loaded
		 *      as reported by dl_iterate_phdr).
		 *
		 * In the meantime, I'll just keep augmenting this
		 * hard-coded version list as people report bugs. */
		const char *version[] = {
			"GLIBC_2.2.5",
			"GLIBC_2.0"
		};
		int num_versions = sizeof(version) / sizeof(version[0]);
		int i;
		for (i = 0; i < num_versions; i++) {
			real_dlsym = (fips_dlsym_t) dlvsym (RTLD_NEXT, "dlsym",
							    version[i]);
			if (real_dlsym)
				break;
		}
		if (i == num_versions) {
			fprintf (stderr, "Internal error: Failed to find real dlsym\n");
			fprintf (stderr,
"This may be a simple matter of fips not knowing about the version of GLIBC that\n"
"your program is using. Current known versions are:\n\n\t");
			for (i = 0; i < num_versions; i++)
				fprintf (stderr, "%s ", version[i]);
			fprintf(stderr,
"\n\nYou can inspect your version by first finding libdl.so.2:\n"
"\n"
"\tldd <your-program> | grep libdl.so\n"
"\n"
"And then inspecting the version attached to the dlsym symbol:\n"
"\n"
"\treadelf -s /path/to/libdl.so.2 | grep dlsym\n"
"\n"
"And finally, adding the version to dlwrap.c:dlwrap_real_dlsym.\n");

			exit (1);
		}
	}

	return real_dlsym (handle, name);
}
