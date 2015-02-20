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

#ifndef SOURCES_GFCPU_CLOCK_SOURCE_H_
#define SOURCES_GFCPU_CLOCK_SOURCE_H_

#include <map>
#include <set>
#include <vector>

#include "sources/gfimetric_source.h"

// TODO(majanes) this control uses sysfs, which is apparently
// unreliable for accessing cpu frequency configuration, especially on
// new platforms.  Kristin Accardi recommended using the MSRs to get
// and modify this information.  See turbostat in the linux sources.

namespace Grafips {
class MetricSinkInterface;

// GlSource produces metrics based on the GL API
class CpuFreqSource : public MetricSourceInterface {
 public:
  CpuFreqSource();
  ~CpuFreqSource();
  void Subscribe(MetricSinkInterface *sink);
  void Enable(int id);
  void Disable(int id);
  void Poll();
 private:
  MetricSinkInterface *m_sink;
  unsigned int m_last_publish_ms;
  int m_frame_count;
  std::set<int> m_enabled_ids;
  std::vector<int> m_core_freq_handles;
  // reverse lookup for enable
  std::map<int, int> m_index_to_id;
  MetricDescriptionSet m_descriptions;
  static const int BUF_SIZE = 100;
  char m_buf[BUF_SIZE];
};
}  // end namespace Grafips

#endif  // SOURCES_GFCPU_CLOCK_SOURCE_H_
