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

#include <dlfcn.h>

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
#include "publish.h"

/* The first appearance of the GLfixed datatype in Mesa was with
 * glext.h of version 20130624. So we'll assume that any older glext.h
 * cannot have any function accepting a GLfixed parameter. */
#if GL_GLEXT_VERSION >= 20130624
#define HAVE_GLFIXED 1
#endif

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
#define RESTORE_METRICS_OP()				\
	SWITCH_METRICS_OP (save);

static void *
open_lib(char *env_name)
{
    void *lib_handle;
    const char *path = getenv (env_name);
        
		if (path == NULL) {
			fprintf (stderr, "fips: %s unset. Please set to path of "
                     "appropriate gl library.\n", env_name);
			exit (1);
		}

		lib_handle = dlopen (path, RTLD_LAZY | RTLD_GLOBAL);
		if (lib_handle == NULL) {
			fprintf (stderr, "fips_lookup: Error: Failed to dlopen %s\n", path);
			exit (1);
		}
        return lib_handle;
}

void *
fips_lookup (char *name)
{
	static void *libgl_handle = NULL, *libegl_handle = NULL;
	void *ret;

    if (libgl_handle == NULL)
    {
        libgl_handle = open_lib("FIPS_GL");
        libegl_handle = open_lib("FIPS_EGL");
	}
	ret = dlsym (libgl_handle, name);

	if (ret != NULL)
        return ret;

	ret = dlsym (libegl_handle, name);
	if (ret != NULL)
        return ret;

    fprintf (stderr, "Error: fips_lookup failed to dlsym %s\n",
			 name);
    exit (1);
}

/* With a program change, we stop the counter, update the
 * active program, then start the counter up again. */
void
glUseProgram (GLuint program)
{
	SWITCH_METRICS_OP (METRICS_OP_SHADER + program);

	FIPS_DEFER(glUseProgram, program);
}

void
glUseProgramObjectARB (GLhandleARB programObj)
{
	SWITCH_METRICS_OP (METRICS_OP_SHADER + programObj);

	FIPS_DEFER(glUseProgramObjectARB, programObj);
}

/* METRICS_OP_ACCUM */
void
glAccum (GLenum op, GLfloat value)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_ACCUM);

	FIPS_DEFER (glAccum, op, value);

	RESTORE_METRICS_OP ();
}

void
glClearAccum (GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_ACCUM);

	FIPS_DEFER (glClearAccum, red, green, blue, alpha);

	RESTORE_METRICS_OP ();
}

#if HAVE_GLFIXED
void
glClearAccumxOES (GLfixed red, GLfixed green, GLfixed blue, GLfixed alpha)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_ACCUM);

	FIPS_DEFER (glClearAccumxOES, red, green, blue, alpha);

	RESTORE_METRICS_OP ();
}
#endif

/* METRICS_OP_BUFFER_DATA */
void
glBufferData (GLenum target, GLsizeiptr size, const GLvoid *data, GLenum usage)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_BUFFER_DATA);

	FIPS_DEFER (glBufferData, target, size, data, usage);

	RESTORE_METRICS_OP ();
}

void
glNamedBufferDataEXT (GLuint buffer, GLsizeiptr size, const GLvoid *data,
		      GLenum usage)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_BUFFER_DATA);

	FIPS_DEFER (glNamedBufferDataEXT, buffer, size, data, usage);

	RESTORE_METRICS_OP ();
}

void
glBufferSubData (GLenum target, GLintptr offset, GLsizeiptr size,
		 const GLvoid *data)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_BUFFER_DATA);

	FIPS_DEFER (glBufferSubData, target, offset, size, data);

	RESTORE_METRICS_OP ();
}

void
glNamedBufferSubDataEXT (GLuint buffer, GLintptr offset, GLsizeiptr size,
			 const GLvoid *data)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_BUFFER_DATA);

	FIPS_DEFER (glNamedBufferSubDataEXT, buffer, offset, size, data);

	RESTORE_METRICS_OP ();
}

void *
glMapBuffer (GLenum target, GLenum access)
{
	void *ret;

	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_BUFFER_DATA);

	FIPS_DEFER_WITH_RETURN (ret, glMapBuffer, target, access);

	RESTORE_METRICS_OP ();

	return ret;
}

void *
glMapBufferARB (GLenum target, GLenum access)
{
	void *ret;

	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_BUFFER_DATA);

	FIPS_DEFER_WITH_RETURN (ret, glMapBufferARB, target, access);

	RESTORE_METRICS_OP ();

	return ret;
}

void *
glMapBufferRange (GLenum target, GLintptr offset, GLsizeiptr length,
		  GLbitfield access)
{
	void *ret;

	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_BUFFER_DATA);

	FIPS_DEFER_WITH_RETURN (ret, glMapBufferRange, target, offset,
				  length, access);

	RESTORE_METRICS_OP ();

	return ret;
}

