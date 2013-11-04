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

#include "context.h"

/* As of glext.h version 20131008 some types changed.
 *
 * I have no idea why some internalFormats changed from GLenum to
 * GLint while others changed from GLint to GLenum.
 *
 * But, in order to compile with either version, we track these.
 */
#if GL_GLEXT_VERSION >= 20131008
#define GLint_or_enum GLenum
#define GLenum_or_int GLint
#else
#define GLint_or_enum GLint
#define GLenum_or_int GLenum
#endif

static void *gl_handle;

/* Switch metrics operation persistently, (until next SWITCH) */
#define SWITCH_METRICS_OP(op)			\
	context_counter_stop ();		\
	context_set_current_op (op);		\
	context_counter_start ();

/* Switch metrics operation temporarily, see RESTORE_METRICS_OP */
#define SAVE_THEN_SWITCH_METRICS_OP(op)			\
	metrics_op_t save = context_get_current_op ();	\
	SWITCH_METRICS_OP (op);

/* Switch back to metrics operation saved by SAVE_THEN_SWITCH_METRICS_OP */
#define RESTORE_METRICS_OP(op)				\
	SWITCH_METRICS_OP (save);

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
		if (! path)
			path = getenv ("GLAZE_LIBGL");
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
		fprintf (stderr, "fips: Error: glwrap_lookup failed to dlsym %s\n",
			 name);
		exit (1);
	}

	return ret;
}

/* With a program change, we stop the counter, update the
 * active program, then start the counter up again. */
void
glUseProgram (GLuint program)
{
	SWITCH_METRICS_OP (METRICS_OP_SHADER + program);

	GLWRAP_DEFER(glUseProgram, program);
}

void
glUseProgramObjectARB (GLhandleARB programObj)
{
	SWITCH_METRICS_OP (METRICS_OP_SHADER + programObj);

	GLWRAP_DEFER(glUseProgramObjectARB, programObj);
}

/* METRICS_OP_ACCUM */
void
glAccum (GLenum op, GLfloat value)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_ACCUM);

	GLWRAP_DEFER (glAccum, op, value);

	RESTORE_METRICS_OP ();
}

void
glClearAccum (GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_ACCUM);

	GLWRAP_DEFER (glClearAccum, red, green, blue, alpha);

	RESTORE_METRICS_OP ();
}

void
glClearAccumxOES (GLfixed red, GLfixed green, GLfixed blue, GLfixed alpha)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_ACCUM);

	GLWRAP_DEFER (glClearAccumxOES, red, green, blue, alpha);

	RESTORE_METRICS_OP ();
}

/* METRICS_OP_BUFFER_DATA */
void
glBufferData (GLenum target, GLsizeiptr size, const GLvoid *data, GLenum usage)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_BUFFER_DATA);

	GLWRAP_DEFER (glBufferData, target, size, data, usage);

	RESTORE_METRICS_OP ();
}

void
glNamedBufferDataEXT (GLuint buffer, GLsizeiptr size, const GLvoid *data,
		      GLenum usage)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_BUFFER_DATA);

	GLWRAP_DEFER (glNamedBufferDataEXT, buffer, size, data, usage);

	RESTORE_METRICS_OP ();
}

void
glBufferSubData (GLenum target, GLintptr offset, GLsizeiptr size,
		 const GLvoid *data)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_BUFFER_DATA);

	GLWRAP_DEFER (glBufferSubData, target, offset, size, data);

	RESTORE_METRICS_OP ();
}

void
glNamedBufferSubDataEXT (GLuint buffer, GLintptr offset, GLsizeiptr size,
			 const GLvoid *data)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_BUFFER_DATA);

	GLWRAP_DEFER (glNamedBufferSubDataEXT, buffer, offset, size, data);

	RESTORE_METRICS_OP ();
}

void *
glMapBuffer (GLenum target, GLenum access)
{
	void *ret;

	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_BUFFER_DATA);

	GLWRAP_DEFER_WITH_RETURN (ret, glMapBuffer, target, access);

	RESTORE_METRICS_OP ();

	return ret;
}

void *
glMapBufferARB (GLenum target, GLenum access)
{
	void *ret;

	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_BUFFER_DATA);

	GLWRAP_DEFER_WITH_RETURN (ret, glMapBufferARB, target, access);

	RESTORE_METRICS_OP ();

	return ret;
}

void *
glMapBufferRange (GLenum target, GLintptr offset, GLsizeiptr length,
		  GLbitfield access)
{
	void *ret;

	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_BUFFER_DATA);

	GLWRAP_DEFER_WITH_RETURN (ret, glMapBufferRange, target, offset,
				  length, access);

	RESTORE_METRICS_OP ();

	return ret;
}

GLboolean
glUnmapBuffer (GLenum target)
{
	GLboolean ret;

	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_BUFFER_DATA);

	GLWRAP_DEFER_WITH_RETURN (ret, glUnmapBuffer, target);

	RESTORE_METRICS_OP ();

	return ret;
}

GLboolean
glUnmapNamedBufferEXT (GLuint buffer)
{
	GLboolean ret;

	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_BUFFER_DATA);

	GLWRAP_DEFER_WITH_RETURN (ret, glUnmapNamedBufferEXT, buffer);

	RESTORE_METRICS_OP ();

	return ret;
}

GLboolean
glUnmapBufferARB (GLenum target)
{
	GLboolean ret;

	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_BUFFER_DATA);

	GLWRAP_DEFER_WITH_RETURN (ret, glUnmapBufferARB, target);

	RESTORE_METRICS_OP ();

	return ret;
}

void
glFlushMappedBufferRange (GLenum target, GLintptr offset, GLsizeiptr length)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_BUFFER_DATA);

	GLWRAP_DEFER (glFlushMappedBufferRange, target, offset, length);

	RESTORE_METRICS_OP ();
}

