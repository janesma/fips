// Copyright (C) Intel Corp.  2014.  All Rights Reserved.

// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:

// The above copyright notice and this permission notice (including the
// next paragraph) shall be included in all copies or substantial
// portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

//  **********************************************************************/
//  * Authors:
//  *   Mark Janes <mark.a.janes@intel.com>
//  **********************************************************************/

#include "sources/gfgpu_perf_functions.h"

#include <GL/gl.h>
#include <GL/glext.h>
#include <GL/glx.h>
#include <assert.h>
#include <stdlib.h>
#include <dlfcn.h>

using Grafips::PerfFunctions;

bool PerfFunctions::m_is_initialized = false;

namespace {

typedef void (PFNGLGETFIRSTQUERYID) (GLuint *queryId);
typedef void (PFNGLGETNEXTQUERYID) (GLuint queryId,
                                    GLuint *nextQueryId);
typedef void (PFNGLGETQUERYINFO)
    (GLuint queryId, GLuint queryNameLength, GLchar *queryName,
     GLuint *dataSize, GLuint *noCounters, GLuint *noInstances,
     GLuint *capsMask);

typedef void (PFNGLGETPERFCOUNTERINFO) (
    GLuint queryId, GLuint counterId, GLuint counterNameLength,
    GLchar *counterName, GLuint counterDescLength,
    GLchar *counterDesc, GLuint *counterOffset,
    GLuint *counterDataSize,
    GLuint *counterTypeEnum, GLuint *counterDataTypeEnum,
    GLuint64 *rawCounterMaxValue);

typedef void (PFNGLCREATEQUERY) (GLuint queryId, GLuint *queryHandle);
typedef void (PFNGLDELETEQUERY) (GLuint queryHandle);
typedef void (PFNGLBEGINQUERY) (GLuint queryHandle);
typedef void (PFNGLENDQUERY) (GLuint queryHandle);
typedef void (PFNGLGETQUERYDATA) (GLuint queryHandle, GLuint flags,
                                  GLsizei dataSize, GLvoid *data,
                                  GLuint *bytesWritten);

typedef GLuint (PFNGLCREATEPROGRAM) (void);
typedef GLuint (PFNGLCREATESHADER) (GLenum type);
typedef void (PFNGLSHADERSOURCE) (GLuint shader, GLsizei count,
                                  const GLchar *const*str,
                                  const GLint *length);
typedef void (PFNGLCOMPILESHADER) (GLuint shader);
typedef void  (PFNGLGETPROGRAMIV) (GLuint program, GLenum pname, GLint *params);
typedef void (PFNGLGETATTACHEDSHADERS) (GLuint program, GLsizei maxCount,
                                        GLsizei *count, GLuint *shaders);
typedef void (PFNGLGETSHADERIV) (GLuint shader, GLenum pname, GLint *params);
typedef void (PFNGLATTACHSHADER) (GLuint program, GLuint shader);
typedef void (PFNGLLINKPROGRAM) (GLuint program);
typedef void (PFNGLENABLE)(GLenum cap);

static const PFNGLCREATEQUERY *p_glCreatePerfQueryINTEL = NULL;
static const PFNGLDELETEQUERY *p_glDeletePerfQueryINTEL = NULL;
static const PFNGLBEGINQUERY *p_glBeginPerfQueryINTEL = NULL;
static const PFNGLENDQUERY *p_glEndPerfQueryINTEL = NULL;
static const PFNGLGETQUERYDATA *p_glGetPerfQueryDataINTEL = NULL;
static const PFNGLGETQUERYINFO *p_glGetPerfQueryInfoINTEL = NULL;
static const PFNGLGETFIRSTQUERYID *p_glGetFirstPerfQueryId = NULL;
static const PFNGLGETNEXTQUERYID *p_glGetNextPerfQueryId = NULL;
static const PFNGLGETPERFCOUNTERINFO *p_glGetPerfCounterInfoINTEL = NULL;
static const PFNGLCREATEPROGRAM *p_glCreateProgram = NULL;
static const PFNGLCREATESHADER *p_glCreateShader = NULL;
static const PFNGLSHADERSOURCE *p_glShaderSource = NULL;
static const PFNGLCOMPILESHADER *p_glCompileShader = NULL;
static const PFNGLGETPROGRAMIV *p_glGetProgramiv = NULL;
static const PFNGLGETATTACHEDSHADERS *p_glGetAttachedShaders = NULL;
static const PFNGLGETSHADERIV *p_glGetShaderiv = NULL;
static const PFNGLATTACHSHADER *p_glAttachShader = NULL;
static const PFNGLLINKPROGRAM *p_glLinkProgram = NULL;
static const PFNGLENABLE *p_glEnable = NULL;


typedef void (PFNGLUSEPROGRAM) (GLuint program);
static const PFNGLUSEPROGRAM *p_glUseProgram = NULL;

typedef void (PFNGLSCISSOR)(GLint x, GLint y, GLsizei width, GLsizei height);
static const PFNGLSCISSOR *p_glScissor = NULL;

typedef void (PFNGLPOLYGONMODE)(GLenum face, GLenum mode);
static const PFNGLPOLYGONMODE *p_glPolygonMode = NULL;

typedef GLenum (PFNGLGETERROR)(void);
static const PFNGLGETERROR *p_glGetError = NULL;

typedef void (PFNGLGENTEXTURES)(GLsizei n, GLuint *textures);
static const PFNGLGENTEXTURES *p_glGenTextures = NULL;

typedef void (PFNGLACTIVETEXTURE)(GLenum texture);
static const PFNGLACTIVETEXTURE *p_glActiveTexture = NULL;

typedef void (PFNGLTEXPARAMETERI)(GLenum target, GLenum pname, GLint param);
static const PFNGLTEXPARAMETERI *p_glTexParameteri = NULL;

typedef void (PFNGLBINDTEXTURE)(GLenum target, GLuint texture);
static const PFNGLBINDTEXTURE *p_glBindTexture = NULL;

typedef void (PFNGLTEXIMAGE2D)(GLenum target, GLint level,
                           GLint internalFormat,
                           GLsizei width, GLsizei height,
                           GLint border, GLenum format, GLenum type,
                           const GLvoid *pixels);
static const PFNGLTEXIMAGE2D *p_glTexImage2D = NULL;

typedef GLboolean (PFNGLISENABLED)(GLenum cap);
static const PFNGLISENABLED *p_glIsEnabled = NULL;

typedef void (PFNGLGETINTEGERV)(GLenum pname, GLint *params);
static const PFNGLGETINTEGERV *p_glGetIntegerv = NULL;

typedef void (PFNGLDISABLE)(GLenum pname);
static const PFNGLDISABLE *p_glDisable = NULL;

}  // namespace

