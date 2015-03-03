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

#include <GLES2/gl2.h>
#include <assert.h>
#include <string.h>

#include <string>
#include <vector>

#include "error/gflog.h"

using Grafips::ApiControl;
using Grafips::ScopedLock;

ApiControl::ApiControl() : m_scissorEnabled(false),
                           m_2x2TextureEnabled(false),
                           m_2x2Texture(0),
                           m_simpleShaderEnabled(false),
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
}

void
ApiControl::PerformDrawExperminents() const {
  ScopedLock s(&m_protect);
  if (m_scissorEnabled) {
    glEnable(GL_SCISSOR_TEST);
    glScissor(0, 0, 1, 1);
  }
}

typedef void (*glBindTexture_fn)( GLenum target, GLuint texture );

void
ApiControl::OnBindTexture(int target, void *bind_texture_fn) {
  ScopedLock s(&m_protect);
  if (!m_2x2TextureEnabled)
    return;

  if (!bind_texture_fn) {
    GFLOG("ApiControl null bind texture function");
    return;
  }

  if (!m_2x2Texture) {
    glEnable(GL_TEXTURE_2D);
    unsigned char data[] {255, 0, 0, 0, 0, 255, 0, 0,
          0, 0, 255, 0, 0, 0, 0, 255};
    glGenTextures(1, reinterpret_cast<GLuint*>(&m_2x2Texture));

    (*(glBindTexture_fn)bind_texture_fn)(GL_TEXTURE_2D, m_2x2Texture);
    glActiveTexture(GL_TEXTURE0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2, 2, 0, GL_RGBA,
                 GL_UNSIGNED_INT, data);
    if (glGetError() != GL_NO_ERROR)
      GFLOG("teximag2d fail");
    (*(glBindTexture_fn)bind_texture_fn)(GL_TEXTURE_2D, 0);
  }

  (*(glBindTexture_fn)bind_texture_fn)(target, m_2x2Texture);
}

typedef void (*glLinkProgram_fn)(GLuint program);

static void
CheckError(const char * file, int line) {
  const int error = glGetError();
  if ( error == GL_NO_ERROR)
    return;
  printf("ERROR: %x %s:%i\n", error, file, line);
  exit(-1);
}

#define GL_CHECK() CheckError(__FILE__, __LINE__)

void
ApiControl::OnLinkProgram(int prog, void *link_program_fn) {
  return;
  ScopedLock s(&m_protect);
  GFLOGF("OnLinkProgram: %d", prog);
  if (m_simpleShader == 0) {
    m_simpleShader = glCreateShader(GL_FRAGMENT_SHADER);
    GL_CHECK();

    static const char * fshader_simple =
        "void main(void) {\n"
        "  gl_FragColor = vec4(1,0,1,1);\n"
        "}";
    const int len = strlen(fshader_simple);

    glShaderSource(m_simpleShader, 1, &fshader_simple, &len);
    GL_CHECK();
  }

  // TODO(majanes) track contexts
  // assert(m_program_to_simple_shader.find(prog) ==
  //        m_program_to_simple_shader.end());

  const int simple_prog = glCreateProgram();
  GL_CHECK();

  int shader_count;
  glGetProgramiv(prog, GL_ATTACHED_SHADERS, &shader_count);
  GL_CHECK();
  std::vector<unsigned int> shaders(shader_count);
  glGetAttachedShaders(prog, shader_count, NULL, shaders.data());
  for (auto shaderp = shaders.begin(); shaderp != shaders.end(); ++shaderp) {
    int shader_type;
    glGetShaderiv(*shaderp, GL_SHADER_TYPE, &shader_type);
    GL_CHECK();
    if (shader_type == GL_FRAGMENT_SHADER) {
      GFLOGF("OnLinkProgram found fragment shader: %d", *shaderp);
      continue;
    }
    glAttachShader(simple_prog, *shaderp);
    GL_CHECK();
  }
  glAttachShader(simple_prog, m_simpleShader);
  GL_CHECK();
  (*(glLinkProgram_fn)link_program_fn)(simple_prog);
  GL_CHECK();
  m_program_to_simple_shader[prog] = simple_prog;
  GFLOGF("Linking %d to %d", prog, simple_prog);
}

typedef void (*glUseProgram_fn)(GLuint program);
void
ApiControl::OnUseProgram(int prog, void *use_program_fun) {
  if (!prog)
    return;

  ScopedLock s(&m_protect);
  GFLOGF("OnUseProgram: %d", prog);
  // if (!m_simpleShaderEnabled)
  //   return;

  auto simple_prog = m_program_to_simple_shader.find(prog);
  assert(simple_prog != m_program_to_simple_shader.end());
  GFLOGF("Using %d", simple_prog->second);
  //(*(glUseProgram_fn)use_program_fun)(simple_prog->second);
  GL_CHECK();
}

void
ApiControl::OnContext(int context) {
  GFLOGF("OnContext %d", context);
}