void
glFlushMappedBufferRangeAPPLE (GLenum target, GLintptr offset, GLsizeiptr size)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_BUFFER_DATA);

	GLWRAP_DEFER (glFlushMappedBufferRangeAPPLE, target, offset, size);

	RESTORE_METRICS_OP ();
}

void
glFlushMappedNamedBufferRangeEXT (GLuint buffer, GLintptr offset,
				  GLsizeiptr length)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_BUFFER_DATA);

	GLWRAP_DEFER (glFlushMappedNamedBufferRangeEXT, buffer, offset, length);

	RESTORE_METRICS_OP ();
}

void *
glMapNamedBufferEXT (GLuint buffer, GLenum access)
{
	void *ret;

	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_BUFFER_DATA);

	GLWRAP_DEFER_WITH_RETURN (ret, glMapNamedBufferEXT, buffer, access);

	RESTORE_METRICS_OP ();

	return ret;
}

void *
glMapNamedBufferRangeEXT (GLuint buffer, GLintptr offset, GLsizeiptr length,
			  GLbitfield access)
{
	void *ret;

	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_BUFFER_DATA);

	GLWRAP_DEFER_WITH_RETURN (ret, glMapNamedBufferRangeEXT, buffer,
				  offset, length, access);

	RESTORE_METRICS_OP ();

	return ret;
}

/* METRICS_OP_BUFFER_SUB_DATA */
void
glCopyBufferSubData (GLenum readTarget, GLenum writeTarget,
		     GLintptr readOffset, GLintptr writeOffset,
		     GLsizeiptr size)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_BUFFER_SUB_DATA);

	GLWRAP_DEFER (glCopyBufferSubData, readTarget, writeTarget,
		      readOffset, writeOffset, size);

	RESTORE_METRICS_OP ();
}

void
glNamedCopyBufferSubDataEXT (GLuint readBuffer, GLuint writeBuffer,
			     GLintptr readOffset, GLintptr writeOffset,
			     GLsizeiptr size)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_BUFFER_SUB_DATA);

	GLWRAP_DEFER (glNamedCopyBufferSubDataEXT, readBuffer,
		      writeBuffer, readOffset, writeOffset, size);

	RESTORE_METRICS_OP ();
}

/* METRICS_OP_BITMAP */
void
glBitmap (GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig,
	  GLfloat xmove, GLfloat ymove, const GLubyte *bitmap)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_BITMAP);

	GLWRAP_DEFER (glBitmap, width, height, xorig, yorig,
		      xmove, ymove, bitmap);

	RESTORE_METRICS_OP ();
}

void
glBitmapxOES (GLsizei width, GLsizei height, GLfixed xorig, GLfixed yorig,
	      GLfixed xmove, GLfixed ymove, const GLubyte *bitmap)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_BITMAP);

	GLWRAP_DEFER (glBitmapxOES, width, height, xorig, yorig,
		      xmove, ymove, bitmap);

	RESTORE_METRICS_OP ();
}

/* METRICS_OP_BLIT_FRAMEBUFFER */
void
glBlitFramebuffer (GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1,
		   GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1,
		   GLbitfield mask, GLenum filter)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_BLIT_FRAMEBUFFER);

	GLWRAP_DEFER (glBlitFramebuffer, srcX0, srcY0, srcX1, srcY1,
		      dstX0, dstY0, dstX1, dstY1, mask, filter);

	RESTORE_METRICS_OP ();
}

void
glBlitFramebufferEXT (GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1,
		      GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1,
		      GLbitfield mask, GLenum filter)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_BLIT_FRAMEBUFFER);

	GLWRAP_DEFER (glBlitFramebufferEXT, srcX0, srcY0, srcX1, srcY1,
		      dstX0, dstY0, dstX1, dstY1, mask, filter);

	RESTORE_METRICS_OP ();
}

/* METRICS_OP_CLEAR */
void 
glClear (GLbitfield mask)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_CLEAR);

	GLWRAP_DEFER (glClear, mask);

	RESTORE_METRICS_OP ();
}

void
glClearBufferfi (GLenum buffer, GLint drawbuffer, GLfloat depth, GLint stencil)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_CLEAR);

	GLWRAP_DEFER (glClearBufferfi, buffer, drawbuffer, depth, stencil);

	RESTORE_METRICS_OP ();
}

void
glClearBufferfv (GLenum buffer, GLint drawbuffer, const GLfloat *value)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_CLEAR);

	GLWRAP_DEFER (glClearBufferfv, buffer, drawbuffer, value);

	RESTORE_METRICS_OP ();
}

void
glClearBufferiv (GLenum buffer, GLint drawbuffer, const GLint *value)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_CLEAR);

	GLWRAP_DEFER (glClearBufferiv, buffer, drawbuffer, value);

	RESTORE_METRICS_OP ();
}

void
glClearBufferuiv (GLenum buffer, GLint drawbuffer, const GLuint *value)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_CLEAR);

	GLWRAP_DEFER (glClearBufferuiv, buffer, drawbuffer, value);

	RESTORE_METRICS_OP ();
}

/* METRICS_OP_CLEAR_BUFFER_DATA */

void
glClearBufferData (GLenum target, GLenum internalformat, GLenum format,
		   GLenum type, const void *data)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_CLEAR_BUFFER_DATA);

	GLWRAP_DEFER (glClearBufferData, target, internalformat, format,
		      type, data);

	RESTORE_METRICS_OP ();
}

void
glClearBufferSubData (GLenum target, GLenum internalformat, GLintptr offset,
		      GLsizeiptr size, GLenum format, GLenum type,
		      const void *data)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_CLEAR_BUFFER_DATA);

	GLWRAP_DEFER (glClearBufferSubData, target, internalformat,
		      offset, size, format, type, data);

	RESTORE_METRICS_OP ();
}

