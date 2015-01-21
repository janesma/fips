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

using Grafips::PerfFunctions;

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

static const PFNGLCREATEQUERY *p_glCreatePerfQueryINTEL = NULL;
static const PFNGLDELETEQUERY *p_glDeletePerfQueryINTEL = NULL;
static const PFNGLBEGINQUERY *p_glBeginPerfQueryINTEL = NULL;
static const PFNGLENDQUERY *p_glEndPerfQueryINTEL = NULL;
static const PFNGLGETQUERYDATA *p_glGetPerfQueryDataINTEL = NULL;
static const PFNGLGETQUERYINFO *p_glGetPerfQueryInfoINTEL = NULL;
static const PFNGLGETFIRSTQUERYID *p_glGetFirstPerfQueryId = NULL;
static const PFNGLGETNEXTQUERYID *p_glGetNextPerfQueryId = NULL;
static const PFNGLGETPERFCOUNTERINFO *p_glGetPerfCounterInfoINTEL = NULL;

}

void
PerfFunctions::Init() {
  const GLubyte *name = reinterpret_cast<const GLubyte*>("glCreatePerfQueryINTEL");
  p_glCreatePerfQueryINTEL =
      reinterpret_cast<const PFNGLCREATEQUERY*>(glXGetProcAddress(name));

  name = reinterpret_cast<const GLubyte*>("glDeletePerfQueryINTEL");
  p_glDeletePerfQueryINTEL =
      reinterpret_cast<const PFNGLDELETEQUERY*>(glXGetProcAddress(name));

  name = reinterpret_cast<const GLubyte*>("glBeginPerfQueryINTEL");
  p_glBeginPerfQueryINTEL =
      reinterpret_cast<const PFNGLBEGINQUERY*>(glXGetProcAddress(name));

  name = reinterpret_cast<const GLubyte*>("glEndPerfQueryINTEL");
  p_glEndPerfQueryINTEL =
      reinterpret_cast<const PFNGLENDQUERY*>(glXGetProcAddress(name));

  name = reinterpret_cast<const GLubyte*>("glGetPerfQueryDataINTEL");
  p_glGetPerfQueryDataINTEL =
      reinterpret_cast<const PFNGLGETQUERYDATA*>(glXGetProcAddress(name));

  name = reinterpret_cast<const GLubyte*>("glGetPerfQueryInfoINTEL");
  p_glGetPerfQueryInfoINTEL =
      reinterpret_cast<const PFNGLGETQUERYINFO*>(glXGetProcAddress(name));

  name = reinterpret_cast<const GLubyte*>("glGetFirstPerfQueryIdINTEL");
  p_glGetFirstPerfQueryId =
      reinterpret_cast<PFNGLGETFIRSTQUERYID*>(glXGetProcAddress(name));

  name = reinterpret_cast<const GLubyte*>("glGetNextPerfQueryIdINTEL");
  p_glGetNextPerfQueryId =
      reinterpret_cast<PFNGLGETNEXTQUERYID*>(glXGetProcAddress(name));

  name = reinterpret_cast<const GLubyte*>("glGetPerfCounterInfoINTEL");
  p_glGetPerfCounterInfoINTEL =
      reinterpret_cast<PFNGLGETPERFCOUNTERINFO*>(glXGetProcAddress(name));

  assert(p_glCreatePerfQueryINTEL);
  assert(p_glDeletePerfQueryINTEL);
  assert(p_glBeginPerfQueryINTEL);
  assert(p_glEndPerfQueryINTEL);
  assert(p_glGetPerfQueryDataINTEL);
  assert(p_glGetPerfQueryInfoINTEL);
  assert(p_glGetFirstPerfQueryId);
  assert(p_glGetNextPerfQueryId);
  assert(p_glGetPerfCounterInfoINTEL);
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
PerfFunctions::GetQueryInfo(GLuint queryId, GLuint queryNameLength, GLchar *queryName,
                            GLuint *dataSize, GLuint *noCounters, GLuint *noInstances,
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
