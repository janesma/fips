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

#define resolve(dispatch, name)			\
	dispatch = fips_dispatch_lookup (name); \
	if (! dispatch)				\
		unsupported (name);

#define resolve2(dispatch, name_a, name_b)			\
	dispatch = fips_dispatch_lookup (name_a);		\
	if (! dispatch)						\
		dispatch = fips_dispatch_lookup (name_b);	\
	if (! dispatch)						\
		unsupported (name_a);

static void
stub_glGetIntegerv (GLenum pname, GLint * params)
{
	check_initialized ();
	resolve (fips_dispatch_glGetIntegerv, "glGetIntegerv");
	fips_dispatch_glGetIntegerv (pname, params);
}

PFNGLGETINTEGERVPROC fips_dispatch_glGetIntegerv = stub_glGetIntegerv;

static const GLubyte *
stub_glGetString (GLenum name)
{
	check_initialized ();
	resolve (fips_dispatch_glGetString, "glGetString");
	return fips_dispatch_glGetString (name);
}

PFNGLGETSTRINGPROC fips_dispatch_glGetString = stub_glGetString;

static const GLubyte *
stub_glGetStringi (GLenum name, GLuint index)
{
	check_initialized ();
	resolve (fips_dispatch_glGetStringi, "glGetStringi");
	return fips_dispatch_glGetStringi (name, index);
}

PFNGLGETSTRINGIPROC fips_dispatch_glGetStringi = stub_glGetStringi;

static void
stub_glGenQueries (GLsizei n, GLuint *ids)
{
	check_initialized ();
	resolve2 (fips_dispatch_glGenQueries,
		  "glGenQueries", "glGenQueriesARB");
	fips_dispatch_glGenQueries (n, ids);
}

PFNGLGENQUERIESPROC fips_dispatch_glGenQueries = stub_glGenQueries;

static void
stub_glDeleteQueries (GLsizei n, const GLuint * ids)
{
	check_initialized ();
	resolve2 (fips_dispatch_glDeleteQueries,
		  "glDeleteQueries", "glDeleteQueriesARB");
	fips_dispatch_glDeleteQueries (n, ids);
}

PFNGLDELETEQUERIESPROC fips_dispatch_glDeleteQueries = stub_glDeleteQueries;

static void
stub_glBeginQuery (GLenum target, GLuint id)
{
	check_initialized ();
	resolve2 (fips_dispatch_glBeginQuery,
		  "glBeginQuery", "glBeginQueryARB");
	fips_dispatch_glBeginQuery (target, id);
}

PFNGLBEGINQUERYPROC fips_dispatch_glBeginQuery = stub_glBeginQuery;

static void
stub_glEndQuery (GLenum target)
{
	check_initialized ();
	resolve2 (fips_dispatch_glEndQuery, "glEndQuery", "glEndQueryARB");
	fips_dispatch_glEndQuery (target);
}

PFNGLENDQUERYPROC fips_dispatch_glEndQuery = stub_glEndQuery;

static void
stub_glGetQueryObjectuiv (GLuint id, GLenum pname, GLuint * params)
{
	check_initialized ();
	resolve2 (fips_dispatch_glGetQueryObjectuiv,
		  "glGetQueryObjectuiv", "glGetQueryObjectuivARB");
	fips_dispatch_glGetQueryObjectuiv (id, pname, params);
}

PFNGLGETQUERYOBJECTUIVPROC fips_dispatch_glGetQueryObjectuiv =
	stub_glGetQueryObjectuiv;

static void
stub_glGetPerfMonitorGroupsAMD (GLint *numGroups, GLsizei groupsSize,
				GLuint *groups)
{
	check_initialized ();
	resolve (fips_dispatch_glGetPerfMonitorGroupsAMD,
		 "glGetPerfMonitorGroupsAMD");
	fips_dispatch_glGetPerfMonitorGroupsAMD (numGroups, groupsSize, groups);
}

PFNGLGETPERFMONITORGROUPSAMDPROC fips_dispatch_glGetPerfMonitorGroupsAMD =
	stub_glGetPerfMonitorGroupsAMD;

static void
stub_glGetPerfMonitorCountersAMD (GLuint group, GLint *numCounters,
				  GLint *maxActiveCounters,
				  GLsizei counterSize, GLuint *counters)
{
	check_initialized ();
	resolve (fips_dispatch_glGetPerfMonitorCountersAMD,
		 "glGetPerfMonitorCountersAMD");
	fips_dispatch_glGetPerfMonitorCountersAMD (group, numCounters,
						   maxActiveCounters,
						   counterSize, counters);
}

PFNGLGETPERFMONITORCOUNTERSAMDPROC fips_dispatch_glGetPerfMonitorCountersAMD =
	stub_glGetPerfMonitorCountersAMD;

static void
stub_glGetPerfMonitorGroupStringAMD (GLuint group, GLsizei bufSize,
				     GLsizei *length, GLchar *groupString)
{
	check_initialized ();
	resolve (fips_dispatch_glGetPerfMonitorGroupStringAMD,
		 "glGetPerfMonitorGroupStringAMD");
	fips_dispatch_glGetPerfMonitorGroupStringAMD (group, bufSize, length,
						      groupString);
}