void
glClearNamedBufferDataEXT (GLuint buffer, GLenum internalformat, GLenum format,
			   GLenum type, const void *data)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_CLEAR_BUFFER_DATA);

	GLWRAP_DEFER (glClearNamedBufferDataEXT, buffer, internalformat,
		      format, type, data);

	RESTORE_METRICS_OP ();
}

void
glClearNamedBufferSubDataEXT (GLuint buffer, GLenum internalformat,
			      GLenum format, GLenum type, GLsizeiptr offset,
			      GLsizeiptr size, const void *data)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_CLEAR_BUFFER_DATA);

	GLWRAP_DEFER (glClearNamedBufferSubDataEXT, buffer,
		      internalformat, format, type, offset, size, data);

	RESTORE_METRICS_OP ();
}

/* METRICS_OP_CLEAR_TEX_IMAGE */


/* METRICS_OP_COPY_PIXELS */
void
glCopyPixels (GLint x, GLint y, GLsizei width, GLsizei height, GLenum type )
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_COPY_PIXELS);

	GLWRAP_DEFER (glCopyPixels, x, y, width, height, type);

	RESTORE_METRICS_OP ();
}

/* METRICS_OP_COPY_TEX_IMAGE */
void
glCopyTexImage1D (GLenum target, GLint level, GLenum internalformat,
		  GLint x, GLint y, GLsizei width, GLint border)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_COPY_TEX_IMAGE);

	GLWRAP_DEFER (glCopyTexImage1D, target, level, internalformat,
		      x, y, width, border);

	RESTORE_METRICS_OP ();
}

void
glCopyTexImage1DEXT (GLenum target, GLint level, GLenum internalformat,
		     GLint x, GLint y, GLsizei width, GLint border)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_COPY_TEX_IMAGE);

	GLWRAP_DEFER (glCopyTexImage1DEXT, target, level, internalformat,
		      x, y, width, border);

	RESTORE_METRICS_OP ();
}

void
glCopyTexImage2D (GLenum target, GLint level, GLenum internalformat,
		  GLint x, GLint y, GLsizei width, GLsizei height,
		  GLint border)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_COPY_TEX_IMAGE);

	GLWRAP_DEFER (glCopyTexImage2D, target, level, internalformat,
		      x, y, width, height, border);

	RESTORE_METRICS_OP ();
}

void
glCopyTexImage2DEXT (GLenum target, GLint level, GLenum internalformat,
		     GLint x, GLint y, GLsizei width, GLsizei height,
		     GLint border)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_COPY_TEX_IMAGE);

	GLWRAP_DEFER (glCopyTexImage2DEXT, target, level, internalformat,
		      x, y, width, height, border);

	RESTORE_METRICS_OP ();
}

void
glCopyTexSubImage1D (GLenum target, GLint level, GLint xoffset,
		     GLint x, GLint y, GLsizei width)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_COPY_TEX_IMAGE);

	GLWRAP_DEFER (glCopyTexSubImage1D, target, level, xoffset,
		      x, y, width);

	RESTORE_METRICS_OP ();
}

void
glCopyTexSubImage1DEXT (GLenum target, GLint level, GLint xoffset,
			GLint x, GLint y, GLsizei width)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_COPY_TEX_IMAGE);

	GLWRAP_DEFER (glCopyTexSubImage1DEXT, target, level, xoffset,
		      x, y, width);

	RESTORE_METRICS_OP ();
}

void
glCopyTexSubImage2D (GLenum target, GLint level, GLint xoffset,
		     GLint yoffset, GLint x, GLint y, GLsizei width,
		     GLsizei height)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_COPY_TEX_IMAGE);

	GLWRAP_DEFER (glCopyTexSubImage2D, target, level, xoffset, yoffset,
		      x, y, width, height);

	RESTORE_METRICS_OP ();
}

void
glCopyTexSubImage2DEXT (GLenum target, GLint level, GLint xoffset,
			GLint yoffset, GLint x, GLint y, GLsizei width,
			GLsizei height)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_COPY_TEX_IMAGE);

	GLWRAP_DEFER (glCopyTexSubImage2DEXT, target, level, xoffset, yoffset,
		      x, y, width, height);

	RESTORE_METRICS_OP ();
}

void
glCopyTexSubImage3D (GLenum target, GLint level, GLint xoffset,
		     GLint yoffset, GLint zoffset, GLint x, GLint y,
		     GLsizei width, GLsizei height)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_COPY_TEX_IMAGE);

	GLWRAP_DEFER (glCopyTexSubImage3D, target, level, xoffset, yoffset,
		      zoffset, x, y, width, height);

	RESTORE_METRICS_OP ();
}

void
glCopyTexSubImage3DEXT (GLenum target, GLint level, GLint xoffset,
			GLint yoffset, GLint zoffset, GLint x, GLint y,
			GLsizei width, GLsizei height)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_COPY_TEX_IMAGE);

	GLWRAP_DEFER (glCopyTexSubImage3DEXT, target, level, xoffset, yoffset,
		      zoffset, x, y, width, height);

	RESTORE_METRICS_OP ();
}

void
glCopyTextureImage1DEXT (GLuint texture, GLenum target, GLint level,
			 GLenum internalformat, GLint x, GLint y,
			 GLsizei width, GLint border)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_COPY_TEX_IMAGE);

	GLWRAP_DEFER (glCopyTextureImage1DEXT, texture, target, level,
		      internalformat, x, y, width, border);

	RESTORE_METRICS_OP ();
}