GLboolean
glUnmapBuffer (GLenum target)
{
	GLboolean ret;

	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_BUFFER_DATA);

	FIPS_DEFER_WITH_RETURN (ret, glUnmapBuffer, target);

	RESTORE_METRICS_OP ();

	return ret;
}

GLboolean
glUnmapNamedBufferEXT (GLuint buffer)
{
	GLboolean ret;

	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_BUFFER_DATA);

	FIPS_DEFER_WITH_RETURN (ret, glUnmapNamedBufferEXT, buffer);

	RESTORE_METRICS_OP ();

	return ret;
}

GLboolean
glUnmapBufferARB (GLenum target)
{
	GLboolean ret;

	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_BUFFER_DATA);

	FIPS_DEFER_WITH_RETURN (ret, glUnmapBufferARB, target);

	RESTORE_METRICS_OP ();

	return ret;
}

void
glFlushMappedBufferRange (GLenum target, GLintptr offset, GLsizeiptr length)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_BUFFER_DATA);

	FIPS_DEFER (glFlushMappedBufferRange, target, offset, length);

	RESTORE_METRICS_OP ();
}

void
glFlushMappedBufferRangeAPPLE (GLenum target, GLintptr offset, GLsizeiptr size)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_BUFFER_DATA);

	FIPS_DEFER (glFlushMappedBufferRangeAPPLE, target, offset, size);

	RESTORE_METRICS_OP ();
}

void
glFlushMappedNamedBufferRangeEXT (GLuint buffer, GLintptr offset,
				  GLsizeiptr length)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_BUFFER_DATA);

	FIPS_DEFER (glFlushMappedNamedBufferRangeEXT, buffer, offset, length);

	RESTORE_METRICS_OP ();
}

void *
glMapNamedBufferEXT (GLuint buffer, GLenum access)
{
	void *ret;

	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_BUFFER_DATA);

	FIPS_DEFER_WITH_RETURN (ret, glMapNamedBufferEXT, buffer, access);

	RESTORE_METRICS_OP ();

	return ret;
}

void *
glMapNamedBufferRangeEXT (GLuint buffer, GLintptr offset, GLsizeiptr length,
			  GLbitfield access)
{
	void *ret;

	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_BUFFER_DATA);

	FIPS_DEFER_WITH_RETURN (ret, glMapNamedBufferRangeEXT, buffer,
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

	FIPS_DEFER (glCopyBufferSubData, readTarget, writeTarget,
		      readOffset, writeOffset, size);

	RESTORE_METRICS_OP ();
}

void
glNamedCopyBufferSubDataEXT (GLuint readBuffer, GLuint writeBuffer,
			     GLintptr readOffset, GLintptr writeOffset,
			     GLsizeiptr size)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_BUFFER_SUB_DATA);

	FIPS_DEFER (glNamedCopyBufferSubDataEXT, readBuffer,
		      writeBuffer, readOffset, writeOffset, size);

	RESTORE_METRICS_OP ();
}

/* METRICS_OP_BITMAP */
void
glBitmap (GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig,
	  GLfloat xmove, GLfloat ymove, const GLubyte *bitmap)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_BITMAP);

	FIPS_DEFER (glBitmap, width, height, xorig, yorig,
		      xmove, ymove, bitmap);

	RESTORE_METRICS_OP ();
}

#if HAVE_GLFIXED
void
glBitmapxOES (GLsizei width, GLsizei height, GLfixed xorig, GLfixed yorig,
	      GLfixed xmove, GLfixed ymove, const GLubyte *bitmap)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_BITMAP);

	FIPS_DEFER (glBitmapxOES, width, height, xorig, yorig,
		      xmove, ymove, bitmap);

	RESTORE_METRICS_OP ();
}
#endif

/* METRICS_OP_BLIT_FRAMEBUFFER */
void
glBlitFramebuffer (GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1,
		   GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1,
		   GLbitfield mask, GLenum filter)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_BLIT_FRAMEBUFFER);

	FIPS_DEFER (glBlitFramebuffer, srcX0, srcY0, srcX1, srcY1,
		      dstX0, dstY0, dstX1, dstY1, mask, filter);

	RESTORE_METRICS_OP ();
}

void
glBlitFramebufferEXT (GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1,
		      GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1,
		      GLbitfield mask, GLenum filter)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_BLIT_FRAMEBUFFER);

	FIPS_DEFER (glBlitFramebufferEXT, srcX0, srcY0, srcX1, srcY1,
		      dstX0, dstY0, dstX1, dstY1, mask, filter);

	RESTORE_METRICS_OP ();
}

/* METRICS_OP_CLEAR */
void 
glClear (GLbitfield mask)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_CLEAR);

	FIPS_DEFER (glClear, mask);

	RESTORE_METRICS_OP ();
}

void
glClearBufferfi (GLenum buffer, GLint drawbuffer, GLfloat depth, GLint stencil)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_CLEAR);

	FIPS_DEFER (glClearBufferfi, buffer, drawbuffer, depth, stencil);

	RESTORE_METRICS_OP ();
}

