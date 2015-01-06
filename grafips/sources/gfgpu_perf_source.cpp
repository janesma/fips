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

#include "sources/gfgpu_perf_source.h"

#include <GL/gl.h>
#include <GL/glext.h>
#include <GL/glx.h>
#include <assert.h>

#include <sstream>
#include <string>
#include <vector>

#include "os/gftraits.h"
#include "remote/gfimetric_sink.h"

using Grafips::GpuPerfSource;
using Grafips::PerfMetricSet;
using Grafips::NoCopy;
using Grafips::NoAssign;
using Grafips::MetricDescriptionSet;
using Grafips::MetricDescription;

namespace {
typedef void (PFNGLGETFIRSTPERFQUERYID) (GLuint *queryId);
typedef void (PFNGLGETNEXTPERFQUERYID) (GLuint queryId,
                                        GLuint *nextQueryId);
typedef void (PFNGLGETPERFQUERYINFOINTEL)
    (GLuint queryId, GLuint queryNameLength, GLchar *queryName,
     GLuint *dataSize, GLuint *noCounters, GLuint *noInstances,
     GLuint *capsMask);

typedef void (PFNGLGETPERFCOUNTERINFOINTEL) (
    GLuint queryId, GLuint counterId, GLuint counterNameLength,
    GLchar *counterName, GLuint counterDescLength,
    GLchar *counterDesc, GLuint *counterOffset,
    GLuint *counterDataSize,
    GLuint *counterTypeEnum, GLuint *counterDataTypeEnum,
    GLuint64 *rawCounterMaxValue);

class PerfMetric : public NoCopy, NoAssign {
 public:
  PerfMetric(int query_id, int counter_num);
  ~PerfMetric() {}
  void AppendDescription(MetricDescriptionSet *descriptions);
  bool Enable(int id);
  bool Disable(int id);
 private:
  const int m_query_id, m_counter_num;
  GLuint m_offset, m_data_size, m_type,
    m_data_type;
  GLuint64 m_max_value;
  std::string m_name, m_description;
  MetricDescription *m_grafips_desc;
};

class PerfMetricGroup : public NoCopy, NoAssign {
 public:
  explicit PerfMetricGroup(int query_id);
  ~PerfMetricGroup();
  void AppendDescriptions(MetricDescriptionSet *descriptions);
  bool Enable(int id);
  bool Disable(int id);
 private:
  const std::string m_query_name;
  const int m_query_id;
  unsigned int m_data_size;
  unsigned int m_number_counters;
  unsigned int m_capabilities_mask;
  std::vector<PerfMetric *> m_metrics;
};
}  // namespace

class PerfMetricSet : public NoCopy, NoAssign {
 public:
  PerfMetricSet();
  ~PerfMetricSet();
  void GetDescriptions(MetricDescriptionSet *descriptions);
  void Enable(int id);
  void Disable(int id);
 private:
  std::vector<PerfMetricGroup *> m_metric_groups;
  int m_enabled_group;
  int m_enable_count;
};


GpuPerfSource::GpuPerfSource()
    : m_sink(NULL), m_metrics(NULL) {
}

GpuPerfSource::~GpuPerfSource() {
}

void
GpuPerfSource::Subscribe(MetricSinkInterface *sink) {
  m_sink = sink;
  MetricDescriptionSet descriptions;
  GetDescriptions(&descriptions);
  m_sink->OnDescriptions(descriptions);
}

void
GpuPerfSource::GetDescriptions(MetricDescriptionSet *descriptions) {
  if (m_metrics)
    m_metrics->GetDescriptions(descriptions);
}

void
GpuPerfSource::Enable(int id) {
  if (m_metrics)
    m_metrics->Enable(id);
}

void
GpuPerfSource::Disable(int id) {
  if (m_metrics)
    m_metrics->Disable(id);
}

void
GpuPerfSource::MakeContextCurrent() {
  if (m_metrics != NULL)
    return;

  m_metrics = new PerfMetricSet();

  MetricDescriptionSet descriptions;
  GetDescriptions(&descriptions);
  m_sink->OnDescriptions(descriptions);
}

void
GpuPerfSource::glSwapBuffers() {
}


PerfMetricSet::PerfMetricSet() : m_enabled_group(-1), m_enable_count(0) {
  const GLubyte* name =
      reinterpret_cast<const GLubyte*>("glGetFirstPerfQueryIdINTEL");
  void (*proc_addr)() = glXGetProcAddress(name);
  if (proc_addr == NULL)
    return;
  PFNGLGETFIRSTPERFQUERYID *p_glGetFirstPerfQueryIdINTEL =
      reinterpret_cast<PFNGLGETFIRSTPERFQUERYID*>(proc_addr);

  name = reinterpret_cast<const GLubyte*>("glGetNextPerfQueryIdINTEL");
  proc_addr = glXGetProcAddress(name);
  assert(proc_addr != NULL);
  if (proc_addr == NULL)
    return;
  PFNGLGETNEXTPERFQUERYID *p_glGetNextPerfQueryIdINTEL =
      reinterpret_cast<PFNGLGETNEXTPERFQUERYID*>(proc_addr);


  unsigned int query_id = 0;
  (*p_glGetFirstPerfQueryIdINTEL)(&query_id);

  if (query_id == 0)
    return;

  std::vector<unsigned int> query_ids;
  query_ids.push_back(query_id);
  while (true) {
    p_glGetNextPerfQueryIdINTEL(query_id, &query_id);
    if (!query_id)
      break;
    query_ids.push_back(query_id);
  }

  for (auto i = query_ids.begin(); i != query_ids.end(); ++i) {
    m_metric_groups.push_back(new PerfMetricGroup(*i));
  }
}

