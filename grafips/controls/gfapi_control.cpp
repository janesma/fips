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

#include "controls/gfapi_control.h"

#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>
#include <assert.h>
#include <string.h>

#include <string>
#include <vector>

#include "sources/gfgpu_perf_functions.h"
#include "error/gflog.h"

using Grafips::ApiControl;
using Grafips::ScopedLock;
using Grafips::PerfFunctions;

ApiControl::ApiControl() : m_scissorEnabled(false),
                           m_2x2TextureEnabled(false),
                           m_simpleShaderEnabled(false),
                           m_disableDraw(false),
                           m_wireframeEnabled(false),
                           m_current_context(0),
                           m_subscriber(NULL),
                           m_simpleShader(0) {
}

ApiControl::~ApiControl() {
}

void
ApiControl::Set(const std::string &key, const std::string &value) {
  ScopedLock s(&m_protect);
  if (key == "ScissorExperiment") {
    if (value == "true") {
      GFLOG("ApiControl ScissorExperiment: true");
      m_scissorEnabled = true;
    } else {
      GFLOG("ApiControl ScissorExperiment: false");
      m_scissorEnabled = false;
    }
  } else if (key == "2x2TextureExperiment") {
    // disable for now, it doesn't work
    if (value == "true") {
      GFLOG("ApiControl 2x2TextureExperiment: true");
      m_2x2TextureEnabled = true;
    } else {
      GFLOG("ApiControl 2x2TextureExperiment: false");
      m_2x2TextureEnabled = false;
    }
  } else if (key == "SimpleShaderExperiment") {
    if (value == "true") {
      GFLOG("ApiControl SimpleShaderExperiment: true");
      m_simpleShaderEnabled = true;
    } else {
      GFLOG("ApiControl SimpleShaderExperiment: false");
      m_simpleShaderEnabled = false;
    }
  } else if (key == "DisableDrawExperiment") {
    if (value == "true") {
      GFLOG("ApiControl DisableDrawExperiment: true");
      m_disableDraw = true;
    } else {
      GFLOG("ApiControl DisableDrawExperiment: false");
      m_disableDraw = false;
    }
  } else if (key == "WireframeExperiment") {
    if (value == "true") {
      GFLOG("ApiControl WireframeExperiment: true");
      m_wireframeEnabled = true;
    } else {
      GFLOG("ApiControl WireframeExperiment: false");
      m_wireframeEnabled = false;
    }
  } else {
    // key is not meant for this control
    return;
  }
  Publish();
}

void
ApiControl::Subscribe(ControlSubscriberInterface *sub) {
  ScopedLock s(&m_protect);
  m_subscriber = sub;
  Publish();
}

void
ApiControl::Publish() {
  if (!m_subscriber) {
    GFLOG("ApiControl Publish without subscriber");
    return;
  }
  m_subscriber->OnControlChanged("ScissorExperiment",
                                 m_scissorEnabled ? "true" : "false");
  m_subscriber->OnControlChanged("2x2TextureExperiment",
                                 m_2x2TextureEnabled ? "true" : "false");
  m_subscriber->OnControlChanged("SimpleShaderExperiment",
                                 m_simpleShaderEnabled ? "true" : "false");
}

class Grafips::ScissorExperiment {
 public:
  ScissorExperiment() : m_scissor_box(4) {
    m_enabled = PerfFunctions::IsEnabled(GL_SCISSOR_TEST);
    if (m_enabled)
      PerfFunctions::GetIntegerv(GL_SCISSOR_BOX, m_scissor_box.data());
  }
  ~ScissorExperiment() {
    if (!m_enabled) {
      PerfFunctions::Disable(GL_SCISSOR_TEST);
      return;
    }
    PerfFunctions::Scissor(m_scissor_box[0], m_scissor_box[1],
                           m_scissor_box[2], m_scissor_box[3]);
  }
  void Override() {
    PerfFunctions::Enable(GL_SCISSOR_TEST);
    PerfFunctions::Scissor(0, 0, 1, 1);
  }
 private:
  bool m_enabled;
  std::vector<int> m_scissor_box;
};

class Grafips::WireframeExperiment {
 public:
  WireframeExperiment() {
    PerfFunctions::GetIntegerv(GL_POLYGON_MODE, m_front_and_back);
    assert(m_front_and_back[0] == m_front_and_back[1]);
  }
  ~WireframeExperiment() {
    PerfFunctions::PolygonMode(GL_FRONT_AND_BACK, m_front_and_back[0]);
  }
  void Override() {
    PerfFunctions::PolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  }