void
glClearBufferfv (GLenum buffer, GLint drawbuffer, const GLfloat *value)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_CLEAR);

	FIPS_DEFER (glClearBufferfv, buffer, drawbuffer, value);

	RESTORE_METRICS_OP ();
}

void
glClearBufferiv (GLenum buffer, GLint drawbuffer, const GLint *value)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_CLEAR);

	FIPS_DEFER (glClearBufferiv, buffer, drawbuffer, value);

	RESTORE_METRICS_OP ();
}

void
glClearBufferuiv (GLenum buffer, GLint drawbuffer, const GLuint *value)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_CLEAR);

	FIPS_DEFER (glClearBufferuiv, buffer, drawbuffer, value);

	RESTORE_METRICS_OP ();
}

/* METRICS_OP_CLEAR_BUFFER_DATA */

void
glClearBufferData (GLenum target, GLenum internalformat, GLenum format,
		   GLenum type, const void *data)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_CLEAR_BUFFER_DATA);

	FIPS_DEFER (glClearBufferData, target, internalformat, format,
		      type, data);

	RESTORE_METRICS_OP ();
}

void
glClearBufferSubData (GLenum target, GLenum internalformat, GLintptr offset,
		      GLsizeiptr size, GLenum format, GLenum type,
		      const void *data)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_CLEAR_BUFFER_DATA);

	FIPS_DEFER (glClearBufferSubData, target, internalformat,
		      offset, size, format, type, data);

	RESTORE_METRICS_OP ();
}

void
glClearNamedBufferDataEXT (GLuint buffer, GLenum internalformat, GLenum format,
			   GLenum type, const void *data)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_CLEAR_BUFFER_DATA);

	FIPS_DEFER (glClearNamedBufferDataEXT, buffer, internalformat,
		      format, type, data);

	RESTORE_METRICS_OP ();
}

#if GL_GLEXT_VERSION < 20131212
void
glClearNamedBufferSubDataEXT (GLuint buffer, GLenum internalformat,
			      GLenum format, GLenum type, GLsizeiptr offset,
			      GLsizeiptr size, const void *data)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_CLEAR_BUFFER_DATA);

	FIPS_DEFER (glClearNamedBufferSubDataEXT, buffer,
		      internalformat, format, type, offset, size, data);

	RESTORE_METRICS_OP ();
}
#else
void
glClearNamedBufferSubDataEXT (GLuint buffer, GLenum internalformat, 
                              GLsizeiptr offset, GLsizeiptr size, 
                              GLenum format, GLenum type, 
                              const void *data)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_CLEAR_BUFFER_DATA);

	FIPS_DEFER (glClearNamedBufferSubDataEXT, buffer, internalformat, 
                  offset, size, format, type, data);

	RESTORE_METRICS_OP ();
}
#endif

/* METRICS_OP_CLEAR_TEX_IMAGE */


/* METRICS_OP_COPY_PIXELS */
void
glCopyPixels (GLint x, GLint y, GLsizei width, GLsizei height, GLenum type )
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_COPY_PIXELS);

	FIPS_DEFER (glCopyPixels, x, y, width, height, type);

	RESTORE_METRICS_OP ();
}

/* METRICS_OP_COPY_TEX_IMAGE */
void
glCopyTexImage1D (GLenum target, GLint level, GLenum internalformat,
		  GLint x, GLint y, GLsizei width, GLint border)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_COPY_TEX_IMAGE);

	FIPS_DEFER (glCopyTexImage1D, target, level, internalformat,
		      x, y, width, border);

	RESTORE_METRICS_OP ();
}

void
glCopyTexImage1DEXT (GLenum target, GLint level, GLenum internalformat,
		     GLint x, GLint y, GLsizei width, GLint border)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_COPY_TEX_IMAGE);

	FIPS_DEFER (glCopyTexImage1DEXT, target, level, internalformat,
		      x, y, width, border);

	RESTORE_METRICS_OP ();
}

void
glCopyTexImage2D (GLenum target, GLint level, GLenum internalformat,
		  GLint x, GLint y, GLsizei width, GLsizei height,
		  GLint border)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_COPY_TEX_IMAGE);

	FIPS_DEFER (glCopyTexImage2D, target, level, internalformat,
		      x, y, width, height, border);

	RESTORE_METRICS_OP ();
}

void
glCopyTexImage2DEXT (GLenum target, GLint level, GLenum internalformat,
		     GLint x, GLint y, GLsizei width, GLsizei height,
		     GLint border)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_COPY_TEX_IMAGE);

	FIPS_DEFER (glCopyTexImage2DEXT, target, level, internalformat,
		      x, y, width, height, border);

	RESTORE_METRICS_OP ();
}

void
glCopyTexSubImage1D (GLenum target, GLint level, GLint xoffset,
		     GLint x, GLint y, GLsizei width)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_COPY_TEX_IMAGE);

	FIPS_DEFER (glCopyTexSubImage1D, target, level, xoffset,
		      x, y, width);

	RESTORE_METRICS_OP ();
}

