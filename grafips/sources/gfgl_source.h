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

#ifndef SOURCES_GFGL_SOURCE_H_
#define SOURCES_GFGL_SOURCE_H_

#include <set>

#include "sources/gfimetric_source.h"

namespace Grafips {
class MetricSinkInterface;

// GlSource produces metrics based on the GL API
class GlSource : public MetricSourceInterface {
 public:
  explicit GlSource(int ms_interval = 100);
  ~GlSource();
  void Subscribe(MetricSinkInterface *sink);
  void Activate(int id);
  void Deactivate(int id);
  void glSwapBuffers();
 private:
  void GetDescriptions(MetricDescriptionSet *descriptions);

  MetricSinkInterface *m_sink;
  uint64_t m_last_time_ns;
  int m_frame_count, m_ms_interval;
  std::set<int> m_active_ids;
};
}  // end namespace Grafips
#endif  // SOURCES_GFGL_SOURCE_H_
