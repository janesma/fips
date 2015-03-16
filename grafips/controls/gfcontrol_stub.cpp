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

#include "controls/gfcontrol_stub.h"

#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl_lite.h>
#include <unistd.h>

#include <string>
#include <vector>

#include "./gfcontrol.pb.h"
#include "controls/gfcontrol.h"
#include "error/gferror.h"

using Grafips::ControlSkel;
using Grafips::ControlStub;
using Grafips::ControlSubscriberSkel;
using Grafips::ControlSubscriberStub;
using Grafips::Error;
using Grafips::NoError;
using Grafips::Raise;
using Grafips::kSocketWriteFail;
using GrafipsControlProto::ControlInvocation;
using google::protobuf::io::ArrayInputStream;
using google::protobuf::io::ArrayOutputStream;
using google::protobuf::io::CodedInputStream;
using google::protobuf::io::CodedOutputStream;

// TODO(majanes) there is a lot of replicated code between the various
// stubs, skeletons, etc.  It would be good to factor it out, and
// possibly generate some of this.  Especially if more / larger
// interfaces are introduced.

ControlStub::ControlStub(const std::string &address, int port)
    : m_socket(new Socket(address, port)) {
  // socket connects in constructor
}
ControlStub::~ControlStub() {
  delete m_socket;
  if (m_subscriber) {
    m_subscriber->Join();
    delete m_subscriber;
    m_subscriber = NULL;
  }
}

void
ControlStub::Set(const std::string &key, const std::string &value) {
  GrafipsControlProto::ControlInvocation request;

  request.set_method(ControlInvocation::kSet);
  ControlInvocation::Set *args = request.mutable_setargs();
  args->set_key(key);
  args->set_value(value);
  WriteMessage(request);

  // asynchronous, no response
}

void
ControlStub::Subscribe(ControlSubscriberInterface *value) {
  m_subscriber = new ControlSubscriberSkel(0, value);
  m_subscriber->Start();

  const int port = m_subscriber->GetPort();

  GrafipsControlProto::ControlInvocation request;

  request.set_method(ControlInvocation::kSubscribe);
  ControlInvocation::Subscribe *args = request.mutable_subscribeargs();
  // TODO(majanes): detect local ip and use a real address
  args->set_port(port);
  WriteMessage(request);

  // asynchronous, no response
}

void
ControlStub::Flush() {
  GrafipsControlProto::ControlInvocation request;

  request.set_method(ControlInvocation::kFlush);
  WriteMessage(request);
  int response;
  m_socket->Read(&response);
  assert(response == 0);
}

void
ControlStub::WriteMessage(const ControlInvocation &m) const {
  const uint32_t write_size = m.ByteSize();
  m_protect.Lock();
  if (!m_socket->Write(write_size) )
    Raise(Error(kSocketWriteFail, ERROR, "ControlStub wrote to closed socket"));

  m_buf.resize(write_size);
  ArrayOutputStream array_out(m_buf.data(), write_size);
  CodedOutputStream coded_out(&array_out);
  m.SerializeToCodedStream(&coded_out);
  if (!m_socket->Write(m_buf.data(), write_size))
    Raise(Error(kSocketWriteFail, ERROR, "ControlStub wrote to closed socket"));
  m_protect.Unlock();
}

ControlSkel::ControlSkel(int port, ControlRouterTarget *target)
    : Thread("ControlSkel"),
      m_server(new ServerSocket(port)),
      m_socket(NULL),
      m_target(target),
      m_subscriber(NULL) {
      }

ControlSkel::~ControlSkel() {
  if (m_server)
    delete m_server;
  if (m_socket)
    delete m_socket;
}

void
ControlSkel::Run() {
  m_socket = m_server->Accept();
  delete m_server;
  m_server = NULL;

  std::vector<unsigned char> buf;
  bool running = true;
  while (running && NoError()) {
    uint32_t msg_len;
    if (!m_socket->Read(&msg_len)) {
      // host is closed, stop processing
      break;
    }

    // std::cout << "read len: " << msg_len << std::endl;
    buf.resize(msg_len);
    if (!m_socket->ReadVec(&buf)) {
      // host is closed, stop processing
      break;
    }

    // for (int i = 0; i < msg_len; ++i)
    // std::cout << " " << (int) buf[i] << " ";
    // std::cout << std::endl;

    const size_t buf_size = buf.size();
    ArrayInputStream array_in(buf.data(), buf_size);
    CodedInputStream coded_in(&array_in);

    GrafipsControlProto::ControlInvocation m;
    CodedInputStream::Limit msg_limit = coded_in.PushLimit(buf_size);
    m.ParseFromCodedStream(&coded_in);
    coded_in.PopLimit(msg_limit);

    using GrafipsControlProto::ControlInvocation;
    switch (m.method()) {
      case ControlInvocation::kSet: {
        const ControlInvocation::Set& args = m.setargs();
        m_target->Set(args.key(), args.value());
        break;
      }
      case ControlInvocation::kSubscribe: {
        assert(m_subscriber == NULL);
        const ControlInvocation::Subscribe& args = m.subscribeargs();
        m_subscriber = new ControlSubscriberStub(m_socket->Address(),
                                                 args.port());
        m_target->Subscribe(m_subscriber);
        break;
      }
      case ControlInvocation::kFlush: {
        if (!m_socket->Write(0))
          // host is closed, stop processing
          running = false;
        break;
      }
      default: {
        assert(false);
        running = false;
        break;
      }
    }
  }

  // clean up subscriber stub
  if (m_subscriber) {
    delete m_subscriber;
    m_subscriber = NULL;
  }
}