void
glCopyTexSubImage1DEXT (GLenum target, GLint level, GLint xoffset,
			GLint x, GLint y, GLsizei width)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_COPY_TEX_IMAGE);

	FIPS_DEFER (glCopyTexSubImage1DEXT, target, level, xoffset,
		      x, y, width);

	RESTORE_METRICS_OP ();
}

void
glCopyTexSubImage2D (GLenum target, GLint level, GLint xoffset,
		     GLint yoffset, GLint x, GLint y, GLsizei width,
		     GLsizei height)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_COPY_TEX_IMAGE);

	FIPS_DEFER (glCopyTexSubImage2D, target, level, xoffset, yoffset,
		      x, y, width, height);

	RESTORE_METRICS_OP ();
}

void
glCopyTexSubImage2DEXT (GLenum target, GLint level, GLint xoffset,
			GLint yoffset, GLint x, GLint y, GLsizei width,
			GLsizei height)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_COPY_TEX_IMAGE);

	FIPS_DEFER (glCopyTexSubImage2DEXT, target, level, xoffset, yoffset,
		      x, y, width, height);

	RESTORE_METRICS_OP ();
}

void
glCopyTexSubImage3D (GLenum target, GLint level, GLint xoffset,
		     GLint yoffset, GLint zoffset, GLint x, GLint y,
		     GLsizei width, GLsizei height)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_COPY_TEX_IMAGE);

	FIPS_DEFER (glCopyTexSubImage3D, target, level, xoffset, yoffset,
		      zoffset, x, y, width, height);

	RESTORE_METRICS_OP ();
}

void
glCopyTexSubImage3DEXT (GLenum target, GLint level, GLint xoffset,
			GLint yoffset, GLint zoffset, GLint x, GLint y,
			GLsizei width, GLsizei height)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_COPY_TEX_IMAGE);

	FIPS_DEFER (glCopyTexSubImage3DEXT, target, level, xoffset, yoffset,
		      zoffset, x, y, width, height);

	RESTORE_METRICS_OP ();
}

void
glCopyTextureImage1DEXT (GLuint texture, GLenum target, GLint level,
			 GLenum internalformat, GLint x, GLint y,
			 GLsizei width, GLint border)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_COPY_TEX_IMAGE);

	FIPS_DEFER (glCopyTextureImage1DEXT, texture, target, level,
		      internalformat, x, y, width, border);

	RESTORE_METRICS_OP ();
}

void
glCopyTextureImage2DEXT (GLuint texture, GLenum target, GLint level,
			 GLenum internalformat, GLint x, GLint y, GLsizei width,
			 GLsizei height, GLint border)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_COPY_TEX_IMAGE);

	FIPS_DEFER (glCopyTextureImage2DEXT, texture, target,
		      level, internalformat, x, y, width, height, border);

	RESTORE_METRICS_OP ();
}

void
glCopyTextureSubImage1DEXT (GLuint texture, GLenum target, GLint level,
			    GLint xoffset, GLint x, GLint y, GLsizei width)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_COPY_TEX_IMAGE);

	FIPS_DEFER (glCopyTextureSubImage1DEXT, texture, target, level,
		      xoffset, x, y, width);

	RESTORE_METRICS_OP ();
}

void
glCopyTextureSubImage2DEXT (GLuint texture, GLenum target, GLint level,
			    GLint xoffset, GLint yoffset, GLint x, GLint y,
			    GLsizei width, GLsizei height)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_COPY_TEX_IMAGE);

	FIPS_DEFER (glCopyTextureSubImage2DEXT, texture, target, level,
		      xoffset, yoffset, x, y, width, height);

	RESTORE_METRICS_OP ();
}

void
glCopyTextureSubImage3DEXT (GLuint texture, GLenum target, GLint level,
			    GLint xoffset, GLint yoffset, GLint zoffset,
			    GLint x, GLint y, GLsizei width, GLsizei height)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_COPY_TEX_IMAGE);

	FIPS_DEFER (glCopyTextureSubImage3DEXT, texture, target, level,
		      xoffset, yoffset, zoffset, x, y, width, height);

	RESTORE_METRICS_OP ();
}

void
glCopyMultiTexImage1DEXT (GLenum texunit, GLenum target, GLint level,
			  GLenum internalformat, GLint x, GLint y,
			  GLsizei width, GLint border)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_COPY_TEX_IMAGE);

	FIPS_DEFER (glCopyMultiTexImage1DEXT, texunit, target, level,
		      internalformat, x, y, width, border);

	RESTORE_METRICS_OP ();
}

void
glCopyMultiTexImage2DEXT (GLenum texunit, GLenum target, GLint level,
			  GLenum internalformat, GLint x, GLint y,
			  GLsizei width, GLsizei height, GLint border)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_COPY_TEX_IMAGE);

	FIPS_DEFER (glCopyMultiTexImage2DEXT, texunit, target, level,
		      internalformat, x, y, width, height, border);

	RESTORE_METRICS_OP ();
}

