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

static int inside_new_list = 0;

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

/* Execute an OpenGL call and time it with a GPU metrics counter. */
#define TIMED_DEFER(function,...) do {					\
	if (! inside_new_list) {					\
		unsigned counter;					\
		counter = metrics_counter_new ();			\
		metrics_counter_start (counter);			\
	}								\
	GLWRAP_DEFER(function, __VA_ARGS__);				\
	if (! inside_new_list) {					\
		metrics_counter_stop ();				\
	}								\
} while (0);

/* Thanks to apitrace source code for the list of OpenGL draw calls. */
void
glDrawArrays (GLenum mode, GLint first, GLsizei count)
{
	TIMED_DEFER (glDrawArrays, mode, first, count);
}

void
glDrawArraysEXT (GLenum mode, GLint first, GLsizei count)
{
	TIMED_DEFER (glDrawArraysEXT, mode, first, count);
}

void
glDrawArraysIndirect (GLenum mode, const GLvoid *indirect)
{
	TIMED_DEFER (glDrawArraysIndirect, mode, indirect);
}

void
glDrawArraysInstanced (GLenum mode, GLint first, GLsizei count,
		       GLsizei primcount)
{
	TIMED_DEFER (glDrawArraysInstanced, mode, first, count, primcount);
}

void
glDrawArraysInstancedARB (GLenum mode, GLint first, GLsizei count,
			  GLsizei primcount)
{
	TIMED_DEFER (glDrawArraysInstancedARB, mode, first, count, primcount);
}

void
glDrawArraysInstancedEXT (GLenum mode, GLint start, GLsizei count,
			  GLsizei primcount)
{
	TIMED_DEFER (glDrawArraysInstancedEXT, mode, start, count, primcount);
}

void
glDrawArraysInstancedBaseInstance (GLenum mode, GLint first, GLsizei count,
				   GLsizei primcount, GLuint baseinstance)
{
	TIMED_DEFER (glDrawArraysInstancedBaseInstance, mode,
		     first, count, primcount, baseinstance);
}

void
glDrawMeshArraysSUN (GLenum mode, GLint first, GLsizei count, GLsizei width)
{
	TIMED_DEFER (glDrawMeshArraysSUN, mode, first, count, width);
}

void
glMultiDrawArrays (GLenum mode, const GLint *first,
		   const GLsizei *count, GLsizei primcount)
{
	TIMED_DEFER (glMultiDrawArrays, mode, first, count, primcount);
}

void
glMultiDrawArraysEXT (GLenum mode, const GLint *first,
		      const GLsizei *count, GLsizei primcount)
{
	TIMED_DEFER (glMultiDrawArraysEXT, mode, first, count, primcount);
}

void
glMultiModeDrawArraysIBM (const GLenum *mode, const GLint *first,
			  const GLsizei *count, GLsizei primcount,
			  GLint modestride)
{
	TIMED_DEFER (glMultiModeDrawArraysIBM, mode,
		     first, count, primcount, modestride);
}

void
glMultiDrawArraysIndirect (GLenum mode, const void *indirect,
			   GLsizei drawcount, GLsizei stride)
{
	TIMED_DEFER (glMultiDrawArraysIndirect, mode, indirect, drawcount, stride);
}

void
glMultiDrawArraysIndirectAMD (GLenum mode, const GLvoid *indirect,
			      GLsizei primcount, GLsizei stride)
{
	TIMED_DEFER (glMultiDrawArraysIndirectAMD, mode,
		     indirect, primcount, stride);
}

void
glDrawElements (GLenum mode, GLsizei count, GLenum type, const GLvoid *indices)
{
	TIMED_DEFER (glDrawElements, mode, count, type, indices);
}

void
glDrawElementsBaseVertex (GLenum mode, GLsizei count, GLenum type,
			  const GLvoid *indices, GLint basevertex)
{
	TIMED_DEFER (glDrawElementsBaseVertex, mode, count,
		     type, indices, basevertex);
}

void
glDrawElementsIndirect (GLenum mode, GLenum type, const GLvoid *indirect)
{
	TIMED_DEFER (glDrawElementsIndirect, mode, type, indirect);
}

void
glDrawElementsInstanced (GLenum mode, GLsizei count, GLenum type,
			 const GLvoid *indices, GLsizei primcount)
{
	TIMED_DEFER (glDrawElementsInstanced, mode, count,
		     type, indices, primcount);
}

void
glDrawElementsInstancedARB (GLenum mode, GLsizei count, GLenum type,
			    const GLvoid *indices, GLsizei primcount)
{
	TIMED_DEFER (glDrawElementsInstancedARB, mode, count,
		     type, indices, primcount);
}

void
glDrawElementsInstancedEXT (GLenum mode, GLsizei count, GLenum type,
			    const GLvoid *indices, GLsizei primcount)
{
	TIMED_DEFER (glDrawElementsInstancedEXT, mode, count,
		     type, indices, primcount);
}

void
glDrawElementsInstancedBaseVertex (GLenum mode, GLsizei count, GLenum type,
				   const GLvoid *indices, GLsizei primcount,
				   GLint basevertex)
{
	TIMED_DEFER (glDrawElementsInstancedBaseVertex, mode, count,
		     type, indices, primcount, basevertex);
}

void
glDrawElementsInstancedBaseInstance (GLenum mode, GLsizei count, GLenum type,
				     const void *indices, GLsizei primcount,
				     GLuint baseinstance)
{
	TIMED_DEFER (glDrawElementsInstancedBaseInstance, mode, count, type,
		     indices, primcount, baseinstance);
}