void
glCopyTextureImage2DEXT (GLuint texture, GLenum target, GLint level,
			 GLenum internalformat, GLint x, GLint y, GLsizei width,
			 GLsizei height, GLint border)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_COPY_TEX_IMAGE);

	GLWRAP_DEFER (glCopyTextureImage2DEXT, texture, target,
		      level, internalformat, x, y, width, height, border);

	RESTORE_METRICS_OP ();
}

void
glCopyTextureSubImage1DEXT (GLuint texture, GLenum target, GLint level,
			    GLint xoffset, GLint x, GLint y, GLsizei width)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_COPY_TEX_IMAGE);

	GLWRAP_DEFER (glCopyTextureSubImage1DEXT, texture, target, level,
		      xoffset, x, y, width);

	RESTORE_METRICS_OP ();
}

void
glCopyTextureSubImage2DEXT (GLuint texture, GLenum target, GLint level,
			    GLint xoffset, GLint yoffset, GLint x, GLint y,
			    GLsizei width, GLsizei height)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_COPY_TEX_IMAGE);

	GLWRAP_DEFER (glCopyTextureSubImage2DEXT, texture, target, level,
		      xoffset, yoffset, x, y, width, height);

	RESTORE_METRICS_OP ();
}

void
glCopyTextureSubImage3DEXT (GLuint texture, GLenum target, GLint level,
			    GLint xoffset, GLint yoffset, GLint zoffset,
			    GLint x, GLint y, GLsizei width, GLsizei height)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_COPY_TEX_IMAGE);

	GLWRAP_DEFER (glCopyTextureSubImage3DEXT, texture, target, level,
		      xoffset, yoffset, zoffset, x, y, width, height);

	RESTORE_METRICS_OP ();
}

void
glCopyMultiTexImage1DEXT (GLenum texunit, GLenum target, GLint level,
			  GLenum internalformat, GLint x, GLint y,
			  GLsizei width, GLint border)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_COPY_TEX_IMAGE);

	GLWRAP_DEFER (glCopyMultiTexImage1DEXT, texunit, target, level,
		      internalformat, x, y, width, border);

	RESTORE_METRICS_OP ();
}

void
glCopyMultiTexImage2DEXT (GLenum texunit, GLenum target, GLint level,
			  GLenum internalformat, GLint x, GLint y,
			  GLsizei width, GLsizei height, GLint border)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_COPY_TEX_IMAGE);

	GLWRAP_DEFER (glCopyMultiTexImage2DEXT, texunit, target, level,
		      internalformat, x, y, width, height, border);

	RESTORE_METRICS_OP ();
}

void
glCopyMultiTexSubImage1DEXT (GLenum texunit, GLenum target, GLint level,
			     GLint xoffset, GLint x, GLint y, GLsizei width)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_COPY_TEX_IMAGE);

	GLWRAP_DEFER (glCopyMultiTexSubImage1DEXT, texunit, target, level,
		      xoffset, x, y, width);

	RESTORE_METRICS_OP ();
}

void
glCopyMultiTexSubImage2DEXT (GLenum texunit, GLenum target, GLint level,
			     GLint xoffset, GLint yoffset, GLint x, GLint y,
			     GLsizei width, GLsizei height)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_COPY_TEX_IMAGE);

	GLWRAP_DEFER (glCopyMultiTexSubImage2DEXT, texunit, target, level,
		      xoffset, yoffset, x, y, width, height);

	RESTORE_METRICS_OP ();
}

void
glCopyMultiTexSubImage3DEXT (GLenum texunit, GLenum target, GLint level,
			     GLint xoffset, GLint yoffset, GLint zoffset,
			     GLint x, GLint y, GLsizei width, GLsizei height)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_COPY_TEX_IMAGE);

	GLWRAP_DEFER (glCopyMultiTexSubImage3DEXT, texunit, target, level,
		      xoffset, yoffset, zoffset, x, y, width, height);

	RESTORE_METRICS_OP ();
}

/* METRICS_OP_DRAW_PIXELS */
void
glDrawPixels (GLsizei width, GLsizei height, GLenum format, GLenum type,
	      const GLvoid *pixels)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_DRAW_PIXELS);

	GLWRAP_DEFER (glDrawPixels, width, height, format, type, pixels);

	RESTORE_METRICS_OP ();
}

/* METRICS_OP_GET_TEX_IMAGE */

void
glGetCompressedMultiTexImageEXT (GLenum texunit, GLenum target,
				 GLint lod, GLvoid *img)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_GET_TEX_IMAGE);

	GLWRAP_DEFER (glGetCompressedMultiTexImageEXT, texunit,
		      target, lod, img);

	RESTORE_METRICS_OP ();
}

void
glGetCompressedTexImage (GLenum target, GLint level, GLvoid *img)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_GET_TEX_IMAGE);

	GLWRAP_DEFER (glGetCompressedTexImage, target, level, img);

	RESTORE_METRICS_OP ();
}

void
glGetCompressedTexImageARB (GLenum target, GLint level, GLvoid *img)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_GET_TEX_IMAGE);

	GLWRAP_DEFER (glGetCompressedTexImageARB, target, level, img);

	RESTORE_METRICS_OP ();
}

void
glGetCompressedTextureImageEXT (GLuint texture, GLenum target,
				GLint lod, GLvoid *img)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_GET_TEX_IMAGE);

	GLWRAP_DEFER (glGetCompressedTextureImageEXT, texture,
		      target, lod, img);

	RESTORE_METRICS_OP ();
}

void
glGetMultiTexImageEXT (GLenum texunit, GLenum target, GLint level,
		       GLenum format, GLenum type, GLvoid *pixels)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_GET_TEX_IMAGE);

	GLWRAP_DEFER (glGetMultiTexImageEXT, texunit,
		      target, level, format, type, pixels);

	RESTORE_METRICS_OP ();
}

