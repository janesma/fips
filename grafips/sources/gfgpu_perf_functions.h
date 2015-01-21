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

#ifndef SOURCES_GFGPU_PERF_FUNCTIONS_H_
#define SOURCES_GFGPU_PERF_FUNCTIONS_H_

#include <GL/gl.h>
#include <GL/glext.h>

namespace Grafips {

class PerfFunctions {
 public:
  static void Init();
  static void GetFirstQueryId(GLuint *queryId);

  static void GetNextQueryId(GLuint queryId,
                             GLuint *nextQueryId);

  static void GetQueryInfo(GLuint queryId, GLuint queryNameLength, GLchar *queryName,
                           GLuint *dataSize, GLuint *noCounters, GLuint *noInstances,
                           GLuint *capsMask);

  static void GetCounterInfo(GLuint queryId, GLuint counterId,
                             GLuint counterNameLength, GLchar *counterName,
                             GLuint counterDescLength, GLchar *counterDesc,
                             GLuint *counterOffset, GLuint *counterDataSize,
                             GLuint *counterTypeEnum,
                             GLuint *counterDataTypeEnum,
                             GLuint64 *rawCounterMaxValue);
  
  static void CreateQuery(GLuint queryId, GLuint *queryHandle);
  static void DeleteQuery(GLuint queryHandle);
  static void BeginQuery(GLuint queryHandle);
  static void EndQuery(GLuint queryHandle);
  static void GetQueryData(GLuint queryHandle, GLuint flags,
                           GLsizei dataSize, GLvoid *data,
                           GLuint *bytesWritten);
 private:
  PerfFunctions();
};
}

#endif  // SOURCES_GFGPU_PERF_FUNCTIONS_H_