void
PerfFunctions::Init(void *lookup_fn) {
  if (m_is_initialized)
    return;
  m_is_initialized = true;

  if (!lookup_fn) {
    void *lib_handle = NULL;
    lib_handle = dlopen("libGL.so", RTLD_LAZY | RTLD_GLOBAL);
    lookup_fn = dlsym(lib_handle, "glXGetProcAddress");
    assert(lookup_fn);
  }
  typedef void* (* PFNGLXGETPROCADDRESSPROC) (const GLubyte *procName);
  PFNGLXGETPROCADDRESSPROC get_proc =
      reinterpret_cast<PFNGLXGETPROCADDRESSPROC>(lookup_fn);
  const GLubyte *name =
      reinterpret_cast<const GLubyte*>("glCreatePerfQueryINTEL");

  p_glCreatePerfQueryINTEL =
      reinterpret_cast<const PFNGLCREATEQUERY*>(get_proc(name));

  name = reinterpret_cast<const GLubyte*>("glDeletePerfQueryINTEL");
  p_glDeletePerfQueryINTEL =
      reinterpret_cast<const PFNGLDELETEQUERY*>(get_proc(name));

  name = reinterpret_cast<const GLubyte*>("glBeginPerfQueryINTEL");
  p_glBeginPerfQueryINTEL =
      reinterpret_cast<const PFNGLBEGINQUERY*>(get_proc(name));

  name = reinterpret_cast<const GLubyte*>("glEndPerfQueryINTEL");
  p_glEndPerfQueryINTEL =
      reinterpret_cast<const PFNGLENDQUERY*>(get_proc(name));

  name = reinterpret_cast<const GLubyte*>("glGetPerfQueryDataINTEL");
  p_glGetPerfQueryDataINTEL =
      reinterpret_cast<const PFNGLGETQUERYDATA*>(get_proc(name));

  name = reinterpret_cast<const GLubyte*>("glGetPerfQueryInfoINTEL");
  p_glGetPerfQueryInfoINTEL =
      reinterpret_cast<const PFNGLGETQUERYINFO*>(get_proc(name));

  name = reinterpret_cast<const GLubyte*>("glGetFirstPerfQueryIdINTEL");
  p_glGetFirstPerfQueryId =
      reinterpret_cast<PFNGLGETFIRSTQUERYID*>(get_proc(name));

  name = reinterpret_cast<const GLubyte*>("glGetNextPerfQueryIdINTEL");
  p_glGetNextPerfQueryId =
      reinterpret_cast<PFNGLGETNEXTQUERYID*>(get_proc(name));

  name = reinterpret_cast<const GLubyte*>("glGetPerfCounterInfoINTEL");
  p_glGetPerfCounterInfoINTEL =
      reinterpret_cast<PFNGLGETPERFCOUNTERINFO*>(get_proc(name));

  name = reinterpret_cast<const GLubyte*>("glCreateProgram");
  p_glCreateProgram =
      reinterpret_cast<PFNGLCREATEPROGRAM*>(get_proc(name));

  name = reinterpret_cast<const GLubyte*>("glCreateShader");
  p_glCreateShader =
      reinterpret_cast<PFNGLCREATESHADER*>(get_proc(name));

  name = reinterpret_cast<const GLubyte*>("glShaderSource");
  p_glShaderSource =
      reinterpret_cast<PFNGLSHADERSOURCE*>(get_proc(name));

  name = reinterpret_cast<const GLubyte*>("glCompileShader");
  p_glCompileShader =
      reinterpret_cast<PFNGLCOMPILESHADER*>(get_proc(name));

  name = reinterpret_cast<const GLubyte*>("glGetProgramiv");
  p_glGetProgramiv =
      reinterpret_cast<PFNGLGETPROGRAMIV*>(get_proc(name));

  name = reinterpret_cast<const GLubyte*>("glGetAttachedShaders");
  p_glGetAttachedShaders =
      reinterpret_cast<PFNGLGETATTACHEDSHADERS*>(get_proc(name));

  name = reinterpret_cast<const GLubyte*>("glGetShaderiv");
  p_glGetShaderiv =
      reinterpret_cast<PFNGLGETSHADERIV*>(get_proc(name));

  name = reinterpret_cast<const GLubyte*>("glAttachShader");
  p_glAttachShader =
      reinterpret_cast<PFNGLATTACHSHADER*>(get_proc(name));

  name = reinterpret_cast<const GLubyte*>("glLinkProgram");
  p_glLinkProgram =
      reinterpret_cast<PFNGLLINKPROGRAM*>(get_proc(name));

  name = reinterpret_cast<const GLubyte*>("glEnable");
  p_glEnable =
      reinterpret_cast<PFNGLENABLE*>(get_proc(name));

  assert(p_glCreatePerfQueryINTEL);
  assert(p_glDeletePerfQueryINTEL);
  assert(p_glBeginPerfQueryINTEL);
  assert(p_glEndPerfQueryINTEL);
  assert(p_glGetPerfQueryDataINTEL);
  assert(p_glGetPerfQueryInfoINTEL);
  assert(p_glGetFirstPerfQueryId);
  assert(p_glGetNextPerfQueryId);
  assert(p_glGetPerfCounterInfoINTEL);
  assert(p_glCreateProgram);
  assert(p_glCreateShader);
  assert(p_glShaderSource);
  assert(p_glCompileShader);
  assert(p_glGetProgramiv);
  assert(p_glGetAttachedShaders);
  assert(p_glGetShaderiv);
  assert(p_glAttachShader);
  assert(p_glLinkProgram);
  assert(p_glEnable);

  name = reinterpret_cast<const GLubyte*>("glUseProgram");
  p_glUseProgram =
      reinterpret_cast<PFNGLUSEPROGRAM*>(get_proc(name));
  assert(p_glUseProgram);

  name = reinterpret_cast<const GLubyte*>("glScissor");
  p_glScissor =
      reinterpret_cast<PFNGLSCISSOR*>(get_proc(name));
  assert(p_glScissor);

  name = reinterpret_cast<const GLubyte*>("glPolygonMode");
  p_glPolygonMode =
      reinterpret_cast<PFNGLPOLYGONMODE*>(get_proc(name));
  assert(p_glPolygonMode);

  name = reinterpret_cast<const GLubyte*>("glGetError");
  p_glGetError =
      reinterpret_cast<PFNGLGETERROR*>(get_proc(name));
  assert(p_glGetError);

  name = reinterpret_cast<const GLubyte*>("glGenTextures");
  p_glGenTextures =
      reinterpret_cast<PFNGLGENTEXTURES*>(get_proc(name));
  assert(p_glGenTextures);

  name = reinterpret_cast<const GLubyte*>("glActiveTexture");
  p_glActiveTexture =
      reinterpret_cast<PFNGLACTIVETEXTURE*>(get_proc(name));
  assert(p_glActiveTexture);

  name = reinterpret_cast<const GLubyte*>("glTexParameteri");
  p_glTexParameteri =
      reinterpret_cast<PFNGLTEXPARAMETERI*>(get_proc(name));
  assert(p_glTexParameteri);

  name = reinterpret_cast<const GLubyte*>("glBindTexture");
  p_glBindTexture =
      reinterpret_cast<PFNGLBINDTEXTURE*>(get_proc(name));
  assert(p_glBindTexture);

  name = reinterpret_cast<const GLubyte*>("glTexImage2D");
  p_glTexImage2D =
      reinterpret_cast<PFNGLTEXIMAGE2D*>(get_proc(name));
  assert(p_glTexImage2D);

  name = reinterpret_cast<const GLubyte*>("glIsEnabled");
  p_glIsEnabled = reinterpret_cast<PFNGLISENABLED*>(get_proc(name));
  assert(p_glIsEnabled);

  name = reinterpret_cast<const GLubyte*>("glGetIntegerv");
  p_glGetIntegerv = reinterpret_cast<PFNGLGETINTEGERV*>(get_proc(name));
  assert(p_glGetIntegerv);

  name = reinterpret_cast<const GLubyte*>("glDisable");
  p_glDisable = reinterpret_cast<PFNGLDISABLE*>(get_proc(name));
  assert(p_glDisable);
}

