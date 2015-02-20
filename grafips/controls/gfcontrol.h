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

#ifndef CONTROLS_GFCONTROL_H_
#define CONTROLS_GFCONTROL_H_

#include <map>
#include <string>

#include "controls/gficontrol.h"

// UI
//   widget   widget
//     ControlRouterHost
//       ControlStub             ControlSubscriberSkel
//        socket                      serversocket

//          <subscribe>                <OnControlChanged>
//          <set>

//        serversocket                    socket
//       ControlRouterTarget             ControlSubscriberStub
//                      ControlSkel
//               <subscribe>
//    ControlInterface ControlInterfac
//    control                control

namespace Grafips {

// multiplexes a single control socket over several controllers, on
// the target side of the connection.  Controllers register themselves
// for the keys that they support with AddControl.  Subsequent Set
// invocations will be delivered to the appropriate controller for the
// key.  ControlRouterTarget will forward any observed control changes
// to its subscriber, which is intended to be a
// ControlSubscriberInterface stub passing the data over the socket to
// the UI.
class ControlRouterTarget : public ControlSubscriberInterface {
 public:
  ControlRouterTarget();
  void AddControl(const std::string &key, ControlInterface* target);
  void Subscribe(ControlSubscriberInterface *sub);
  bool Set(const std::string &key, const std::string &value);

  void OnControlChanged(const std::string &key,
                        const std::string &value);

 private:
  std::map<std::string, ControlInterface *> m_targets;
  std::map<std::string, std::string> m_current_state;

  // this is a stub, to be instantiated by the skeleton that owns the
  // ControlRouterTarget
  ControlSubscriberInterface *m_subscriber;
};

}  // namespace Grafips

#endif  // CONTROLS_GFCONTROL_H_
