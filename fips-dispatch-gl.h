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

/* This header file provides an interface to (the fips-dispatch subset
 * of) OpenGL through dynamic dispatch as an alternative to including
 * <GL/gl.h>.
 *
 * No file in fips should include both <GL/gl.h> and
 * "fips-dispatch-gl.h" (whether directly or indirectly).
 */

#ifndef FIPS_DISPATCH_GL_H
#define FIPS_DISPATCH_GL_H

#include "fips-dispatch.h"

#include <stdint.h>
#include <stddef.h>

#define GL_FALSE 0
#define GL_TRUE	 1

#define GL_BYTE					0x1400
#define GL_UNSIGNED_BYTE			0x1401
#define GL_SHORT				0x1402
#define GL_UNSIGNED_SHORT			0x1403
#define GL_INT					0x1404
#define GL_UNSIGNED_INT				0x1405
#define GL_FLOAT				0x1406
#define GL_2_BYTES				0x1407
#define GL_3_BYTES				0x1408
#define GL_4_BYTES				0x1409
#define GL_DOUBLE				0x140A
#define GL_EXTENSIONS				0x1F03
#define GL_NUM_EXTENSIONS                 	0x821D

typedef unsigned int GLenum;
typedef unsigned int GLbitfield;
typedef unsigned int GLuint;
typedef int GLint;
typedef int GLsizei;
typedef unsigned char GLboolean;
typedef signed char GLbyte;
typedef short GLshort;
typedef unsigned char GLubyte;
typedef unsigned short GLushort;
typedef unsigned long GLulong;
typedef float GLfloat;
typedef float GLclampf;
typedef double GLdouble;
typedef double GLclampd;
typedef void GLvoid;
typedef int64_t GLint64EXT;
typedef uint64_t GLuint64EXT;
typedef GLint64EXT  GLint64;
typedef GLuint64EXT GLuint64;
typedef char GLchar;
typedef ptrdiff_t GLintptr;
typedef ptrdiff_t GLsizeiptr;
typedef ptrdiff_t GLintptrARB;
typedef ptrdiff_t GLsizeiptrARB;
typedef char GLcharARB;
typedef unsigned int GLhandleARB;

typedef void (*PFNGLGETINTEGERVPROC) (GLenum pname, GLint *params);
typedef const GLubyte* (*PFNGLGETSTRINGPROC)(GLenum name);
typedef const GLubyte* (*PFNGLGETSTRINGIPROC)(GLenum name, GLuint index);
typedef void (*PFNGLGENQUERIESPROC)(GLsizei, GLuint *);
typedef void (*PFNGLDELETEQUERIESPROC)(GLsizei, const GLuint *);
typedef void (*PFNGLBEGINQUERYPROC)(GLenum, GLuint);
typedef void (*PFNGLENDQUERYPROC)(GLenum);
typedef void (*PFNGLGETQUERYOBJECTUIVPROC)(GLuint, GLenum, GLuint *);

typedef void (*PFNGLGETPERFMONITORGROUPSAMDPROC)(GLint *, GLsizei, GLuint *);
typedef void (*PFNGLGETPERFMONITORCOUNTERSAMDPROC)(GLuint, GLint *, GLint *,
						   GLsizei, GLuint *);
typedef void (*PFNGLGETPERFMONITORGROUPSTRINGAMDPROC)(GLuint, GLsizei,
						      GLsizei *, GLchar *);
typedef void (*PFNGLGETPERFMONITORCOUNTERSTRINGAMDPROC)(GLuint, GLuint,
							GLsizei, GLsizei *, GLchar *);
typedef void (*PFNGLGETPERFMONITORCOUNTERINFOAMDPROC)(GLuint, GLuint,
						      GLenum, GLvoid *);
typedef void (*PFNGLGENPERFMONITORSAMDPROC)(GLsizei, GLuint *);
typedef void (*PFNGLDELETEPERFMONITORSAMDPROC)(GLsizei, GLuint *);
typedef void (*PFNGLSELECTPERFMONITORCOUNTERSAMDPROC)(GLuint, GLboolean,
						      GLuint, GLint, GLuint *);
typedef void (*PFNGLBEGINPERFMONITORAMDPROC)(GLuint);
typedef void (*PFNGLENDPERFMONITORAMDPROC)(GLuint);
typedef void (*PFNGLGETPERFMONITORCOUNTERDATAAMDPROC)(GLuint, GLenum,
						      GLsizei, GLuint *, GLint *);

extern PFNGLGETINTEGERVPROC fips_dispatch_glGetIntegerv;
#define glGetIntegerv fips_dispatch_glGetIntegerv