void
glDrawElementsInstancedBaseVertexBaseInstance (GLenum mode, GLsizei count,
	              GLenum type, const void *indices, GLsizei primcount,
		      GLint basevertex, GLuint baseinstance)
{
	TIMED_DEFER (glDrawElementsInstancedBaseVertexBaseInstance, mode,
		     count, type, indices, primcount, basevertex, baseinstance);
}

void
glDrawRangeElements (GLenum mode, GLuint start, GLuint end, GLsizei count,
		     GLenum type, const GLvoid *indices)
{
	TIMED_DEFER (glDrawRangeElements, mode, start, end,
		     count, type, indices);
}

void
glDrawRangeElementsEXT (GLenum mode, GLuint start, GLuint end, GLsizei count,
			GLenum type, const GLvoid *indices)
{
	TIMED_DEFER (glDrawRangeElementsEXT, mode, start, end,
		     count, type, indices);
}

void
glDrawRangeElementsBaseVertex (GLenum mode, GLuint start, GLuint end,
			       GLsizei count, GLenum type,
			       const GLvoid *indices, GLint basevertex)
{
	TIMED_DEFER (glDrawRangeElementsBaseVertex, mode, start, end,
		     count, type, indices, basevertex);
}

void
glMultiDrawElements (GLenum mode, const GLsizei *count, GLenum type,
		     const GLvoid* *indices, GLsizei primcount)
{
	TIMED_DEFER (glMultiDrawElements, mode, count, type,
		     indices, primcount);
}

void
glMultiDrawElementsBaseVertex (GLenum mode, const GLsizei *count,
			       GLenum type, const GLvoid* *indices,
			       GLsizei primcount, const GLint *basevertex)
{
	TIMED_DEFER (glMultiDrawElementsBaseVertex, mode, count,
		     type, indices, primcount, basevertex);
}

void
glMultiDrawElementsEXT (GLenum mode, const GLsizei *count, GLenum type,
			const GLvoid* *indices, GLsizei primcount)
{
	TIMED_DEFER (glMultiDrawElementsEXT, mode, count,
		     type, indices, primcount);
}

void
glMultiModeDrawElementsIBM (const GLenum *mode, const GLsizei *count,
			    GLenum type, const GLvoid* const *indices,
			    GLsizei primcount, GLint modestride)
{
	TIMED_DEFER (glMultiModeDrawElementsIBM, mode, count,
		     type, indices, primcount, modestride);
}

void
glMultiDrawElementsIndirect (GLenum mode, GLenum type, const void *indirect,
			     GLsizei drawcount, GLsizei stride)
{
	TIMED_DEFER (glMultiDrawElementsIndirect, mode, type,
		     indirect, drawcount, stride);
}

void
glMultiDrawElementsIndirectAMD (GLenum mode, GLenum type,
				const GLvoid *indirect,
				GLsizei primcount, GLsizei stride)
{
	TIMED_DEFER (glMultiDrawElementsIndirectAMD, mode, type,
		     indirect, primcount, stride);
}

void
glCallList (GLuint list)
{
	TIMED_DEFER (glCallList, list);
}

void
glCallLists (GLsizei n, GLenum type, const GLvoid *lists)
{
	TIMED_DEFER (glCallLists, n, type, lists);
}

void
glClear (GLbitfield mask)
{
	TIMED_DEFER (glClear, mask);
}

/* We can't just use TIMED_DEFER for glBegin/glEnd since the metrics
 * counter must be started before glBegin and stopped after glEnd,
 * (that is, everything from glBegin to glEnd is counted as a single
 * operation). */
void
glBegin (GLenum mode)
{
	if (! inside_new_list)
	{
		unsigned counter;
		counter = metrics_counter_new ();
		metrics_counter_start (counter);
	}

	GLWRAP_DEFER (glBegin, mode);
}

void
glEnd (void)
{
	GLWRAP_DEFER (glEnd);

	if (! inside_new_list) {
		metrics_counter_stop ();
	}
}

/* And we need to track display lists to avoid inserting queries
 * inside the list while it's being constructed. */
void
glNewList (GLuint list, GLenum mode)
{
	inside_new_list = 1;
	GLWRAP_DEFER (glNewList, list, mode);
}

void
glEndList (void)
{
	GLWRAP_DEFER (glEndList);
	inside_new_list = 0;
}

void
glDrawPixels (GLsizei width, GLsizei height, GLenum format,
	      GLenum type, const GLvoid *pixels)
{
	TIMED_DEFER (glDrawPixels, width, height, format, type, pixels);
}

void
glBlitFramebuffer (GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1,
		   GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1,
		   GLbitfield mask, GLenum filter)
{
	TIMED_DEFER (glBlitFramebuffer,
		     srcX0, srcY0, srcX1, srcY1,
		     dstX0, dstY0, dstX1, dstY1,
		     mask, filter);
}

void
glBlitFramebufferEXT (GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1,
		      GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1,
		      GLbitfield mask, GLenum filter)
{
	TIMED_DEFER (glBlitFramebufferEXT,
		     srcX0, srcY0, srcX1, srcY1,
		     dstX0, dstY0, dstX1, dstY1,
		     mask, filter);
}

void
glUseProgram (GLuint program)
{
	metrics_set_current_program (program);

	GLWRAP_DEFER(glUseProgram, program);
}

void
glUseProgramObjectARB (GLhandleARB programObj)
{
	metrics_set_current_program (programObj);

	GLWRAP_DEFER(glUseProgramObjectARB, programObj);
}