void
PerfFunctions::GetFirstQueryId(GLuint *queryId) {
  p_glGetFirstPerfQueryId(queryId);
}

void
PerfFunctions::GetNextQueryId(GLuint queryId,
                              GLuint *nextQueryId) {
  p_glGetNextPerfQueryId(queryId, nextQueryId);
}

void
PerfFunctions::GetQueryInfo(GLuint queryId, GLuint queryNameLength,
                            GLchar *queryName, GLuint *dataSize,
                            GLuint *noCounters,
                            GLuint *noInstances,
                            GLuint *capsMask) {
  p_glGetPerfQueryInfoINTEL(queryId, queryNameLength, queryName,
                            dataSize, noCounters, noInstances,
                            capsMask);
}

void
PerfFunctions::GetCounterInfo(GLuint queryId, GLuint counterId,
                              GLuint counterNameLength, GLchar *counterName,
                              GLuint counterDescLength, GLchar *counterDesc,
                              GLuint *counterOffset, GLuint *counterDataSize,
                              GLuint *counterTypeEnum,
                              GLuint *counterDataTypeEnum,
                              GLuint64 *rawCounterMaxValue) {
  p_glGetPerfCounterInfoINTEL(queryId, counterId,
                              counterNameLength, counterName,
                              counterDescLength, counterDesc,
                              counterOffset, counterDataSize,
                              counterTypeEnum,
                              counterDataTypeEnum,
                              rawCounterMaxValue);
}

