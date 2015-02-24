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

#include "error/gferror.h"

#include <assert.h>

#include <vector>

using Grafips::DEBUG;
using Grafips::ERROR;
using Grafips::Error;
using Grafips::ErrorHandler;
using Grafips::ErrorInterface;
using Grafips::FATAL;
using Grafips::INFO;
using Grafips::Severity;
using Grafips::WARN;

namespace {

class HandlerStorage {
 public:
  HandlerStorage() : m_stack_error_count(0) {}
  void Raise(const ErrorInterface &e);
  void Push(ErrorHandler *h);
  void Pop(ErrorHandler *h);
  void DecrementErrorCount(int amount) { m_stack_error_count -= amount; }
  bool NoError() const { return (m_stack_error_count == 0); }
 private:
  std::vector<ErrorHandler *> m_handlers;
  int m_stack_error_count;
};

void free_handler_storage_at_thread_exit(void* storage) {
  delete reinterpret_cast<HandlerStorage*>(storage);
}

static pthread_key_t storage_key;

class Initializer {
 public:
  Initializer() {
    pthread_key_create(&storage_key, free_handler_storage_at_thread_exit);
  }
};

static Initializer static_object_to_create_key;
}  // namespace

// helper function to ensure a valid handler is always in place.
inline HandlerStorage *
get_thread_storage() {
  void * tmp = pthread_getspecific(storage_key);
  HandlerStorage *storage = reinterpret_cast<HandlerStorage*>(tmp);
  if (storage == NULL) {
    // if no storage exists, create it
    storage = new HandlerStorage;
    pthread_setspecific(storage_key, storage);
  }
  return storage;
}

Error::Error(ErrorTypes type, Severity level, const char *msg)
    : m_type(type), m_level(level), m_msg(msg) {}

void
Grafips::Raise(const ErrorInterface &e) {
  get_thread_storage()->Raise(e);
}

bool
Grafips::NoError() {
  return get_thread_storage()->NoError();
}

ErrorHandler::ErrorHandler()
    : m_handled_count(0) {
  // place handler on storage
  get_thread_storage()->Push(this);
}

ErrorHandler::~ErrorHandler() {
  HandlerStorage *storage = get_thread_storage();

  // remove handler from storage
  storage->Pop(this);

  // any errors handled should not impact code at a higher scope than
  // the handler.
  storage->DecrementErrorCount(m_handled_count);
}

void
HandlerStorage::Raise(const ErrorInterface &e) {
  if (e.Level() == FATAL) {
    // unhandled error: terminate
    printf("FATAL: %s\n", e.ToString());
    assert(false);  // provide backtrace
    exit(-1);
  }

  // TODO(majanes) add control here
  if (e.Level() == INFO) {
    printf("INFO: %s\n", e.ToString());
    return;
  }

  if (e.Level() == DEBUG) {
    printf("DEBUG: %s\n", e.ToString());
    return;
  }

  // increment the error count, so code at scope between the handler
  // and the error can detect that the error occured.
  ++m_stack_error_count;

  for (std::vector<ErrorHandler *>::reverse_iterator i = m_handlers.rbegin();
       i != m_handlers.rend(); ++i) {
    if ((*i)->OnError(e)) {
      // keep a count of how many errors were handled, so errorcount
      // can be correctly decremented when the handler goes out of
      // scope.
      (*i)->IncrementHandled();
      return;
    }
  }
  if (e.Level() == WARN) {
    printf("WARN: %s\n", e.ToString());
    return;
  }
  if (e.Level() == ERROR) {
    // unhandled error: terminate
    printf("ERROR: unhandled error, %s\n", e.ToString());
    assert(false);  // provide backtrace
    exit(-1);
  }
}

void
HandlerStorage::Push(ErrorHandler *h) { m_handlers.push_back(h); }

void
HandlerStorage::Pop(ErrorHandler *h) {
  // the handler is *very* likely to be at the end of the
  // vector, since handlers will be usually created and
  // destroyed on the stack.  However, they may be slightly
  // out of order, or created on the heap.  Start the search
  // at the end of the vector.
  for (std::vector<ErrorHandler *>::reverse_iterator i = m_handlers.rbegin();
      i != m_handlers.rend(); ++i) {
    if (*i != h)
      continue;

    // reverse iterators are lame.  sometimes the STL is gross.
    // http://stackoverflow.com/questions/1830158/how-to-call-erase-with-a-reverse-iterator
    m_handlers.erase((i+1).base());
    return;
  }
}

