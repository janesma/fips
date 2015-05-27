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

#include "sources/gfproc_self_source.h"

#include <assert.h>
#include <unistd.h>
#include <inttypes.h>

#include "remote/gfimetric_sink.h"

using Grafips::ProcSelfSource;
using Grafips::StatLine;

class StatLine {
 public:
  uint64_t utime;
  uint64_t stime;
  int64_t cutime;
  int64_t cstime;
  int64_t rss;

  void Parse();
};

void
StatLine::Parse() {
  FILE *fh = fopen("/proc/self/stat", "r");
  fscanf(fh, "%*d (%*s %*c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u %"
         SCNu64 " %" SCNu64 " %" SCNd64 " %" SCNd64 " %*d %*d %*d "
         "%*d %*u %*u %" SCNd64 " %*u %*u %*u %*u %*u %*u %*u %*u "
         "%*u %*u %*u %*u %*u %*d %*d %*u %*u %*u %*u %*d %*u %*u %*u "
         "%*u %*u %*u %*u %*d",
         &utime, &stime, &cutime, &cstime, &rss);
  fclose(fh);
}


uint64_t
ProcSelfSource::GetUptime() const {
  FILE* fh = fopen("/proc/uptime", "r");
  fseek(fh, 0, SEEK_SET);
  uint64_t seconds, centiseconds;
  fscanf(fh, "%" SCNu64 ".%" SCNu64, &seconds, &centiseconds);
  fclose(fh);
  return (seconds * m_hz) + (centiseconds * m_hz / 100);
}

ProcSelfSource::ProcSelfSource()
    : m_uptime(0),
      m_hz(0),
      m_last_publish(0),
      m_rss_id(0), m_cpu_id(0),
      m_stat(new StatLine),
      m_metric_sink(NULL),
      m_rss_active(false), m_cpu_active(false) {
  m_descriptions.push_back(
      MetricDescription("/proc/pid/rss",
                        "Resident set size: number  of pages the "
                        "process has in real memory.",
                        "Memory RSS",
                        GR_METRIC_COUNT));
  m_rss_id = m_descriptions.back().id();
  m_descriptions.push_back(MetricDescription("/proc/pid/utilization",
                                             "Process CPU Utilization",
                                             "Process %CPU",
                                             GR_METRIC_PERCENT));
  m_cpu_id = m_descriptions.back().id();

  int64_t ticks = sysconf(_SC_CLK_TCK);
  assert(ticks != -1);
  m_hz = static_cast<unsigned int>(ticks);

  m_stat->Parse();
  m_uptime = GetUptime();
  m_last_publish = get_ms_time();
}

ProcSelfSource::~ProcSelfSource() {
  delete m_stat;
}

void
ProcSelfSource::Subscribe(MetricSinkInterface *sink) {
  m_metric_sink = sink;
  sink->OnDescriptions(m_descriptions);
}

void
ProcSelfSource::Activate(int id) {
  if (id == m_cpu_id)
    m_cpu_active = true;
  else if (id == m_rss_id)
    m_rss_active = true;
}

void
ProcSelfSource::Deactivate(int id) {
  if (id == m_cpu_id)
    m_cpu_active = false;
  else if (id == m_rss_id)
    m_rss_active = false;
}

void
ProcSelfSource::Poll() {
  if (!m_rss_active && !m_cpu_active)
    return;

  int current_time = get_ms_time();
  if (current_time - m_last_publish < 300)
    return;

  DataSet d;
  m_last_publish = current_time;

  const uint64_t last_usage = m_stat->utime + m_stat->stime +
                              m_stat->cutime + m_stat->cstime;
  m_stat->Parse();
  if (m_cpu_active) {
    const uint64_t current_usage = m_stat->utime + m_stat->stime +
                                   m_stat->cutime + m_stat->cstime;
    const float elapsed_usage = current_usage - last_usage;
    const uint64_t uptime = GetUptime();
    const float elapsed_uptime = uptime - m_uptime;

    const float utilization = elapsed_usage / elapsed_uptime;
    m_uptime = uptime;
    d.push_back(DataPoint(current_time, m_cpu_id, utilization * 100.0));
  }

  if (m_rss_active) {
    d.push_back(DataPoint(current_time, m_rss_id, m_stat->rss));
  }

  m_metric_sink->OnMetric(d);
}