PFNGLGETPERFMONITORGROUPSTRINGAMDPROC
fips_dispatch_glGetPerfMonitorGroupStringAMD =
	stub_glGetPerfMonitorGroupStringAMD;

static void
stub_glGetPerfMonitorCounterStringAMD (GLuint group, GLuint counter,
				       GLsizei bufSize, GLsizei *length,
				       GLchar *counterString)
{
	check_initialized ();
	resolve (fips_dispatch_glGetPerfMonitorCounterStringAMD,
		 "glGetPerfMonitorCounterStringAMD");
	fips_dispatch_glGetPerfMonitorCounterStringAMD (group, counter,
							bufSize, length,
							counterString);
}

PFNGLGETPERFMONITORCOUNTERSTRINGAMDPROC
fips_dispatch_glGetPerfMonitorCounterStringAMD =
	stub_glGetPerfMonitorCounterStringAMD;

static void
stub_glGetPerfMonitorCounterInfoAMD (GLuint group, GLuint counter, GLenum pname, GLvoid *data)
{
	check_initialized ();
	resolve (fips_dispatch_glGetPerfMonitorCounterInfoAMD,
		 "glGetPerfMonitorCounterInfoAMD");
	fips_dispatch_glGetPerfMonitorCounterInfoAMD (group, counter,
						      pname, data);
}

PFNGLGETPERFMONITORCOUNTERINFOAMDPROC
fips_dispatch_glGetPerfMonitorCounterInfoAMD =
	stub_glGetPerfMonitorCounterInfoAMD;

static void
stub_glGenPerfMonitorsAMD (GLsizei n, GLuint *monitors)
{
	check_initialized ();
	resolve (fips_dispatch_glGenPerfMonitorsAMD, "glGenPerfMonitorsAMD");
	fips_dispatch_glGenPerfMonitorsAMD (n, monitors);
}

PFNGLGENPERFMONITORSAMDPROC fips_dispatch_glGenPerfMonitorsAMD =
	stub_glGenPerfMonitorsAMD;

static void
stub_glDeletePerfMonitorsAMD (GLsizei n, GLuint *monitors)
{
	check_initialized ();
	resolve (fips_dispatch_glDeletePerfMonitorsAMD,
		 "glDeletePerfMonitorsAMD");
	fips_dispatch_glDeletePerfMonitorsAMD (n, monitors);
}

PFNGLDELETEPERFMONITORSAMDPROC fips_dispatch_glDeletePerfMonitorsAMD =
	stub_glDeletePerfMonitorsAMD;

static void
stub_glSelectPerfMonitorCountersAMD (GLuint monitor, GLboolean enable,
				     GLuint group, GLint numCounters,
				     GLuint *counterList)
{
	check_initialized ();
	resolve (fips_dispatch_glSelectPerfMonitorCountersAMD,
		 "glSelectPerfMonitorCountersAMD");
	fips_dispatch_glSelectPerfMonitorCountersAMD (monitor, enable, group,
						      numCounters, counterList);
}

PFNGLSELECTPERFMONITORCOUNTERSAMDPROC
fips_dispatch_glSelectPerfMonitorCountersAMD =
	stub_glSelectPerfMonitorCountersAMD;

static void
stub_glBeginPerfMonitorAMD (GLuint monitor)
{
	check_initialized ();
	resolve (fips_dispatch_glBeginPerfMonitorAMD, "glBeginPerfMonitorAMD");
	fips_dispatch_glBeginPerfMonitorAMD (monitor);
}

PFNGLBEGINPERFMONITORAMDPROC fips_dispatch_glBeginPerfMonitorAMD =
	stub_glBeginPerfMonitorAMD;

static void
stub_glEndPerfMonitorAMD (GLuint monitor)
{
	check_initialized ();
	resolve (fips_dispatch_glEndPerfMonitorAMD, "glEndPerfMonitorAMD");
	fips_dispatch_glEndPerfMonitorAMD (monitor);
}

PFNGLENDPERFMONITORAMDPROC fips_dispatch_glEndPerfMonitorAMD =
	stub_glEndPerfMonitorAMD;

static void
stub_glGetPerfMonitorCounterDataAMD (GLuint monitor, GLenum pname, GLsizei dataSize, GLuint *data, GLint *bytesWritten)
{
	check_initialized ();
	resolve (fips_dispatch_glGetPerfMonitorCounterDataAMD,
		 "glGetPerfMonitorCounterDataAMD");
	fips_dispatch_glGetPerfMonitorCounterDataAMD (monitor, pname, dataSize,
						      data, bytesWritten);
}

PFNGLGETPERFMONITORCOUNTERDATAAMDPROC
fips_dispatch_glGetPerfMonitorCounterDataAMD =
	stub_glGetPerfMonitorCounterDataAMD;