void
PerfFunctions::CreateQuery(GLuint queryId, GLuint *queryHandle) {
  p_glCreatePerfQueryINTEL(queryId, queryHandle);
}

void
PerfFunctions::DeleteQuery(GLuint queryHandle) {
  p_glDeletePerfQueryINTEL(queryHandle);
}

void
PerfFunctions::BeginQuery(GLuint queryHandle) {
  p_glBeginPerfQueryINTEL(queryHandle);
}

void
PerfFunctions::EndQuery(GLuint queryHandle) {
  p_glEndPerfQueryINTEL(queryHandle);
}

void
PerfFunctions::GetQueryData(GLuint queryHandle, GLuint flags,
                            GLsizei dataSize, GLvoid *data,
                            GLuint *bytesWritten) {
  p_glGetPerfQueryDataINTEL(queryHandle, flags,
                            dataSize, data,
                            bytesWritten);
}

GLuint
PerfFunctions::CreateProgram(void) {
  return p_glCreateProgram();
}

GLuint
PerfFunctions::CreateShader(GLenum type) {
  return p_glCreateShader(type);
}

void
PerfFunctions::ShaderSource(GLuint shader, GLsizei count,
                            const GLchar *const*str,
                             const GLint *length) {
  return p_glShaderSource(shader, count, str, length);
}

