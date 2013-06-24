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

typedef void (*PFNGLGENQUERIESPROC)(GLsizei, GLuint *);
typedef void (*PFNGLDELETEQUERIESPROC)(GLsizei, const GLuint *);
typedef void (*PFNGLBEGINQUERYPROC)(GLenum, GLuint);
typedef void (*PFNGLENDQUERYPROC)(GLenum);
typedef void (*PFNGLGETQUERYOBJECTUIVPROC)(GLuint, GLenum, GLuint *);

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

#endif /* FIPS_DISPATCH_GL_H */