void
glGetnCompressedTexImageARB (GLenum target, GLint lod,
			     GLsizei bufSize, GLvoid *img)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_GET_TEX_IMAGE);

	GLWRAP_DEFER (glGetnCompressedTexImageARB, target, lod, bufSize, img);

	RESTORE_METRICS_OP ();
}

void
glGetnTexImageARB (GLenum target, GLint level, GLenum format,
		   GLenum type, GLsizei bufSize, GLvoid *img)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_GET_TEX_IMAGE);

	GLWRAP_DEFER (glGetnTexImageARB, target, level,
		      format, type, bufSize, img);

	RESTORE_METRICS_OP ();
}

void
glGetTexImage (GLenum target, GLint level, GLenum format, GLenum type,
	       GLvoid *pixels )
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_GET_TEX_IMAGE);

	GLWRAP_DEFER (glGetTexImage, target, level, format, type, pixels);

	RESTORE_METRICS_OP ();
}

/* METRICS_OP_READ_PIXELS */
void
glReadPixels (GLint x, GLint y, GLsizei width, GLsizei height,
	      GLenum format, GLenum type, GLvoid *pixels )
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_READ_PIXELS);

	GLWRAP_DEFER (glReadPixels, x, y, width, height, format, type, pixels);

	RESTORE_METRICS_OP ();
}

void
glReadnPixelsARB (GLint x, GLint y, GLsizei width, GLsizei height,
		  GLenum format, GLenum type, GLsizei bufSize, GLvoid *data)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_READ_PIXELS);

	GLWRAP_DEFER (glReadnPixelsARB, x, y, width, height,
		      format, type, bufSize, data);

	RESTORE_METRICS_OP ();
}

/* METRICS_OP_TEX_IMAGE */
void
glTexImage1D (GLenum target, GLint level, GLint internalFormat,
	      GLsizei width, GLint border, GLenum format, GLenum type,
	      const GLvoid *pixels)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_TEX_IMAGE);

	GLWRAP_DEFER (glTexImage1D, target, level, internalFormat, width,
		      border, format, type, pixels);

	RESTORE_METRICS_OP ();
}

void
glTexImage2D (GLenum target, GLint level, GLint internalFormat,
	      GLsizei width, GLsizei height, GLint border, GLenum format,
	      GLenum type, const GLvoid *pixels )
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_TEX_IMAGE);

	GLWRAP_DEFER (glTexImage2D, target, level, internalFormat,
		      width, height, border, format, type, pixels);

	RESTORE_METRICS_OP ();
}


void
glTexImage2DMultisample (GLenum target, GLsizei samples,
			 GLint_or_enum internalformat,
			 GLsizei width, GLsizei height,
			 GLboolean fixedsamplelocations)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_TEX_IMAGE);

	GLWRAP_DEFER (glTexImage2DMultisample, target, samples,
		      internalformat, width, height, fixedsamplelocations);

	RESTORE_METRICS_OP ();
}

void
glTexImage2DMultisampleCoverageNV (GLenum target, GLsizei coverageSamples,
				   GLsizei colorSamples, GLint internalFormat,
				   GLsizei width, GLsizei height,
				   GLboolean fixedSampleLocations)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_TEX_IMAGE);

	GLWRAP_DEFER (glTexImage2DMultisampleCoverageNV, target,
		      coverageSamples, colorSamples, internalFormat,
		      width, height, fixedSampleLocations);

	RESTORE_METRICS_OP ();
}

void
glTexImage3D (GLenum target, GLint level, GLint internalformat,
	      GLsizei width, GLsizei height, GLsizei depth, GLint border,
	      GLenum format, GLenum type, const GLvoid *pixels)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_TEX_IMAGE);

	GLWRAP_DEFER (glTexImage3D, target, level, internalformat,
		      width, height, depth, border, format, type, pixels);

	RESTORE_METRICS_OP ();
}

void
glTexImage3DEXT (GLenum target, GLint level, GLenum internalformat,
		 GLsizei width, GLsizei height, GLsizei depth, GLint border,
		 GLenum format, GLenum type, const GLvoid *pixels)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_TEX_IMAGE);

	GLWRAP_DEFER (glTexImage3DEXT, target, level, internalformat,
		      width, height, depth, border, format, type, pixels);

	RESTORE_METRICS_OP ();
}

void
glTexImage3DMultisample (GLenum target, GLsizei samples,
			 GLint_or_enum internalformat,
			 GLsizei width, GLsizei height, GLsizei depth,
			 GLboolean fixedsamplelocations)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_TEX_IMAGE);

	GLWRAP_DEFER (glTexImage3DMultisample, target, samples,
		      internalformat, width, height, depth,
		      fixedsamplelocations);

	RESTORE_METRICS_OP ();
}

void
glTexImage3DMultisampleCoverageNV (GLenum target, GLsizei coverageSamples,
				   GLsizei colorSamples, GLint internalFormat,
				   GLsizei width, GLsizei height, GLsizei depth,
				   GLboolean fixedSampleLocations)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_TEX_IMAGE);

	GLWRAP_DEFER (glTexImage3DMultisampleCoverageNV, target,
		      coverageSamples, colorSamples, internalFormat,
		      width, height, depth, fixedSampleLocations);

	RESTORE_METRICS_OP ();
}

void
glTexImage4DSGIS (GLenum target, GLint level, GLenum internalformat,
		  GLsizei width, GLsizei height, GLsizei depth,
		  GLsizei size4d, GLint border, GLenum format,
		  GLenum type, const GLvoid *pixels)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_TEX_IMAGE);

	GLWRAP_DEFER (glTexImage4DSGIS, target, level,
		      internalformat, width, height, depth,
		      size4d, border, format, type, pixels);

	RESTORE_METRICS_OP ();
}

