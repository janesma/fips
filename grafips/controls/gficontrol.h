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

#ifndef CONTROLS_GFICONTROL_H_
#define CONTROLS_GFICONTROL_H_

#include <string>

namespace Grafips {

class ControlSubscriberInterface {
 public:
  virtual ~ControlSubscriberInterface() {}
  virtual void OnControlChanged(const std::string &key,
                                const std::string &value) = 0;
};

// this interface supports invoking expirements on the target
// workload.  This mechanism is intended to support:
//
// - CpuFreq control
// - 2x2 texture substitution
// - 1x1 scissor rect override
// - Wireframe mode
// - Disable draw calls
// - Disable Z-test
// - Disable alpha blending
// - Disable MSAA
// - Simple fragment shader
// - Disable alpha test
// - Pause / Continue
class ControlInterface {
 public:
  virtual ~ControlInterface() {}
  virtual void Set(const std::string &key, const std::string &value) = 0;
  virtual void Subscribe(ControlSubscriberInterface *value) = 0;
};

}  // namespace Grafips

#endif  // CONTROLS_GFICONTROL_H_
