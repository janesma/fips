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

#include "sources/gfcpu_clock_source.h"

#include <dirent.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <sstream>
#include <string>

#include "remote/gfimetric_sink.h"

using Grafips::CpuFreqSource;

// Convenience class to clean up DIR resources on exit;
class ScopedDirClose {
 public:
  explicit ScopedDirClose(DIR *d) : m_d(d) {}
  ~ScopedDirClose() { closedir(m_d); }
 private:
  DIR *m_d;
};

CpuFreqSource::CpuFreqSource()
    : m_sink(NULL), m_last_publish_ms(0) {
  // open handles
  static const char *base_dir = "/sys/devices/system/cpu";
  DIR * base_dir_h = opendir(base_dir);
  if (!base_dir_h)
    return;
  ScopedDirClose d(base_dir_h);

  struct dirent *current_cpu_dir = readdir(base_dir_h);
  while (NULL != current_cpu_dir) {
    if (strncmp("cpu", current_cpu_dir->d_name, 3) == 0) {
      const std::string core_file_name = std::string(base_dir) + "/"
                                        + current_cpu_dir->d_name
                                        + "/cpufreq/cpuinfo_cur_freq";

      const int core_freq_handle = open(core_file_name.c_str(), O_RDONLY);
      if (core_freq_handle == -1)
        break;
      m_core_freq_handles.push_back(core_freq_handle);
    }

    current_cpu_dir = readdir(base_dir_h);
  }
  for (unsigned int i = 0; i < m_core_freq_handles.size(); ++i) {
    std::stringstream path, name;
    path << "cpu/core/" << i << "/frequency";
    name << "CPU" << i << " frequency";
    static const char *desc = "Displays current frequency of core";

    m_descriptions.push_back(MetricDescription(path.str(),
                                               name.str(),
                                               desc,
                                               GR_METRIC_RATE));
    m_index_to_id[i] = m_descriptions.back().id();
  }
}

CpuFreqSource::~CpuFreqSource() {
  for (auto handle = m_core_freq_handles.begin();
       handle != m_core_freq_handles.end(); ++handle)
    close(*handle);
}

void
CpuFreqSource::Subscribe(MetricSinkInterface *sink) {
  m_sink = sink;
  sink->OnDescriptions(m_descriptions);
}

void
CpuFreqSource::Enable(int id) {
  m_enabled_ids.emplace(id);
}

void
CpuFreqSource::Disable(int id) {
  auto i = m_enabled_ids.find(id);
  if (i == m_enabled_ids.end())
    return;
  m_enabled_ids.erase(i);
}

void
CpuFreqSource::Poll() {
  const unsigned int ms = get_ms_time();
  if (ms - m_last_publish_ms < 500)
    return;
  if (!m_sink)
    return;
  m_last_publish_ms = ms;

  DataSet dset;
  for (unsigned int i = 0; i < m_core_freq_handles.size(); ++i) {
    const int id = m_index_to_id[i];
    if (m_enabled_ids.find(id) == m_enabled_ids.end())
      continue;

    const int fh = m_core_freq_handles[i];
    lseek(fh, 0, SEEK_SET);
    const ssize_t bytes = read(fh, m_buf, BUF_SIZE - 1);
    if (bytes < 0)
      continue;
    m_buf[bytes] = '\0';
    int freq = atoi(m_buf);
    dset.push_back(DataPoint(ms, m_index_to_id[i], freq));
  }
  if (!dset.empty() && m_sink)
    m_sink->OnMetric(dset);
}