PerfMetricSet::~PerfMetricSet() {
  while (!m_metric_groups.empty()) {
    delete(m_metric_groups.back());
    m_metric_groups.pop_back();
  }
}

void
PerfMetricSet::Enable(int id) {
  if (m_enabled_group != -1) {
    if (m_metric_groups[m_enabled_group]->Enable(id))
      ++m_enable_count;
    else
      assert(false);
    return;
  }

  for (unsigned int i = 0; i < m_metric_groups.size(); ++i) {
    if (!m_metric_groups[i]->Enable(id))
      continue;
    m_enabled_group = i;
    m_enable_count = 1;
    return;
  }
}

void
PerfMetricSet::Disable(int id) {
  if (m_enabled_group == -1)
    return;
  if (m_metric_groups[m_enabled_group]->Disable(id) == true) {
    --m_enable_count;
    if (0 == m_enable_count)
      m_enabled_group = -1;
  } else {
    assert(false);
  }
}

void
PerfMetricSet::GetDescriptions(MetricDescriptionSet *desc) {
  for (auto i = m_metric_groups.begin(); i != m_metric_groups.end(); ++i)
    (*i)->AppendDescriptions(desc);
}

PerfMetricGroup::PerfMetricGroup(int query_id)
    : m_query_id(query_id) {

  static const GLubyte *name =
      reinterpret_cast<const GLubyte*>("glGetPerfQueryInfoINTEL");

  static void (*proc_addr)() = glXGetProcAddress(name);

  assert(proc_addr != NULL);
  if (proc_addr == NULL)
    return;

  PFNGLGETPERFQUERYINFOINTEL *p_glGetPerfQueryInfoINTEL =
      reinterpret_cast<PFNGLGETPERFQUERYINFOINTEL*>(proc_addr);

  static GLint max_name_len = 0;
  if (max_name_len == 0)
    glGetIntegerv(GL_PERFQUERY_QUERY_NAME_LENGTH_MAX_INTEL, &max_name_len);

  std::vector<GLchar> query_name(max_name_len);
  unsigned int number_instances;
  p_glGetPerfQueryInfoINTEL(m_query_id,
                            query_name.size(), query_name.data(),
                            &m_data_size, &m_number_counters,
                            &number_instances, &m_capabilities_mask);
  for (unsigned int counter_num = 1; counter_num < m_number_counters;
       ++counter_num) {
    m_metrics.push_back(new PerfMetric(m_query_id, counter_num));
  }
}

PerfMetricGroup::~PerfMetricGroup() {
  while (!m_metrics.empty()) {
    delete(m_metrics.back());
    m_metrics.pop_back();
  }
}

bool
PerfMetricGroup::Enable(int id) {
  for (auto i = m_metrics.begin(); i != m_metrics.end(); ++i) {
    if ((*i)->Enable(id))
      return true;
  }
  return false;
}

bool
PerfMetricGroup::Disable(int id) {
  for (auto i = m_metrics.begin(); i != m_metrics.end(); ++i) {
    if ((*i)->Disable(id))
      return true;
  }
  return false;
}

void
PerfMetricGroup::AppendDescriptions(MetricDescriptionSet *descriptions) {
  for (auto i = m_metrics.begin(); i != m_metrics.end(); ++i)
    (*i)->AppendDescription(descriptions);
}

PerfMetric::PerfMetric(int query_id, int counter_num)
    : m_query_id(query_id), m_counter_num(counter_num) {
  static const GLubyte *name =
      reinterpret_cast<const GLubyte*>("glGetPerfCounterInfoINTEL");
  static void (*proc_addr)() = glXGetProcAddress(name);
  assert(proc_addr != NULL);
  if (proc_addr == NULL)
    return;
  static GLint max_name_len = 0, max_desc_len = 0;
  if (max_name_len == 0)
    glGetIntegerv(GL_PERFQUERY_COUNTER_NAME_LENGTH_MAX_INTEL, &max_name_len);

  if (max_desc_len == 0)
    glGetIntegerv(GL_PERFQUERY_COUNTER_DESC_LENGTH_MAX_INTEL, &max_desc_len);

  static PFNGLGETPERFCOUNTERINFOINTEL *p_glGetPerfCounterInfoINTEL =
      reinterpret_cast<PFNGLGETPERFCOUNTERINFOINTEL*>(proc_addr);

  std::vector<GLchar> counter_name(max_name_len);
  std::vector<GLchar> counter_description(max_desc_len);

  p_glGetPerfCounterInfoINTEL(m_query_id, m_counter_num,
                              counter_name.size(), counter_name.data(),
                              counter_description.size(),
                              counter_description.data(),
                              &m_offset, &m_data_size, &m_type,
                              &m_data_type, &m_max_value);
  m_name = counter_name.data();
  m_description = counter_description.data();

  std::stringstream path;
  path << "gpu/intel/" << m_counter_num << "/" << m_name;
  m_grafips_desc = new MetricDescription(path.str(),
                                         "", m_name,
                                         Grafips::GR_METRIC_COUNT);
}

void
PerfMetric::AppendDescription(MetricDescriptionSet *desc) {
  desc->push_back(*m_grafips_desc);
}

bool
PerfMetric::Enable(int id) {
  if (id != m_grafips_desc->id())
    return false;
  return true;
}

bool
PerfMetric::Disable(int id) {
  if (id != m_grafips_desc->id())
    return false;
  return true;
}