 private:
  GLint m_front_and_back[2];
};

static void
CheckError(const char * file, int line) {
  const int error = PerfFunctions::GetError();
  if ( error == GL_NO_ERROR)
    return;
  GFLOGF("ERROR: %x %s:%i\n", error, file, line);
}
#define GL_CHECK() CheckError(__FILE__, __LINE__)

class Grafips::TextureExperiment {
 public:
  TextureExperiment() {
    PerfFunctions::GetError();
    PerfFunctions::GetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &m_texture_units);

    int prev_active_texture_unit;
    PerfFunctions::GetIntegerv(GL_ACTIVE_TEXTURE, &prev_active_texture_unit);
    GL_CHECK();

    PerfFunctions::Enable(GL_TEXTURE_2D);
    GL_CHECK();
    unsigned char data[] {255, 0, 0, 0, 0, 255, 0, 0,
          0, 0, 255, 0, 0, 0, 0, 255};

    PerfFunctions::ActiveTexture(GL_TEXTURE0);
    GL_CHECK();

    int prev_bound_texture;
    PerfFunctions::GetIntegerv(GL_TEXTURE_BINDING_2D, &prev_bound_texture);
    GL_CHECK();

    PerfFunctions::GenTextures(1, reinterpret_cast<GLuint*>(&m_texture_handle));
    GL_CHECK();

    PerfFunctions::BindTexture(GL_TEXTURE_2D, m_texture_handle);
    GL_CHECK();
    PerfFunctions::TexParameteri(GL_TEXTURE_2D,
                                 GL_TEXTURE_MIN_FILTER,
                                 GL_NEAREST);
    GL_CHECK();
    PerfFunctions::TexParameteri(GL_TEXTURE_2D,
                                 GL_TEXTURE_MAG_FILTER,
                                 GL_NEAREST);
    GL_CHECK();
    PerfFunctions::TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2, 2, 0, GL_RGBA,
                              GL_UNSIGNED_INT, data);
    GL_CHECK();
    PerfFunctions::BindTexture(GL_TEXTURE_2D, prev_bound_texture);
    GL_CHECK();

    PerfFunctions::ActiveTexture(prev_active_texture_unit);
    GL_CHECK();
  }
  void Override() {
    // simply bind the 2x2 texture to every unit
    // TODO(majanes): needs to happen for cube maps also
    PerfFunctions::GetError();
    int prev_active_texture_unit;
    PerfFunctions::GetIntegerv(GL_ACTIVE_TEXTURE, &prev_active_texture_unit);
    GL_CHECK();
    for (int i = GL_TEXTURE0; i < GL_TEXTURE0 + m_texture_units; ++i) {
      PerfFunctions::ActiveTexture(i);
      GL_CHECK();
      PerfFunctions::BindTexture(GL_TEXTURE_2D, m_texture_handle);
      GL_CHECK();
    }
    PerfFunctions::ActiveTexture(prev_active_texture_unit);
    GL_CHECK();
  }

 private:
  int m_texture_handle, m_texture_units;
};

bool
ApiControl::PerformDrawExperiments() {
  ScopedLock s(&m_protect);
  if (m_disableDraw)
    return false;

  if (m_scissorEnabled) {
    ScissorExperiment *scissor = m_scissor_overrides[m_current_context];
    if (scissor == NULL) {
      scissor = new ScissorExperiment;
      m_scissor_overrides[m_current_context] = scissor;
    }
    scissor->Override();
  } else {
    ScissorExperiment *scissor = m_scissor_overrides[m_current_context];
    if (scissor != NULL) {
      delete scissor;
      m_scissor_overrides[m_current_context] = NULL;
    }
  }

  if (m_wireframeEnabled) {
    WireframeExperiment *wf = m_wireframe_overrides[m_current_context];
    if (wf == NULL) {
      wf = new WireframeExperiment;
      m_wireframe_overrides[m_current_context] = wf;
    }
    wf->Override();
  } else {
    WireframeExperiment *wf = m_wireframe_overrides[m_current_context];
    if (wf != NULL) {
      delete wf;
      m_wireframe_overrides[m_current_context] = NULL;
    }
  }

  if (m_2x2TextureEnabled) {
    if (m_2x2Textures[m_current_context] == NULL)
      m_2x2Textures[m_current_context] = new TextureExperiment;
    m_2x2Textures[m_current_context]->Override();
  }

  return true;
}