ControlSubscriberStub::ControlSubscriberStub(const std::string &address,
                                             int port)
    : m_socket(new Socket(address, port)) {
}

ControlSubscriberStub::~ControlSubscriberStub() {
  delete(m_socket);
}

void
ControlSubscriberStub::OnControlChanged(const std::string &key,
                                        const std::string &value) {
  GrafipsControlProto::ControlInvocation request;

  request.set_method(ControlInvocation::kOnControlChanged);
  ControlInvocation::OnControlChanged *args =
      request.mutable_oncontrolchangedargs();
  args->set_key(key);
  args->set_value(value);
  WriteMessage(request);

  // asynchronous, no response
}

void
ControlSubscriberStub::WriteMessage(const ControlInvocation &m) const {
  const uint32_t write_size = m.ByteSize();
  m_protect.Lock();
  if (!m_socket->Write(write_size)) {
    Raise(Error(kSocketWriteFail, ERROR,
                "ControlSubscriberStub wrote to closed socket"));
    return;
  }

  m_buf.resize(write_size);
  ArrayOutputStream array_out(m_buf.data(), write_size);
  CodedOutputStream coded_out(&array_out);
  m.SerializeToCodedStream(&coded_out);
  if (!m_socket->Write(m_buf.data(), write_size)) {
    Raise(Error(kSocketWriteFail, ERROR,
                "ControlSubscriberStub wrote to closed socket"));
    return;
  }
  m_protect.Unlock();
}


ControlSubscriberSkel::ControlSubscriberSkel(int port,
                                             ControlSubscriberInterface *target)
    : Thread("ControlSubscriberSkel"),
      m_server(new ServerSocket(port)),
      m_socket(NULL),
      m_target(target) {
      }

void
ControlSubscriberSkel::Run() {
  m_socket = m_server->Accept();
  delete m_server;
  m_server = NULL;

  std::vector<unsigned char> buf;
  bool running = true;
  while (running) {
    uint32_t msg_len;
    if (!m_socket->Read(&msg_len)) {
      // target is closed, stop processing
      break;
    }
    buf.resize(msg_len);
    if (!m_socket->ReadVec(&buf)) {
      // target is closed, stop processing
      break;
    }

    const size_t buf_size = buf.size();
    ArrayInputStream array_in(buf.data(), buf_size);
    CodedInputStream coded_in(&array_in);

    GrafipsControlProto::ControlInvocation m;
    CodedInputStream::Limit msg_limit = coded_in.PushLimit(buf_size);
    m.ParseFromCodedStream(&coded_in);
    coded_in.PopLimit(msg_limit);
    using GrafipsControlProto::ControlInvocation;
    switch (m.method()) {
      case ControlInvocation::kOnControlChanged: {
        const ControlInvocation::OnControlChanged& args
            = m.oncontrolchangedargs();
        m_target->OnControlChanged(args.key(), args.value());
        break;
      }
      case ControlInvocation::kFlush: {
        if (!m_socket->Write(0)) {
          // target is closed, stop processing
          running = false;
        }
        break;
      }
      default: {
        assert(false);
        running = false;
        break;
      }
    }
  }
}

int
ControlSubscriberSkel::GetPort() const { return m_server->GetPort(); }

void
ControlSubscriberStub::Flush() {
  GrafipsControlProto::ControlInvocation request;

  request.set_method(ControlInvocation::kFlush);
  WriteMessage(request);
  int response;
  if (!m_socket->Read(&response)) {
    Raise(Error(kSocketReadFail, ERROR,
                "ControlSubscriberStub read from closed socket"));
    return;
  }
  assert(response == 0);
}

void
ControlSkel::Flush() {
  if (m_subscriber) {
    m_subscriber->Flush();
  }
}


