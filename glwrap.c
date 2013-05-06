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

#include "fips.h"

#include "glwrap.h"

#include "metrics.h"

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

#include "dlwrap.h"

void *
glwrap_lookup (char *name)
{
	const char *libgl_filename = "libGL.so.1";
	static void *libgl_handle = NULL;

	if (! libgl_handle) {
		libgl_handle = dlwrap_real_dlopen (libgl_filename, RTLD_NOW | RTLD_DEEPBIND);
		if (! libgl_handle) {
			fprintf (stderr, "Error: Failed to dlopen %s\n",
				 libgl_filename);
			exit (1);
		}
	}

	return dlwrap_real_dlsym (libgl_handle, name);
}

/* Execute a glBegineQuery/glEndQuery pair around an OpenGL call. */
#define TIMED_DEFER(function,...) do {			\
	unsigned counter;				\
	counter = metrics_add_counter ();		\
	glBeginQuery (GL_TIME_ELAPSED, counter);	\
	GLWRAP_DEFER(function, __VA_ARGS__);		\
	glEndQuery (GL_TIME_ELAPSED);			\
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

/* FIXME?
void
glMultiDrawArraysIndirect (...)
{
	TIMED_DEFER (glMultiDrawArraysIndirect, ...);
}
*/

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

/* FIXME?
void
glMultiDrawElementsIndirect (...)
{
	TIMED_DEFER (glMultiDrawElementsIndirect, ...);
}
*/

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

void
glEnd (void)
{
	TIMED_DEFER (glEnd,);
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
