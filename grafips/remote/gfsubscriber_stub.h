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

#ifndef REMOTE_GFSUBSCRIBER_STUB_H_
#define REMOTE_GFSUBSCRIBER_STUB_H_

#include <string>
#include <vector>

#include "os/gfsocket.h"
#include "remote/gfisubscriber.h"
#include "os/gftraits.h"
#include "os/gfmutex.h"

namespace GrafipsProto {
class SubscriberInvocation;
}

namespace Grafips {

class SubscriberStub : public SubscriberInterface,
                       NoCopy, NoAssign, NoMove {
 public:
  SubscriberStub(const std::string &address, int port);
  ~SubscriberStub();
  void Clear(int id);
  void OnMetric(const DataSet &d);
  void OnDescriptions(const std::vector<MetricDescription> &descriptions);
  void Flush() const;
 private:
  void WriteMessage(const GrafipsProto::SubscriberInvocation&m) const;
  mutable Socket m_socket;
  mutable std::vector<unsigned char> m_buf;
  mutable Mutex m_protect;
};

}  // namespace Grafips

#endif  // REMOTE_GFSUBSCRIBER_STUB_H_