void
glTexSubImage1D (GLenum target, GLint level, GLint xoffset,
		 GLsizei width, GLenum format, GLenum type,
		 const GLvoid *pixels)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_TEX_IMAGE);

	GLWRAP_DEFER (glTexSubImage1D, target, level, xoffset,
		      width, format, type, pixels);

	RESTORE_METRICS_OP ();
}

void
glTexSubImage1DEXT (GLenum target, GLint level, GLint xoffset,
		    GLsizei width, GLenum format, GLenum type,
		    const GLvoid *pixels)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_TEX_IMAGE);

	GLWRAP_DEFER (glTexSubImage1DEXT, target, level, xoffset,
		      width, format, type, pixels);

	RESTORE_METRICS_OP ();
}

void
glTexSubImage2D (GLenum target, GLint level, GLint xoffset, GLint yoffset,
		 GLsizei width, GLsizei height, GLenum format, GLenum type,
		 const GLvoid *pixels)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_TEX_IMAGE);

	GLWRAP_DEFER (glTexSubImage2D, target, level, xoffset, yoffset,
		      width, height, format, type, pixels);

	RESTORE_METRICS_OP ();
}

void
glTexSubImage2DEXT (GLenum target, GLint level, GLint xoffset, GLint yoffset,
		    GLsizei width, GLsizei height, GLenum format, GLenum type,
		    const GLvoid *pixels)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_TEX_IMAGE);

	GLWRAP_DEFER (glTexSubImage2DEXT, target, level, xoffset, yoffset,
		      width, height, format, type, pixels);

	RESTORE_METRICS_OP ();
}

void
glTexSubImage3D (GLenum target, GLint level, GLint xoffset, GLint yoffset,
		 GLint zoffset, GLsizei width, GLsizei height, GLsizei depth,
		 GLenum format, GLenum type, const GLvoid *pixels)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_TEX_IMAGE);

	GLWRAP_DEFER (glTexSubImage3D, target, level, xoffset, yoffset,
		      zoffset, width, height, depth, format, type, pixels);

	RESTORE_METRICS_OP ();
}

void
glTexSubImage3DEXT (GLenum target, GLint level, GLint xoffset, GLint yoffset,
		    GLint zoffset, GLsizei width, GLsizei height, GLsizei depth,
		    GLenum format, GLenum type, const GLvoid *pixels)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_TEX_IMAGE);

	GLWRAP_DEFER (glTexSubImage3DEXT, target, level, xoffset, yoffset,
		      zoffset, width, height, depth, format, type, pixels);

	RESTORE_METRICS_OP ();
}

void
glTexSubImage4DSGIS (GLenum target, GLint level, GLint xoffset, GLint yoffset,
		     GLint zoffset, GLint woffset, GLsizei width,
		     GLsizei height, GLsizei depth, GLsizei size4d,
		     GLenum format, GLenum type, const GLvoid *pixels)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_TEX_IMAGE);

	GLWRAP_DEFER (glTexSubImage4DSGIS, target, level, xoffset,
		      yoffset, zoffset, woffset, width, height,
		      depth, size4d, format, type, pixels);

	RESTORE_METRICS_OP ();
}

void
glCompressedMultiTexImage1DEXT (GLenum texunit, GLenum target, GLint level,
				GLenum internalformat, GLsizei width,
				GLint border, GLsizei imageSize,
				const GLvoid *bits)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_TEX_IMAGE);

	GLWRAP_DEFER (glCompressedMultiTexImage1DEXT, texunit, target,
		      level, internalformat, width, border, imageSize, bits);

	RESTORE_METRICS_OP ();
}

void
glCompressedMultiTexImage2DEXT (GLenum texunit, GLenum target, GLint level,
				GLenum internalformat, GLsizei width,
				GLsizei height, GLint border,
				GLsizei imageSize, const GLvoid *bits)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_TEX_IMAGE);

	GLWRAP_DEFER (glCompressedMultiTexImage2DEXT, texunit, target, level,
		      internalformat, width, height, border, imageSize, bits);

	RESTORE_METRICS_OP ();
}

void
glCompressedMultiTexImage3DEXT (GLenum texunit, GLenum target, GLint level,
				GLenum internalformat, GLsizei width,
				GLsizei height, GLsizei depth, GLint border,
				GLsizei imageSize, const GLvoid *bits)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_TEX_IMAGE);

	GLWRAP_DEFER (glCompressedMultiTexImage3DEXT, texunit, target,
		      level, internalformat, width, height, depth,
		      border, imageSize, bits);

	RESTORE_METRICS_OP ();
}

void
glCompressedMultiTexSubImage1DEXT (GLenum texunit, GLenum target, GLint level,
				   GLint xoffset, GLsizei width, GLenum format,
				   GLsizei imageSize, const GLvoid *bits)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_TEX_IMAGE);

	GLWRAP_DEFER (glCompressedMultiTexSubImage1DEXT, texunit, target,
		      level, xoffset, width, format, imageSize, bits);

	RESTORE_METRICS_OP ();
}

void
glCompressedMultiTexSubImage2DEXT (GLenum texunit, GLenum target, GLint level,
				   GLint xoffset, GLint yoffset, GLsizei width,
				   GLsizei height, GLenum format,
				   GLsizei imageSize, const GLvoid *bits)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_TEX_IMAGE);

	GLWRAP_DEFER (glCompressedMultiTexSubImage2DEXT, texunit, target, level,
		      xoffset, yoffset, width, height, format, imageSize, bits);

	RESTORE_METRICS_OP ();
}