void
glCopyMultiTexSubImage1DEXT (GLenum texunit, GLenum target, GLint level,
			     GLint xoffset, GLint x, GLint y, GLsizei width)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_COPY_TEX_IMAGE);

	FIPS_DEFER (glCopyMultiTexSubImage1DEXT, texunit, target, level,
		      xoffset, x, y, width);

	RESTORE_METRICS_OP ();
}

void
glCopyMultiTexSubImage2DEXT (GLenum texunit, GLenum target, GLint level,
			     GLint xoffset, GLint yoffset, GLint x, GLint y,
			     GLsizei width, GLsizei height)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_COPY_TEX_IMAGE);

	FIPS_DEFER (glCopyMultiTexSubImage2DEXT, texunit, target, level,
		      xoffset, yoffset, x, y, width, height);

	RESTORE_METRICS_OP ();
}

void
glCopyMultiTexSubImage3DEXT (GLenum texunit, GLenum target, GLint level,
			     GLint xoffset, GLint yoffset, GLint zoffset,
			     GLint x, GLint y, GLsizei width, GLsizei height)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_COPY_TEX_IMAGE);

	FIPS_DEFER (glCopyMultiTexSubImage3DEXT, texunit, target, level,
		      xoffset, yoffset, zoffset, x, y, width, height);

	RESTORE_METRICS_OP ();
}

/* METRICS_OP_DRAW_PIXELS */
void
glDrawPixels (GLsizei width, GLsizei height, GLenum format, GLenum type,
	      const GLvoid *pixels)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_DRAW_PIXELS);

	FIPS_DEFER (glDrawPixels, width, height, format, type, pixels);

	RESTORE_METRICS_OP ();
}

/* METRICS_OP_GET_TEX_IMAGE */

void
glGetCompressedMultiTexImageEXT (GLenum texunit, GLenum target,
				 GLint lod, GLvoid *img)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_GET_TEX_IMAGE);

	FIPS_DEFER (glGetCompressedMultiTexImageEXT, texunit,
		      target, lod, img);

	RESTORE_METRICS_OP ();
}

void
glGetCompressedTexImage (GLenum target, GLint level, GLvoid *img)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_GET_TEX_IMAGE);

	FIPS_DEFER (glGetCompressedTexImage, target, level, img);

	RESTORE_METRICS_OP ();
}

void
glGetCompressedTexImageARB (GLenum target, GLint level, GLvoid *img)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_GET_TEX_IMAGE);

	FIPS_DEFER (glGetCompressedTexImageARB, target, level, img);

	RESTORE_METRICS_OP ();
}

void
glGetCompressedTextureImageEXT (GLuint texture, GLenum target,
				GLint lod, GLvoid *img)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_GET_TEX_IMAGE);

	FIPS_DEFER (glGetCompressedTextureImageEXT, texture,
		      target, lod, img);

	RESTORE_METRICS_OP ();
}

void
glGetMultiTexImageEXT (GLenum texunit, GLenum target, GLint level,
		       GLenum format, GLenum type, GLvoid *pixels)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_GET_TEX_IMAGE);

	FIPS_DEFER (glGetMultiTexImageEXT, texunit,
		      target, level, format, type, pixels);

	RESTORE_METRICS_OP ();
}

void
glGetnCompressedTexImageARB (GLenum target, GLint lod,
			     GLsizei bufSize, GLvoid *img)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_GET_TEX_IMAGE);

	FIPS_DEFER (glGetnCompressedTexImageARB, target, lod, bufSize, img);

	RESTORE_METRICS_OP ();
}

void
glGetnTexImageARB (GLenum target, GLint level, GLenum format,
		   GLenum type, GLsizei bufSize, GLvoid *img)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_GET_TEX_IMAGE);

	FIPS_DEFER (glGetnTexImageARB, target, level,
		      format, type, bufSize, img);

	RESTORE_METRICS_OP ();
}

void
glGetTexImage (GLenum target, GLint level, GLenum format, GLenum type,
	       GLvoid *pixels )
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_GET_TEX_IMAGE);

	FIPS_DEFER (glGetTexImage, target, level, format, type, pixels);

	RESTORE_METRICS_OP ();
}

/* METRICS_OP_READ_PIXELS */
void
glReadPixels (GLint x, GLint y, GLsizei width, GLsizei height,
	      GLenum format, GLenum type, GLvoid *pixels )
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_READ_PIXELS);

	FIPS_DEFER (glReadPixels, x, y, width, height, format, type, pixels);

	RESTORE_METRICS_OP ();
}

void
glReadnPixelsARB (GLint x, GLint y, GLsizei width, GLsizei height,
		  GLenum format, GLenum type, GLsizei bufSize, GLvoid *data)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_READ_PIXELS);

	FIPS_DEFER (glReadnPixelsARB, x, y, width, height,
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

	FIPS_DEFER (glTexImage1D, target, level, internalFormat, width,
		      border, format, type, pixels);

	RESTORE_METRICS_OP ();
}

