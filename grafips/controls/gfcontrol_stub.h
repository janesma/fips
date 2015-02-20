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

#ifndef CONTROLS_GFCONTROL_STUB_H_
#define CONTROLS_GFCONTROL_STUB_H_

#include <vector>
#include <string>

#include "controls/gficontrol.h"
#include "os/gfsocket.h"
#include "os/gfmutex.h"
#include "os/gfthread.h"
#include "./gfcontrol.pb.h"

namespace GrafipsControlProto {
class ControlInvocation;
}

namespace Grafips {
class ControlRouterHost;
class ControlRouterTarget;
class ControlSubscriberSkel;

class ControlStub {
 public:
  ControlStub(const std::string &address, int port);
  ~ControlStub();

  void Set(const std::string &key, const std::string &value);
  void Subscribe(ControlSubscriberInterface *value);
  void Flush();

 private:
  void WriteMessage(const GrafipsControlProto::ControlInvocation &m) const;
  ControlSubscriberSkel *m_subscriber;
  mutable Socket *m_socket;
  mutable std::vector<unsigned char> m_buf;
  mutable Mutex m_protect;
};

class ControlSubscriberStub;
class ControlSkel : public Thread {
 public:
  ControlSkel(int port, ControlRouterTarget *target);
  ~ControlSkel();
  void Run();
  int GetPort() { return m_server->GetPort(); }
  void Flush();

 private:
  void WriteMessage(const GrafipsControlProto::ControlInvocation &m);
  ServerSocket *m_server;
  Socket *m_socket;
  ControlRouterTarget *m_target;

  // on Subscribe(), this member is created to send publications remotely
  ControlSubscriberStub *m_subscriber;
};

class ControlSubscriberStub : public ControlSubscriberInterface {
 public:
  ControlSubscriberStub(const std::string &address, int port);
  ~ControlSubscriberStub();
  void OnControlChanged(const std::string &key,
                        const std::string &value);
  void Flush();
 private:
  void WriteMessage(const GrafipsControlProto::ControlInvocation &m) const;
  mutable Socket *m_socket;
  mutable std::vector<unsigned char> m_buf;
  mutable Mutex m_protect;
};

class ControlSubscriberSkel : public Thread {
 public:
  ControlSubscriberSkel(int port,
                        ControlSubscriberInterface *target);
  int GetPort() const;
  void Run();
 private:
  ServerSocket *m_server;
  Socket *m_socket;
  ControlSubscriberInterface *m_target;
};

}  // namespace Grafips

#endif  // CONTROLS_GFCONTROL_STUB_H_