void
glCompressedMultiTexSubImage3DEXT (GLenum texunit, GLenum target, GLint level,
				   GLint xoffset, GLint yoffset, GLint zoffset,
				   GLsizei width, GLsizei height, GLsizei depth,
				   GLenum format, GLsizei imageSize,
				   const GLvoid *bits)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_TEX_IMAGE);

	GLWRAP_DEFER (glCompressedMultiTexSubImage3DEXT, texunit, target,
		      level, xoffset, yoffset, zoffset, width, height,
		      depth, format, imageSize, bits);

	RESTORE_METRICS_OP ();
}

void
glCompressedTexImage1D (GLenum target, GLint level, GLenum internalformat,
			GLsizei width, GLint border, GLsizei imageSize,
			const GLvoid *data)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_TEX_IMAGE);

	GLWRAP_DEFER (glCompressedTexImage1D, target, level,
		      internalformat, width, border, imageSize, data);

	RESTORE_METRICS_OP ();
}

void
glCompressedTexImage1DARB (GLenum target, GLint level, GLenum internalformat,
			   GLsizei width, GLint border, GLsizei imageSize,
			   const GLvoid *data)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_TEX_IMAGE);

	GLWRAP_DEFER (glCompressedTexImage1DARB, target, level, internalformat,
		      width, border, imageSize, data);

	RESTORE_METRICS_OP ();
}

void
glCompressedTexImage2D (GLenum target, GLint level, GLenum internalformat,
			GLsizei width, GLsizei height, GLint border,
			GLsizei imageSize, const GLvoid *data)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_TEX_IMAGE);

	GLWRAP_DEFER (glCompressedTexImage2D, target, level, internalformat,
		      width, height, border, imageSize, data);

	RESTORE_METRICS_OP ();
}

void
glCompressedTexImage2DARB (GLenum target, GLint level, GLenum internalformat,
			   GLsizei width, GLsizei height, GLint border,
			   GLsizei imageSize, const GLvoid *data)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_TEX_IMAGE);

	GLWRAP_DEFER (glCompressedTexImage2DARB, target, level, internalformat,
		      width, height, border, imageSize, data);

	RESTORE_METRICS_OP ();
}

void
glCompressedTexImage3D (GLenum target, GLint level, GLenum internalformat,
			GLsizei width, GLsizei height, GLsizei depth,
			GLint border, GLsizei imageSize, const GLvoid *data)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_TEX_IMAGE);

	GLWRAP_DEFER (glCompressedTexImage3D, target, level, internalformat,
		      width, height, depth, border, imageSize, data);

	RESTORE_METRICS_OP ();
}

void
glCompressedTexImage3DARB (GLenum target, GLint level, GLenum internalformat,
			   GLsizei width, GLsizei height, GLsizei depth,
			   GLint border, GLsizei imageSize, const GLvoid *data)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_TEX_IMAGE);

	GLWRAP_DEFER (glCompressedTexImage3DARB, target, level, internalformat,
		      width, height, depth, border, imageSize, data);

	RESTORE_METRICS_OP ();
}

void
glCompressedTexSubImage1D (GLenum target, GLint level, GLint xoffset,
			   GLsizei width, GLenum format, GLsizei imageSize,
			   const GLvoid *data)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_TEX_IMAGE);

	GLWRAP_DEFER (glCompressedTexSubImage1D, target, level, xoffset,
		      width, format, imageSize, data);

	RESTORE_METRICS_OP ();
}

void
glCompressedTexSubImage1DARB (GLenum target, GLint level, GLint xoffset,
			      GLsizei width, GLenum format, GLsizei imageSize,
			      const GLvoid *data)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_TEX_IMAGE);

	GLWRAP_DEFER (glCompressedTexSubImage1DARB, target, level, xoffset,
		      width, format, imageSize, data);

	RESTORE_METRICS_OP ();
}

void
glCompressedTexSubImage2D (GLenum target, GLint level, GLint xoffset,
			   GLint yoffset, GLsizei width, GLsizei height,
			   GLenum format, GLsizei imageSize, const GLvoid *data)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_TEX_IMAGE);

	GLWRAP_DEFER (glCompressedTexSubImage2D, target, level, xoffset,
		      yoffset, width, height, format, imageSize, data);

	RESTORE_METRICS_OP ();
}

void
glCompressedTexSubImage2DARB (GLenum target, GLint level, GLint xoffset,
			      GLint yoffset, GLsizei width, GLsizei height,
			      GLenum format, GLsizei imageSize,
			      const GLvoid *data)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_TEX_IMAGE);

	GLWRAP_DEFER (glCompressedTexSubImage2DARB, target, level, xoffset,
		      yoffset, width, height, format, imageSize, data);

	RESTORE_METRICS_OP ();
}

void
glCompressedTexSubImage3D (GLenum target, GLint level, GLint xoffset,
			   GLint yoffset, GLint zoffset, GLsizei width,
			   GLsizei height, GLsizei depth, GLenum format,
			   GLsizei imageSize, const GLvoid *data)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_TEX_IMAGE);

	GLWRAP_DEFER (glCompressedTexSubImage3D, target, level, xoffset,
		      yoffset, zoffset, width, height, depth, format,
		      imageSize, data);

	RESTORE_METRICS_OP ();
}

void
glCompressedTexSubImage3DARB (GLenum target, GLint level, GLint xoffset,
			      GLint yoffset, GLint zoffset, GLsizei width,
			      GLsizei height, GLsizei depth, GLenum format,
			      GLsizei imageSize, const GLvoid *data)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_TEX_IMAGE);

	GLWRAP_DEFER (glCompressedTexSubImage3DARB, target, level, xoffset,
		      yoffset, zoffset, width, height, depth, format,
		      imageSize, data);

	RESTORE_METRICS_OP ();
}

