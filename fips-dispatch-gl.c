/* Copyright © 2012, Intel Corporation
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

#include "fips-dispatch-gl.h"

static void
check_initialized (void)
{
	if (fips_dispatch_initialized)
		return;

	fprintf (stderr,
		 "Internal error: fips_dispatch_init must be called before\n"
		 "any OpenGL function supported via fips_dispatch.\n");
	exit (1);
}

static void
unsupported (const char *name)
{
	fprintf (stderr, "Error: fips failed to find support for %s\n", name);

	exit (1);
}

static void
resolve_glGenQueries (void)
{
	fips_dispatch_glGenQueries = fips_dispatch_lookup ("glGenQueries");

	if (! fips_dispatch_glGenQueries)
		fips_dispatch_glGenQueries = fips_dispatch_lookup ("glGenQueriesARB");

	if (! fips_dispatch_glGenQueries)
		unsupported ("GenQueries");
}

static void
stub_glGenQueries (GLsizei n, GLuint *ids)
{
	check_initialized ();
	resolve_glGenQueries ();
	fips_dispatch_glGenQueries (n, ids);
}

PFNGLGENQUERIESPROC fips_dispatch_glGenQueries = stub_glGenQueries;

static void
resolve_glDeleteQueries (void)
{
	fips_dispatch_glDeleteQueries = fips_dispatch_lookup ("glDeleteQueries");

	if (! fips_dispatch_glDeleteQueries)
		fips_dispatch_glDeleteQueries = fips_dispatch_lookup ("glDeleteQueriesARB");

	if (! fips_dispatch_glDeleteQueries)
		unsupported ("DeleteQueries");
}

static void
stub_glDeleteQueries (GLsizei n, const GLuint * ids)
{
	check_initialized ();
	resolve_glDeleteQueries ();
	fips_dispatch_glDeleteQueries (n, ids);
}

PFNGLDELETEQUERIESPROC fips_dispatch_glDeleteQueries = stub_glDeleteQueries;

static void
resolve_glBeginQuery (void)
{
	fips_dispatch_glBeginQuery = fips_dispatch_lookup ("glBeginQuery");

	if (! fips_dispatch_glBeginQuery)
		fips_dispatch_glBeginQuery = fips_dispatch_lookup ("glBeginQueryARB");

	if (! fips_dispatch_glBeginQuery)
		unsupported ("BeginQuery");
}

static void
stub_glBeginQuery (GLenum target, GLuint id)
{
	check_initialized ();
	resolve_glBeginQuery ();
	fips_dispatch_glBeginQuery (target, id);
}

PFNGLBEGINQUERYPROC fips_dispatch_glBeginQuery = stub_glBeginQuery;

static void
resolve_glEndQuery (void)
{
	fips_dispatch_glEndQuery = fips_dispatch_lookup ("glEndQuery");

	if (! fips_dispatch_glEndQuery)
		fips_dispatch_glEndQuery = fips_dispatch_lookup ("glEndQueryARB");

	if (! fips_dispatch_glEndQuery)
		unsupported ("EndQuery");
}

static void
stub_glEndQuery (GLenum target)
{
	check_initialized ();
	resolve_glEndQuery ();
	fips_dispatch_glEndQuery (target);
}

PFNGLENDQUERYPROC fips_dispatch_glEndQuery = stub_glEndQuery;

static void
resolve_glGetQueryObjectuiv (void)
{
	fips_dispatch_glGetQueryObjectuiv = (PFNGLGETQUERYOBJECTUIVPROC) fips_dispatch_lookup ("glGetQueryObjectuivARB");

	if (! fips_dispatch_glGetQueryObjectuiv)
		unsupported ("GetQueryObjectuiv");
}

static void
stub_glGetQueryObjectuiv (GLuint id, GLenum pname, GLuint * params)
{
	check_initialized ();
	resolve_glGetQueryObjectuiv ();
	fips_dispatch_glGetQueryObjectuiv (id, pname, params);
}

PFNGLGETQUERYOBJECTUIVPROC fips_dispatch_glGetQueryObjectuiv = stub_glGetQueryObjectuiv;