void
PerfFunctions::CompileShader(GLuint shader) {
  return p_glCompileShader(shader);
}

void
PerfFunctions::GetProgramiv(GLuint program, GLenum pname, GLint *params) {
  return p_glGetProgramiv(program, pname, params);
}

void
PerfFunctions::GetAttachedShaders(GLuint program, GLsizei maxCount,
                                  GLsizei *count, GLuint *shaders) {
  p_glGetAttachedShaders(program, maxCount, count, shaders);
}

void
PerfFunctions::GetShaderiv(GLuint shader, GLenum pname, GLint *params) {
  p_glGetShaderiv(shader, pname, params);
}

void
PerfFunctions::AttachShader(GLuint program, GLuint shader) {
  p_glAttachShader(program, shader);
}

void
PerfFunctions::LinkProgram(GLuint program) {
  p_glLinkProgram(program);
}

void
PerfFunctions::Enable(GLenum cap) {
  p_glEnable(cap);
}

void
PerfFunctions::UseProgram(GLuint program) {
  p_glUseProgram(program);
}

void
PerfFunctions::Scissor(GLint x, GLint y, GLsizei width, GLsizei height) {
  p_glScissor(x, y, width, height);
}

void
PerfFunctions::PolygonMode(GLenum face, GLenum mode) {
  p_glPolygonMode(face, mode);
}

GLenum
PerfFunctions::GetError(void) {
  return p_glGetError();
}

void
PerfFunctions::GenTextures(GLsizei n, GLuint *textures) {
  p_glGenTextures(n, textures);
}

void
PerfFunctions::ActiveTexture(GLenum texture) {
  p_glActiveTexture(texture);
}

void
PerfFunctions::TexParameteri(GLenum target, GLenum pname, GLint param) {
  p_glTexParameteri(target, pname, param);
}

void
PerfFunctions::TexImage2D(GLenum target, GLint level,
                           GLint internalFormat,
                           GLsizei width, GLsizei height,
                           GLint border, GLenum format, GLenum type,
                           const GLvoid *pixels) {
  p_glTexImage2D(target, level,
                  internalFormat,
                  width, height,
                  border, format, type,
                  pixels);
}

void
PerfFunctions::BindTexture(GLenum target, GLuint texture) {
  p_glBindTexture(target, texture);
}

GLboolean PerfFunctions::IsEnabled(GLenum cap) {
  return p_glIsEnabled(cap);
}

void
PerfFunctions::GetIntegerv(GLenum pname, GLint *params) {
  p_glGetIntegerv(pname, params);
}

void
PerfFunctions::Disable(GLenum cap) {
  p_glDisable(cap);
}