void
glCompressedTextureImage1DEXT (GLuint texture, GLenum target, GLint level,
			       GLenum internalformat, GLsizei width,
			       GLint border, GLsizei imageSize,
			       const GLvoid *bits)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_TEX_IMAGE);

	GLWRAP_DEFER (glCompressedTextureImage1DEXT, texture, target, level,
		      internalformat, width, border, imageSize, bits);

	RESTORE_METRICS_OP ();
}

void
glCompressedTextureImage2DEXT (GLuint texture, GLenum target, GLint level,
			       GLenum internalformat, GLsizei width,
			       GLsizei height, GLint border,
			       GLsizei imageSize, const GLvoid *bits)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_TEX_IMAGE);

	GLWRAP_DEFER (glCompressedTextureImage2DEXT, texture, target, level,
		      internalformat, width, height, border, imageSize, bits);

	RESTORE_METRICS_OP ();
}

void
glCompressedTextureImage3DEXT (GLuint texture, GLenum target, GLint level,
			       GLenum internalformat, GLsizei width,
			       GLsizei height, GLsizei depth, GLint border,
			       GLsizei imageSize, const GLvoid *bits)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_TEX_IMAGE);

	GLWRAP_DEFER (glCompressedTextureImage3DEXT, texture, target,
		      level, internalformat, width, height, depth,
		      border, imageSize, bits);

	RESTORE_METRICS_OP ();
}

void
glCompressedTextureSubImage1DEXT (GLuint texture, GLenum target, GLint level,
				  GLint xoffset, GLsizei width, GLenum format,
				  GLsizei imageSize, const GLvoid *bits)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_TEX_IMAGE);

	GLWRAP_DEFER (glCompressedTextureSubImage1DEXT, texture, target,
		      level, xoffset, width, format, imageSize, bits);

	RESTORE_METRICS_OP ();
}

void
glCompressedTextureSubImage2DEXT (GLuint texture, GLenum target, GLint level,
				  GLint xoffset, GLint yoffset, GLsizei width,
				  GLsizei height, GLenum format,
				  GLsizei imageSize, const GLvoid *bits)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_TEX_IMAGE);

	GLWRAP_DEFER (glCompressedTextureSubImage2DEXT, texture, target, level,
		      xoffset, yoffset, width, height, format, imageSize, bits);

	RESTORE_METRICS_OP ();
}

void
glCompressedTextureSubImage3DEXT (GLuint texture, GLenum target, GLint level,
				  GLint xoffset, GLint yoffset, GLint zoffset,
				  GLsizei width, GLsizei height, GLsizei depth,
				  GLenum format, GLsizei imageSize,
				  const GLvoid *bits)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_TEX_IMAGE);

	GLWRAP_DEFER (glCompressedTextureSubImage3DEXT, texture, target,
		      level, xoffset, yoffset, zoffset, width, height,
		      depth, format, imageSize, bits);

	RESTORE_METRICS_OP ();
}

void
glMultiTexImage1DEXT (GLenum texunit, GLenum target, GLint level,
		      GLenum_or_int internalformat,
		      GLsizei width, GLint border,
		      GLenum format, GLenum type, const GLvoid *pixels)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_TEX_IMAGE);

	GLWRAP_DEFER (glMultiTexImage1DEXT, texunit, target, level,
		      internalformat, width, border, format, type, pixels);

	RESTORE_METRICS_OP ();
}

void
glMultiTexImage2DEXT (GLenum texunit, GLenum target, GLint level,
		      GLenum_or_int internalformat,
		      GLsizei width, GLsizei height,
		      GLint border, GLenum format, GLenum type,
		      const GLvoid *pixels)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_TEX_IMAGE);

	GLWRAP_DEFER (glMultiTexImage2DEXT, texunit, target, level,
		      internalformat, width, height, border, format,
		      type, pixels);

	RESTORE_METRICS_OP ();
}

void
glMultiTexImage3DEXT (GLenum texunit, GLenum target, GLint level,
		      GLenum_or_int internalformat,
		      GLsizei width, GLsizei height,
		      GLsizei depth, GLint border, GLenum format,
		      GLenum type, const GLvoid *pixels)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_TEX_IMAGE);

	GLWRAP_DEFER (glMultiTexImage3DEXT, texunit, target, level,
		      internalformat, width, height, depth, border,
		      format, type, pixels);

	RESTORE_METRICS_OP ();
}

void
glMultiTexSubImage1DEXT (GLenum texunit, GLenum target, GLint level,
			 GLint xoffset, GLsizei width, GLenum format,
			 GLenum type, const GLvoid *pixels)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_TEX_IMAGE);

	GLWRAP_DEFER (glMultiTexSubImage1DEXT, texunit, target, level,
		      xoffset, width, format, type, pixels);

	RESTORE_METRICS_OP ();
}

void
glMultiTexSubImage2DEXT (GLenum texunit, GLenum target, GLint level,
			 GLint xoffset, GLint yoffset, GLsizei width,
			 GLsizei height, GLenum format, GLenum type,
			 const GLvoid *pixels)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_TEX_IMAGE);

	GLWRAP_DEFER (glMultiTexSubImage2DEXT, texunit, target, level, xoffset,
		      yoffset, width, height, format, type, pixels);

	RESTORE_METRICS_OP ();
}

void
glMultiTexSubImage3DEXT (GLenum texunit, GLenum target, GLint level,
			 GLint xoffset, GLint yoffset, GLint zoffset,
			 GLsizei width, GLsizei height, GLsizei depth,
			 GLenum format, GLenum type, const GLvoid *pixels)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_TEX_IMAGE);

	GLWRAP_DEFER (glMultiTexSubImage3DEXT, texunit, target, level,
		      xoffset, yoffset, zoffset, width, height, depth,
		      format, type, pixels);

	RESTORE_METRICS_OP ();
}
