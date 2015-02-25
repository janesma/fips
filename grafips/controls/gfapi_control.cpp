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

#include <GL/gl.h>
#include <assert.h>

#include <string>
#include <vector>

#include "error/gflog.h"

using Grafips::ApiControl;
using Grafips::ScopedLock;

ApiControl::ApiControl() : m_scissorEnabled(false),
                           m_2x2TextureEnabled(false),
                           m_2x2Texture(0),
                           m_subscriber(NULL) {
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
      // m_2x2TextureEnabled = true;
    } else {
      GFLOG("ApiControl 2x2TextureExperiment: false");
      // m_2x2TextureEnabled = false;
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
ApiControl::PerformBindTextureExperiment(int target, void *bind_texture_fn) {
  ScopedLock s(&m_protect);
  if (!m_2x2TextureEnabled)
    return;

  if (!bind_texture_fn) {
    GFLOG("ApiControl null bind texture function");
    return;
  }

  if (!m_2x2Texture) {
    unsigned char data[] {
      255, 0, 0, 0,
          0, 255, 0, 0,
          0, 0, 255, 0,
          0, 0, 0, 0
          };
    GLuint t;
    glGenTextures(1, &t);
    m_2x2Texture = t;

    (*(glBindTexture_fn)bind_texture_fn)(GL_TEXTURE_2D, m_2x2Texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 2, 2, 0, GL_RGBA,
                 GL_UNSIGNED_INT_8_8_8_8, data);
    if (glGetError() != GL_NO_ERROR)
      GFLOG("teximag2d fail");
  }

  (*(glBindTexture_fn)bind_texture_fn)(target, m_2x2Texture);
    if (glGetError() != GL_NO_ERROR)
      GFLOG("bind fail");
}
