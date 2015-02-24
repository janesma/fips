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

#ifndef ERROR_GFERROR_H_
#define ERROR_GFERROR_H_

#include <string>

namespace Grafips {
enum Severity {
  DEBUG,   // if verbose, will be printed
  INFO,    // will be printed, unless quiet mode
  WARN,    // will be printed unless handled
  ERROR,   // will result in error if unhandled
  FATAL    // will terminate program, even if it is handled
};

enum ErrorTypes {
  kSocketWriteFail,
  kSocketReadFail,
  kLogMsg,
};

class ErrorInterface {
 public:
  virtual ~ErrorInterface() {}
  virtual const char *ToString() const = 0;
  virtual uint32_t Type() const = 0;
  virtual Severity Level() const = 0;
};

class Error : public ErrorInterface {
 public:
  Error(ErrorTypes type, Severity level, const char *msg);
  const char *ToString() const { return m_msg.c_str(); }
  uint32_t Type() const { return m_type; }
  Severity Level() const { return m_level; }
 private:
  const ErrorTypes m_type;
  const Severity m_level;
  const std::string m_msg;
};

class ErrorHandler {
 public:
  ErrorHandler();
  virtual ~ErrorHandler();
  virtual bool OnError(const ErrorInterface &e) = 0;
  void IncrementHandled() { ++m_handled_count; }
 private:
  int m_handled_count;
};

void Raise(const ErrorInterface &);
bool NoError();
}  // namespace Grafips

#endif  // ERROR_GFERROR_H_