void
ApiControl::OnLinkProgram(int prog) {
  ScopedLock s(&m_protect);
  GFLOGF("OnLinkProgram: %d", prog);
  PerfFunctions::GetError();
  if (m_simpleShader == 0) {
    m_simpleShader = PerfFunctions::CreateShader(GL_FRAGMENT_SHADER);
    GFLOGF("OnLinkProgram created shader: %d", m_simpleShader);
    GL_CHECK();

    static const char * fshader_simple =
        "void main(void) {\n"
        "  gl_FragColor = vec4(1,0,1,1);\n"
        "}";
    const int len = strlen(fshader_simple);

    PerfFunctions::ShaderSource(m_simpleShader, 1, &fshader_simple, &len);
    GL_CHECK();
    PerfFunctions::CompileShader(m_simpleShader);
    GL_CHECK();
  }

  // TODO(majanes) track contexts
  // assert(m_program_to_simple_shader.find(prog) ==
  //        m_program_to_simple_shader.end());

  GL_CHECK();
  const int simple_prog = PerfFunctions::CreateProgram();
  GL_CHECK();
  GFLOGF("OnLinkProgram created program: %d", simple_prog);

  int shader_count;
  PerfFunctions::GetProgramiv(prog, GL_ATTACHED_SHADERS, &shader_count);
  GL_CHECK();
  GFLOGF("OnLinkProgram shader count: %d", shader_count);
  std::vector<unsigned int> shaders(shader_count);
  PerfFunctions::GetAttachedShaders(prog, shader_count, NULL, shaders.data());
  GL_CHECK();
  for (auto shaderp = shaders.begin(); shaderp != shaders.end(); ++shaderp) {
    int shader_type;
    PerfFunctions::GetShaderiv(*shaderp, GL_SHADER_TYPE, &shader_type);
    GL_CHECK();
    if (shader_type == GL_FRAGMENT_SHADER) {
      GFLOGF("OnLinkProgram found fragment shader: %d", *shaderp);
      continue;
    }
    GFLOGF("OnLinkProgram found other shader: %d", *shaderp);
    PerfFunctions::AttachShader(simple_prog, *shaderp);
    GL_CHECK();
    GFLOGF("OnLinkProgram attached prog:%d, shader:%d", simple_prog, *shaderp);
  }
  PerfFunctions::AttachShader(simple_prog, m_simpleShader);
  GL_CHECK();
  GFLOGF("OnLinkProgram attached prog:%d, shader:%d",
         simple_prog,
         m_simpleShader);

  PerfFunctions::LinkProgram(simple_prog);
  GL_CHECK();
  m_program_to_simple_shader[ProgramKey(m_current_context, prog)] = simple_prog;
  GFLOGF("Linking %d to %d", prog, simple_prog);
}

void
ApiControl::OnUseProgram(int prog) {
  if (!prog)
    return;

  ScopedLock s(&m_protect);
  // GFLOGF("OnUseProgram: %d", prog);
  if (!m_simpleShaderEnabled)
    return;

  PerfFunctions::GetError();
  auto simple_prog =
      m_program_to_simple_shader.find(ProgramKey(m_current_context, prog));
  assert(simple_prog != m_program_to_simple_shader.end());
  // GFLOGF("Using %d instead of %d", simple_prog->second, prog);
  PerfFunctions::UseProgram(simple_prog->second);
  GL_CHECK();
}

void
ApiControl::OnContext(void* context) {
  ScopedLock s(&m_protect);
  GFLOGF("OnContext %d", context);
  m_current_context = context;

  if (m_scissor_overrides.find(context) == m_scissor_overrides.end())
    m_scissor_overrides[context] = NULL;
  if (m_wireframe_overrides.find(context) == m_wireframe_overrides.end())
    m_wireframe_overrides[context] = NULL;
  if (m_2x2Textures.find(m_current_context) == m_2x2Textures.end())
    m_2x2Textures[m_current_context] = NULL;
}

bool
ApiControl::ProgramKey::operator<(const ProgramKey&o) const {
  if (context < o.context)
    return true;
  if (o.context < context)
    return false;
  return program < o.program;
}


