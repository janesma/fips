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

#ifndef SOURCES_GFPROC_SELF_SOURCE_H_
#define SOURCES_GFPROC_SELF_SOURCE_H_

#include <stdio.h>
#include <linux/limits.h>

#include <vector>

#include "sources/gfimetric_source.h"

namespace Grafips {
class StatLine;
class ProcSelfSource : public MetricSourceInterface {
 public:
  ProcSelfSource();
  ~ProcSelfSource();
  void Subscribe(MetricSinkInterface *sink);
  void Activate(int id);
  void Deactivate(int id);
  void Poll();

 private:
  uint64_t GetUptime() const;
  uint64_t m_uptime;
  int m_hz;  // ticks per sec
  int m_last_publish;
  int m_rss_id, m_cpu_id;
  StatLine *m_stat;
  MetricSinkInterface *m_metric_sink;
  MetricDescriptionSet m_descriptions;
  bool m_rss_active, m_cpu_active;
};
}  // namespace Grafips

#endif  // SOURCES_GFPROC_SELF_SOURCE_H_
