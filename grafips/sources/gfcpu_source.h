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

#ifndef SOURCES_GFCPU_SOURCE_H_
#define SOURCES_GFCPU_SOURCE_H_

#include <set>
#include <vector>

#include "sources/gfimetric_source.h"
#include "remote/gfpublisher.h"
#include "os/gfthread.h"
#include "os/gfmutex.h"

namespace Grafips {

class CpuSource : public MetricSourceInterface, public Thread {
 public:
  void stop();

  CpuSource();
  ~CpuSource();
  void GetDescriptions(std::vector<MetricDescription> *descriptions);
  void Enable(int id);
  void Disable(int id);
  void Poll();
  void Run();

  friend class CpuSourceFixture;

  MetricSinkInterface *MetricSink() { return m_metric_sink; }
  void SetMetricSink(MetricSinkInterface *p);

 private:
  struct CpuLine {
    CpuLine() : user(0), nice(0), system(0), idle(0),
                iowait(0), irq(0), softirq(0), steal(0),
                guest(0), guest_nice(0), utilization(0) {}

    int user;
    int nice;
    int system;
    int idle;
    int iowait;
    int irq;
    int softirq;
    int steal;
    int guest;
    int guest_nice;
    float utilization;
  };

  bool IsEnabled() const;
  void Refresh();
  void ParseCpuLine(CpuLine *dest, char **savePtr);
  void Publish(unsigned int ms);

  // file handle for /proc/stat
  int m_cpu_info_handle;

  // data structures to store the parsed line
  CpuLine m_systemStats;
  std::vector<CpuLine> m_core_stats;

  // stable buffer for reading
  std::vector<char> m_buf;

  // receives updates
  MetricSinkInterface *m_metric_sink;

  // tracks subscriptions
  std::set<int> m_enabled_cores;

  // translates metric ids to offsets
  int m_sysId;
  std::vector<int> m_ids;

  // rate limits publication.  cpu metrics in sysfs are not accurate when
  // polled faster than 500ms interval
  unsigned int m_last_publish_ms;

  bool m_running;
  Mutex m_protect;
};
}  // namespace Grafips

#endif  // SOURCES_GFCPU_SOURCE_H_
