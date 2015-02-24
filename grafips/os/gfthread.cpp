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

#include "os/gfthread.h"

#include <assert.h>
#include <string>

#include "error/gflog.h"

using Grafips::Thread;

Thread::Thread(const std::string &name) : m_name(name) {}

void *start_thread(void*ctx);
void *start_thread(void*ctx) {
  reinterpret_cast<Thread*>(ctx)->Run();
  return NULL;
}

void
Thread::Start() {
  GFLOG("thread started: %s", m_name.c_str());
  const int result = pthread_create(&m_thread, NULL, &start_thread, this);
  assert(result == 0);
}

void
Thread::Join() {
  GFLOG("joining thread: %s", m_name.c_str());
  pthread_join(m_thread, NULL);
  GFLOG("thread joined: %s", m_name.c_str());
}


