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

#ifndef CONTROLS_GFAPI_CONTROL_H_
#define CONTROLS_GFAPI_CONTROL_H_

#include <map>
#include <string>
#include <vector>

#include "controls/gficontrol.h"
#include "os/gfmutex.h"

namespace Grafips {

class ApiControl : public ControlInterface {
 public:
  ApiControl();
  ~ApiControl();
  void Set(const std::string &key, const std::string &value);
  void Subscribe(ControlSubscriberInterface *sub);
  void PerformDrawExperminents() const;
  void OnContext(void *context);
  void OnBindTexture(int target, void *bind_texture_fn);
  void OnLinkProgram(int prog, void *link_program_fn);
  void OnUseProgram(int prog, void *use_program_fun);
  void PerformSimpleShaderExperiment(void *use_program_fn);
 private:
  void Publish();

  class ProgramKey {
   public:
    ProgramKey(void *c, int p) : context(c), program(p) {}
    void* context;
    int program;
    bool operator<(const ProgramKey&o) const;
  };

  bool m_scissorEnabled;
  bool m_2x2TextureEnabled;
  std::map<void *, int> m_2x2Textures;
  bool m_simpleShaderEnabled;
  void* m_current_context;
  ControlSubscriberInterface *m_subscriber;
  std::map<ProgramKey, int> m_program_to_simple_shader;
  int m_simpleShader;
  mutable Mutex m_protect;
};

}  // namespace Grafips

#endif  // CONTROLS_GFAPI_CONTROL_H_
