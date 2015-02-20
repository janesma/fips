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

#ifndef REMOTE_GFPUBLISHER_SKEL_H_
#define REMOTE_GFPUBLISHER_SKEL_H_

#include "os/gfthread.h"

namespace GrafipsProto {
class PublisherInvocation;
}

namespace Grafips {
class ServerSocket;
class Socket;
class PublisherInterface;
class SubscriberStub;

class PublisherSkeleton : public Thread {
 public:
  PublisherSkeleton(int port, PublisherInterface *target);
  ~PublisherSkeleton();
  void Run();
  void Flush() const;
  int GetPort() const;
 private:
  ServerSocket *m_server;
  Socket *m_socket;
  PublisherInterface *m_target;

  // on Subscribe(), this member is created to send publications remotely
  SubscriberStub *m_subscriber;
};
}  // namespace Grafips

#endif  // REMOTE_GFPUBLISHER_SKEL_H_
