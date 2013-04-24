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
#include <limits.h>
#include <getopt.h>

#include "execute.h"

static void
usage (void)
{
	printf("Usage: fips [OPTIONS...] <program> [program args...]\n"
	       "\n"
	       "Execute <program> and report GPU performance counters\n"
	       "\n"
	       "Options:\n"
	       "	-h, --help	show this help message\n"
	       "\n");
}

int
main (int argc, char *argv[])
{
	int opt, ret;

	const char *short_options = "h";
	const struct option long_options[] = {
		{"help", no_argument, 0, 'h'},
		{0, 0, 0, 0}
	};

	while (1)
	{
		opt = getopt_long(argc, argv, short_options, long_options, NULL);
		if (opt == -1)
			break;

		switch (opt) {
		case 'h':
			usage ();
			return 0;
		case '?':
			break;
		default:
			fprintf(stderr, "Internal error: "
				"unexpected getopt value: %d\n", opt);
			exit (1);
		}
	}

	if (optind >= argc) {
		fprintf (stderr, "Error: No program name provided, "
			 "see (fips --help)\n");
		exit (1);
	}

	ret = execute_with_fips_preload (argc - optind, &argv[optind]);

	return ret;
}


