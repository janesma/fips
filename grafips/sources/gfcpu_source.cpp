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

#include "sources/gfcpu_source.h"

#include <assert.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <sstream>
#include <vector>

#include "remote/gfpublisher.h"

using Grafips::CpuSource;

static const int READ_BUF_SIZE = 4096;

CpuSource::CpuSource() : m_metric_sink(NULL),
                         m_last_publish_ms(0),
                         m_running(true) {
    m_cpu_info_handle = open("/proc/stat", O_RDONLY);
    m_buf.resize(READ_BUF_SIZE);
    Refresh();
}

CpuSource::~CpuSource() {
    close(m_cpu_info_handle);
}

void
CpuSource::Refresh() {
    // seek will refresh the data in the proc file
    lseek(m_cpu_info_handle, 0, SEEK_SET);

    const ssize_t bytes = read(m_cpu_info_handle, m_buf.data(), m_buf.size());
    assert(bytes > 0);
    m_buf[bytes] = '\0';

    char *linesaveptr, *wordsaveptr;
    char *line = strtok_r(m_buf.data(), "\n", &linesaveptr);
    assert(line != NULL);
    while (true) {
        // parse line
        char *word = strtok_r(line, " ", &wordsaveptr);
        assert(word != NULL);
        if (0 != strncmp("cpu", word, 3))
            // cpu lines expected at front of /proc/stat
            break;

        if (0 == strncmp("cpu", word, 5)) {
            ParseCpuLine(&m_systemStats, &wordsaveptr);
        } else {
            unsigned int core = atoi(&(word[3]));
            if (core >= m_core_stats.size())
                m_core_stats.resize(core + 1);
            ParseCpuLine(&(m_core_stats[core]), &wordsaveptr);
        }

        // advance to next line
        line = strtok_r(NULL, "\n", &linesaveptr);
    }
}

void
CpuSource::ParseCpuLine(CpuLine *dest, char **savePtr) {
    static const int COUNT_STAT_ITEMS = 10;
    CpuLine current;
    int *current_item = &(current.user);

    for (int i = 0; i < COUNT_STAT_ITEMS; ++i, ++current_item) {
        const char *num = strtok_r(NULL, " ", savePtr);
        assert(num != NULL);
        *current_item = atoi(num);
    }

    CpuLine delta;
    int *prev_item = &(dest->user);
    int *delta_item = &(delta.user);
    current_item = &(current.user);

    for (int i = 0; i < COUNT_STAT_ITEMS;
         ++i, ++current_item, ++prev_item, ++delta_item) {
        *delta_item = *current_item - *prev_item;
    }

    const float active = delta.user + delta.nice + delta.system +
                         delta.irq + delta.softirq + delta.guest +
                         delta.guest_nice;
    const float total = active + delta.idle + delta.iowait + delta.steal;
    if (total == 0) {
        current.utilization = 0;
    } else {
        current.utilization = active / total * 100.0;
        assert(current.utilization <= 100);
        assert(current.utilization >= 0);
    }
    memcpy(dest, &current, sizeof(CpuLine));
}

bool
CpuSource::IsActivated() const {
    return !m_active_cores.empty();
}

void
CpuSource::Subscribe(MetricSinkInterface *sink) {
  m_metric_sink = sink;
  std::vector<MetricDescription> descriptions;
  {
    ScopedLock s(&m_protect);
    GetDescriptions(&descriptions);
  }
  sink->OnDescriptions(descriptions);
}


void
CpuSource::GetDescriptions(std::vector<MetricDescription> *descriptions) {
  descriptions->push_back(MetricDescription("cpu/system/utilization",
                                            "Displays percent cpu "
                                            "activity for the system",
                                            "CPU Busy", GR_METRIC_PERCENT));
  m_sysId = descriptions->back().id();

  for (unsigned int i = 0; i < m_core_stats.size(); ++i) {
    std::stringstream s, name;
    s << "cpu/core/" << i << "/utilization";
    name << "CPU Core " << i << " Busy";
    descriptions->push_back(MetricDescription(s.str(),
                                              "Displays percent cpu "
                                              "activity for the core",
                                              name.str(),
                                              GR_METRIC_PERCENT));
    if (m_ids.size() <= i)
      m_ids.push_back(descriptions->back().id());
  }
}

void
CpuSource::Activate(int id) {
  ScopedLock s(&m_protect);
    if (id == m_sysId) {
        m_active_cores.insert(-1);
        return;
    }
    for (unsigned int i = 0; i < m_ids.size(); ++i) {
        if (m_ids[i] == id) {
            m_active_cores.insert(i);
            return;
        }
    }
}

void
CpuSource::Deactivate(int id) {
  ScopedLock s(&m_protect);
    if (id == m_sysId) {
        m_active_cores.erase(-1);
        return;
    }
    for (unsigned int i = 0; i < m_ids.size(); ++i) {
        if (m_ids[i] == id) {
            m_active_cores.erase(i);
            return;
        }
    }
}

void
CpuSource::Poll() {
  ScopedLock s(&m_protect);
    if (!IsActivated())
        return;

    const unsigned int ms = get_ms_time();
    if (ms - m_last_publish_ms < 500)
        return;
    m_last_publish_ms = ms;

    Refresh();
    Publish(ms);
}

void
CpuSource::Publish(unsigned int ms) {
    if (!m_metric_sink)
        return;

    DataSet d;

    if (m_active_cores.count(-1) != 0)
        d.push_back(DataPoint(ms, m_sysId, m_systemStats.utilization));

    for (unsigned int i = 0; i < m_ids.size(); ++i) {
        if (m_active_cores.count(i) == 0)
            continue;
        d.push_back(DataPoint(ms, m_ids[i], m_core_stats[i].utilization));
    }
    m_metric_sink->OnMetric(d);
}