void
glTexImage2D (GLenum target, GLint level, GLint internalFormat,
	      GLsizei width, GLsizei height, GLint border, GLenum format,
	      GLenum type, const GLvoid *pixels )
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_TEX_IMAGE);

	FIPS_DEFER (glTexImage2D, target, level, internalFormat,
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

	FIPS_DEFER (glTexImage2DMultisample, target, samples,
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

	FIPS_DEFER (glTexImage2DMultisampleCoverageNV, target,
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

	FIPS_DEFER (glTexImage3D, target, level, internalformat,
		      width, height, depth, border, format, type, pixels);

	RESTORE_METRICS_OP ();
}

void
glTexImage3DEXT (GLenum target, GLint level, GLenum internalformat,
		 GLsizei width, GLsizei height, GLsizei depth, GLint border,
		 GLenum format, GLenum type, const GLvoid *pixels)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_TEX_IMAGE);

	FIPS_DEFER (glTexImage3DEXT, target, level, internalformat,
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

	FIPS_DEFER (glTexImage3DMultisample, target, samples,
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

	FIPS_DEFER (glTexImage3DMultisampleCoverageNV, target,
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

	FIPS_DEFER (glTexImage4DSGIS, target, level,
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

	FIPS_DEFER (glTexSubImage1D, target, level, xoffset,
		      width, format, type, pixels);

	RESTORE_METRICS_OP ();
}

void
glTexSubImage1DEXT (GLenum target, GLint level, GLint xoffset,
		    GLsizei width, GLenum format, GLenum type,
		    const GLvoid *pixels)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_TEX_IMAGE);

	FIPS_DEFER (glTexSubImage1DEXT, target, level, xoffset,
		      width, format, type, pixels);

	RESTORE_METRICS_OP ();
}

void
glTexSubImage2D (GLenum target, GLint level, GLint xoffset, GLint yoffset,
		 GLsizei width, GLsizei height, GLenum format, GLenum type,
		 const GLvoid *pixels)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_TEX_IMAGE);

	FIPS_DEFER (glTexSubImage2D, target, level, xoffset, yoffset,
		      width, height, format, type, pixels);

	RESTORE_METRICS_OP ();
}

void
glTexSubImage2DEXT (GLenum target, GLint level, GLint xoffset, GLint yoffset,
		    GLsizei width, GLsizei height, GLenum format, GLenum type,
		    const GLvoid *pixels)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_TEX_IMAGE);

	FIPS_DEFER (glTexSubImage2DEXT, target, level, xoffset, yoffset,
		      width, height, format, type, pixels);

	RESTORE_METRICS_OP ();
}

void
glTexSubImage3D (GLenum target, GLint level, GLint xoffset, GLint yoffset,
		 GLint zoffset, GLsizei width, GLsizei height, GLsizei depth,
		 GLenum format, GLenum type, const GLvoid *pixels)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_TEX_IMAGE);

	FIPS_DEFER (glTexSubImage3D, target, level, xoffset, yoffset,
		      zoffset, width, height, depth, format, type, pixels);

	RESTORE_METRICS_OP ();
}

void
glTexSubImage3DEXT (GLenum target, GLint level, GLint xoffset, GLint yoffset,
		    GLint zoffset, GLsizei width, GLsizei height, GLsizei depth,
		    GLenum format, GLenum type, const GLvoid *pixels)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_TEX_IMAGE);

	FIPS_DEFER (glTexSubImage3DEXT, target, level, xoffset, yoffset,
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

	FIPS_DEFER (glTexSubImage4DSGIS, target, level, xoffset,
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

	FIPS_DEFER (glCompressedMultiTexImage1DEXT, texunit, target,
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

	FIPS_DEFER (glCompressedMultiTexImage2DEXT, texunit, target, level,
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

	FIPS_DEFER (glCompressedMultiTexImage3DEXT, texunit, target,
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

	FIPS_DEFER (glCompressedMultiTexSubImage1DEXT, texunit, target,
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

	FIPS_DEFER (glCompressedMultiTexSubImage2DEXT, texunit, target, level,
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

	FIPS_DEFER (glCompressedMultiTexSubImage3DEXT, texunit, target,
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

	FIPS_DEFER (glCompressedTexImage1D, target, level,
		      internalformat, width, border, imageSize, data);

	RESTORE_METRICS_OP ();
}

void
glCompressedTexImage1DARB (GLenum target, GLint level, GLenum internalformat,
			   GLsizei width, GLint border, GLsizei imageSize,
			   const GLvoid *data)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_TEX_IMAGE);

	FIPS_DEFER (glCompressedTexImage1DARB, target, level, internalformat,
		      width, border, imageSize, data);

	RESTORE_METRICS_OP ();
}

void
glCompressedTexImage2D (GLenum target, GLint level, GLenum internalformat,
			GLsizei width, GLsizei height, GLint border,
			GLsizei imageSize, const GLvoid *data)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_TEX_IMAGE);

	FIPS_DEFER (glCompressedTexImage2D, target, level, internalformat,
		      width, height, border, imageSize, data);

	RESTORE_METRICS_OP ();
}

void
glCompressedTexImage2DARB (GLenum target, GLint level, GLenum internalformat,
			   GLsizei width, GLsizei height, GLint border,
			   GLsizei imageSize, const GLvoid *data)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_TEX_IMAGE);

	FIPS_DEFER (glCompressedTexImage2DARB, target, level, internalformat,
		      width, height, border, imageSize, data);

	RESTORE_METRICS_OP ();
}

void
glCompressedTexImage3D (GLenum target, GLint level, GLenum internalformat,
			GLsizei width, GLsizei height, GLsizei depth,
			GLint border, GLsizei imageSize, const GLvoid *data)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_TEX_IMAGE);

	FIPS_DEFER (glCompressedTexImage3D, target, level, internalformat,
		      width, height, depth, border, imageSize, data);

	RESTORE_METRICS_OP ();
}

