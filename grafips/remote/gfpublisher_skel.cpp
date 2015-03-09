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

#include "remote/gfpublisher_skel.h"

#include <google/protobuf/io/zero_copy_stream_impl_lite.h>
#include <google/protobuf/io/coded_stream.h>

#include <vector>

#include "./gfpublisher.pb.h"
#include "error/gferror.h"
#include "error/gflog.h"
#include "os/gfsocket.h"
#include "remote/gfipublisher.h"
#include "remote/gfsubscriber_stub.h"

using Grafips::Error;
using Grafips::NoError;
using Grafips::PublisherSkeleton;
using Grafips::Raise;
using Grafips::WARN;
using Grafips::kSocketWriteFail;

PublisherSkeleton::PublisherSkeleton(int port, PublisherInterface *target)
    : Thread("PublisherSkeleton"), m_server(new ServerSocket(port)),
      m_socket(NULL), m_target(target), m_subscriber(NULL) {
}

PublisherSkeleton::~PublisherSkeleton() {
  if (m_socket)
    delete m_socket;
  if (m_server)
    delete m_server;
  if (m_subscriber)
    delete m_subscriber;
}

void
PublisherSkeleton::Run() {
  m_socket = m_server->Accept();
  delete m_server;
  m_server = NULL;

  std::vector<unsigned char> buf;
  bool running = true;
  while (running) {
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
    google::protobuf::io::ArrayInputStream array_in(buf.data(), buf_size);
    google::protobuf::io::CodedInputStream coded_in(&array_in);

    GrafipsProto::PublisherInvocation m;
    using google::protobuf::io::CodedInputStream;
    // typedef google::protobuf::io::CodedInputStream::Limit Limit;
    CodedInputStream::Limit msg_limit = coded_in.PushLimit(buf_size);
    m.ParseFromCodedStream(&coded_in);
    coded_in.PopLimit(msg_limit);

    using GrafipsProto::PublisherInvocation;
    switch (m.method()) {
      case PublisherInvocation::kFlush: {
        if (!m_socket->Write((uint32_t)0)) {
          // host is closed, stop processing
          running = false;
        }
        break;
      }
      case PublisherInvocation::kEnable: {
        typedef GrafipsProto::PublisherInvocation_Enable Enable;
        const Enable& args= m.enableargs();
        m_target->Enable(args.id());
        break;
      }
      case PublisherInvocation::kDisable: {
        typedef GrafipsProto::PublisherInvocation_Disable Disable;
        const Disable& args= m.disableargs();
        m_target->Disable(args.id());
        break;
      }
      case PublisherInvocation::kSubscribe: {
        assert(m_subscriber == NULL);
        typedef GrafipsProto::PublisherInvocation_Subscribe Subscribe;
        const Subscribe& args = m.subscribeargs();
        m_subscriber = new SubscriberStub(m_socket->Address(), args.port());
        m_target->Subscribe(m_subscriber);
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
    m_subscriber->Close();
  }
}

void
PublisherSkeleton::Flush() const {
  if (m_subscriber)
    m_subscriber->Flush();
}

int
PublisherSkeleton::GetPort() const {
  return m_server->GetPort();
}
