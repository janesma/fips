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

#ifndef ERROR_GFLOG_H_
#define ERROR_GFLOG_H_

#include <stdarg.h>

#include "error/gferror.h"

namespace Grafips {

class LogError : public ErrorInterface {
 public:
  LogError(const char *file, int line, const char *msg) {
    snprintf(m_buf, BUF_SIZE, "%s:%d %s", file, line, msg);
    m_buf[BUF_SIZE - 1] = '\0';
  }
const char *ToString() const { return m_buf; }
  uint32_t Type() const { return kLogMsg; }
  Severity Level() const { return INFO; }
 private:
  static const int BUF_SIZE = 255;
  char m_buf[BUF_SIZE];
};

}  // namespace Grafips

inline void log_message( const char *file, int line, const char *format, ... ) {
  static const int BUF_SIZE = 255;
  char buf[BUF_SIZE];
  va_list ap;
  va_start( ap, format );
  vsnprintf( buf, BUF_SIZE, format, ap );
  va_end(ap);
  
  Grafips::Raise(Grafips::LogError(file, line, buf));
}

#define GFLOG(format, ...) log_message( __FILE__, __LINE__, format, __VA_ARGS__);
#endif  // ERROR_GFLOG_H_