extern PFNGLGETSTRINGPROC fips_dispatch_glGetString;
#define glGetString fips_dispatch_glGetString

extern PFNGLGETSTRINGIPROC fips_dispatch_glGetStringi;
#define glGetStringi fips_dispatch_glGetStringi

#define GL_QUERY_RESULT 0x8866
#define GL_QUERY_RESULT_AVAILABLE 0x8867
#define GL_TIME_ELAPSED 0x88BF

extern PFNGLGENQUERIESPROC fips_dispatch_glGenQueries;
#define glGenQueries fips_dispatch_glGenQueries

extern PFNGLDELETEQUERIESPROC fips_dispatch_glDeleteQueries;
#define glDeleteQueries fips_dispatch_glDeleteQueries

extern PFNGLBEGINQUERYPROC fips_dispatch_glBeginQuery;
#define glBeginQuery fips_dispatch_glBeginQuery

extern PFNGLENDQUERYPROC fips_dispatch_glEndQuery;
#define glEndQuery fips_dispatch_glEndQuery

extern PFNGLGETQUERYOBJECTUIVPROC fips_dispatch_glGetQueryObjectuiv;
#define glGetQueryObjectuiv fips_dispatch_glGetQueryObjectuiv

#define GL_COUNTER_TYPE_AMD               0x8BC0
#define GL_COUNTER_RANGE_AMD              0x8BC1
#define GL_UNSIGNED_INT64_AMD             0x8BC2
#define GL_PERCENTAGE_AMD                 0x8BC3
#define GL_PERFMON_RESULT_AVAILABLE_AMD   0x8BC4
#define GL_PERFMON_RESULT_SIZE_AMD        0x8BC5
#define GL_PERFMON_RESULT_AMD             0x8BC6

extern PFNGLGETPERFMONITORGROUPSAMDPROC fips_dispatch_glGetPerfMonitorGroupsAMD;
#define glGetPerfMonitorGroupsAMD fips_dispatch_glGetPerfMonitorGroupsAMD

extern PFNGLGETPERFMONITORCOUNTERSAMDPROC fips_dispatch_glGetPerfMonitorCountersAMD;
#define glGetPerfMonitorCountersAMD fips_dispatch_glGetPerfMonitorCountersAMD

extern PFNGLGETPERFMONITORGROUPSTRINGAMDPROC fips_dispatch_glGetPerfMonitorGroupStringAMD;
#define glGetPerfMonitorGroupStringAMD fips_dispatch_glGetPerfMonitorGroupStringAMD

extern PFNGLGETPERFMONITORCOUNTERSTRINGAMDPROC fips_dispatch_glGetPerfMonitorCounterStringAMD;
#define glGetPerfMonitorCounterStringAMD fips_dispatch_glGetPerfMonitorCounterStringAMD

extern PFNGLGETPERFMONITORCOUNTERINFOAMDPROC fips_dispatch_glGetPerfMonitorCounterInfoAMD;
#define glGetPerfMonitorCounterInfoAMD fips_dispatch_glGetPerfMonitorCounterInfoAMD

extern PFNGLGENPERFMONITORSAMDPROC fips_dispatch_glGenPerfMonitorsAMD;
#define glGenPerfMonitorsAMD fips_dispatch_glGenPerfMonitorsAMD

extern PFNGLDELETEPERFMONITORSAMDPROC fips_dispatch_glDeletePerfMonitorsAMD;
#define glDeletePerfMonitorsAMD fips_dispatch_glDeletePerfMonitorsAMD

extern PFNGLSELECTPERFMONITORCOUNTERSAMDPROC fips_dispatch_glSelectPerfMonitorCountersAMD;
#define glSelectPerfMonitorCountersAMD fips_dispatch_glSelectPerfMonitorCountersAMD

extern PFNGLBEGINPERFMONITORAMDPROC fips_dispatch_glBeginPerfMonitorAMD;
#define glBeginPerfMonitorAMD fips_dispatch_glBeginPerfMonitorAMD

extern PFNGLENDPERFMONITORAMDPROC fips_dispatch_glEndPerfMonitorAMD;
#define glEndPerfMonitorAMD fips_dispatch_glEndPerfMonitorAMD

extern PFNGLGETPERFMONITORCOUNTERDATAAMDPROC fips_dispatch_glGetPerfMonitorCounterDataAMD;
#define glGetPerfMonitorCounterDataAMD fips_dispatch_glGetPerfMonitorCounterDataAMD

#endif /* FIPS_DISPATCH_GL_H */