void
glCompressedTexImage3DARB (GLenum target, GLint level, GLenum internalformat,
			   GLsizei width, GLsizei height, GLsizei depth,
			   GLint border, GLsizei imageSize, const GLvoid *data)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_TEX_IMAGE);

	FIPS_DEFER (glCompressedTexImage3DARB, target, level, internalformat,
		      width, height, depth, border, imageSize, data);

	RESTORE_METRICS_OP ();
}

void
glCompressedTexSubImage1D (GLenum target, GLint level, GLint xoffset,
			   GLsizei width, GLenum format, GLsizei imageSize,
			   const GLvoid *data)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_TEX_IMAGE);

	FIPS_DEFER (glCompressedTexSubImage1D, target, level, xoffset,
		      width, format, imageSize, data);

	RESTORE_METRICS_OP ();
}

void
glCompressedTexSubImage1DARB (GLenum target, GLint level, GLint xoffset,
			      GLsizei width, GLenum format, GLsizei imageSize,
			      const GLvoid *data)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_TEX_IMAGE);

	FIPS_DEFER (glCompressedTexSubImage1DARB, target, level, xoffset,
		      width, format, imageSize, data);

	RESTORE_METRICS_OP ();
}

void
glCompressedTexSubImage2D (GLenum target, GLint level, GLint xoffset,
			   GLint yoffset, GLsizei width, GLsizei height,
			   GLenum format, GLsizei imageSize, const GLvoid *data)
{
	SAVE_THEN_SWITCH_METRICS_OP (METRICS_OP_TEX_IMAGE);

	FIPS_DEFER (glCompressedTexSubImage2D, target, level, xoffset,
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

	FIPS_DEFER (glCompressedTexSubImage2DARB, target, level, xoffset,
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

	FIPS_DEFER (glCompressedTexSubImage3D, target, level, xoffset,
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

	FIPS_DEFER (glCompressedTexSubImage3DARB, target, level, xoffset,
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

	FIPS_DEFER (glCompressedTextureImage1DEXT, texture, target, level,
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

	FIPS_DEFER (glCompressedTextureImage2DEXT, texture, target, level,
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

	FIPS_DEFER (glCompressedTextureImage3DEXT, texture, target,
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

	FIPS_DEFER (glCompressedTextureSubImage1DEXT, texture, target,
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

	FIPS_DEFER (glCompressedTextureSubImage2DEXT, texture, target, level,
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

	FIPS_DEFER (glCompressedTextureSubImage3DEXT, texture, target,
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

	FIPS_DEFER (glMultiTexImage1DEXT, texunit, target, level,
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

	FIPS_DEFER (glMultiTexImage2DEXT, texunit, target, level,
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

	FIPS_DEFER (glMultiTexImage3DEXT, texunit, target, level,
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

	FIPS_DEFER (glMultiTexSubImage1DEXT, texunit, target, level,
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

	FIPS_DEFER (glMultiTexSubImage2DEXT, texunit, target, level, xoffset,
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

	FIPS_DEFER (glMultiTexSubImage3DEXT, texunit, target, level,
		      xoffset, yoffset, zoffset, width, height, depth,
		      format, type, pixels);

	RESTORE_METRICS_OP ();
}

void glDrawArrays( GLenum mode, GLint first, GLsizei count )
{
	perform_draw_experiments();
	FIPS_DEFER(glDrawArrays, mode, first, count );
}

void glDrawArraysEXT (GLenum mode, GLint first, GLsizei count)
{
	perform_draw_experiments();
	FIPS_DEFER(glDrawArraysEXT, mode, first, count);
}	

void glDrawArraysIndirect (GLenum mode, const void *indirect)
{
	perform_draw_experiments();
	FIPS_DEFER(glDrawArraysIndirect, mode, indirect);
}

void glDrawArraysInstanced (GLenum mode, GLint first, GLsizei count, GLsizei instancecount)
{
	perform_draw_experiments();
	FIPS_DEFER(glDrawArraysInstanced, mode, first, count, instancecount);
}

void glDrawArraysInstancedARB (GLenum mode, GLint first, GLsizei count, GLsizei primcount)
{
	perform_draw_experiments();
	FIPS_DEFER(glDrawArraysInstancedARB, mode, first, count, primcount);
}

void glDrawArraysInstancedBaseInstance (GLenum mode, GLint first, GLsizei count, GLsizei instancecount, GLuint baseinstance)
{
	perform_draw_experiments();
	FIPS_DEFER(glDrawArraysInstancedBaseInstance, mode, first, count, instancecount, baseinstance);
}

void glDrawArraysInstancedEXT (GLenum mode, GLint start, GLsizei count, GLsizei primcount)
{
	perform_draw_experiments();
	FIPS_DEFER(glDrawArraysInstancedEXT, mode, start, count, primcount);
}

void glDrawElements( GLenum mode, GLsizei count, GLenum type, const GLvoid *indices )
{
	perform_draw_experiments();
	FIPS_DEFER(glDrawElements, mode, count, type, indices );
}
void glDrawElementsBaseVertex (GLenum mode, GLsizei count, GLenum type, const void *indices, GLint basevertex)
{
	perform_draw_experiments();
	FIPS_DEFER(glDrawElementsBaseVertex, mode, count, type, indices, basevertex);
}
void glDrawElementsIndirect (GLenum mode, GLenum type, const void *indirect)
{
	perform_draw_experiments();
	FIPS_DEFER(glDrawElementsIndirect, mode, type, indirect);
}

void glDrawElementsInstanced (GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount)
{
	perform_draw_experiments();
	FIPS_DEFER(glDrawElementsInstanced, mode, count, type, indices, instancecount);
}

void glDrawElementsInstancedARB (GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei primcount)
{
	perform_draw_experiments();
	FIPS_DEFER(glDrawElementsInstancedARB, mode, count, type, indices, primcount);
}
void glDrawElementsInstancedBaseInstance (GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount, GLuint baseinstance)
{
	perform_draw_experiments();
	FIPS_DEFER(glDrawElementsInstancedBaseInstance, mode, count, type, indices, instancecount, baseinstance);
}

void glDrawElementsInstancedBaseVertex (GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount, GLint basevertex)
{
	perform_draw_experiments();
	FIPS_DEFER(glDrawElementsInstancedBaseVertex, mode, count, type, indices, instancecount, basevertex);
}

void glDrawElementsInstancedBaseVertexBaseInstance (GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount, GLint basevertex, GLuint baseinstance)
{
	perform_draw_experiments();
	FIPS_DEFER(glDrawElementsInstancedBaseVertexBaseInstance, mode, count, type, indices, instancecount, basevertex, baseinstance);
}

void glDrawElementsInstancedEXT (GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei primcount)
{
	perform_draw_experiments();
	FIPS_DEFER(glDrawElementsInstancedEXT, mode, count, type, indices, primcount);
}

void glDrawRangeElementArrayAPPLE (GLenum mode, GLuint start, GLuint end, GLint first, GLsizei count)
{
	perform_draw_experiments();
	FIPS_DEFER(glDrawRangeElementArrayAPPLE, mode, start, end, first, count);
}

void glDrawRangeElementArrayATI (GLenum mode, GLuint start, GLuint end, GLsizei count)
{
	perform_draw_experiments();
	FIPS_DEFER(glDrawRangeElementArrayATI, mode, start, end, count);
}

void glDrawRangeElements (GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const void *indices)
{
	perform_draw_experiments();
	FIPS_DEFER(glDrawRangeElements, mode, start, end, count, type, indices);
}

void glDrawRangeElementsBaseVertex (GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const void *indices, GLint basevertex)
{
	perform_draw_experiments();
	FIPS_DEFER(glDrawRangeElementsBaseVertex, mode, start, end, count, type, indices, basevertex);
}

void glDrawRangeElementsEXT (GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const void *indices)
{
	perform_draw_experiments();
	FIPS_DEFER(glDrawRangeElementsEXT, mode, start, end, count, type, indices);
}

void glDrawTransformFeedback (GLenum mode, GLuint id)
{
	perform_draw_experiments();
	FIPS_DEFER(glDrawTransformFeedback, mode, id);
}

void glDrawTransformFeedbackInstanced (GLenum mode, GLuint id, GLsizei instancecount)
{
	perform_draw_experiments();
	FIPS_DEFER(glDrawTransformFeedbackInstanced, mode, id, instancecount);
}

void glDrawTransformFeedbackNV (GLenum mode, GLuint id)
{
	perform_draw_experiments();
	FIPS_DEFER(glDrawTransformFeedbackNV, mode, id);
}

void glDrawTransformFeedbackStream (GLenum mode, GLuint id, GLuint stream)
{
	perform_draw_experiments();
	FIPS_DEFER(glDrawTransformFeedbackStream, mode, id, stream);
}

void glDrawTransformFeedbackStreamInstanced (GLenum mode, GLuint id, GLuint stream, GLsizei instancecount)
{
	perform_draw_experiments();
	FIPS_DEFER(glDrawTransformFeedbackStreamInstanced, mode, id, stream, instancecount);
}
