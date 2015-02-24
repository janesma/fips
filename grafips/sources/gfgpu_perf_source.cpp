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
#include <string.h>

#include <sstream>
#include <string>
#include <vector>

#include "os/gftraits.h"
#include "remote/gfimetric_sink.h"
#include "sources/gfgpu_perf_functions.h"

using Grafips::GpuPerfSource;
using Grafips::PerfFunctions;
using Grafips::PerfMetricSet;
using Grafips::NoCopy;
using Grafips::NoAssign;
using Grafips::MetricDescriptionSet;
using Grafips::MetricDescription;
using Grafips::MetricSinkInterface;
using Grafips::DataSet;
using Grafips::DataPoint;
using Grafips::get_ms_time;
using Grafips::ScopedLock;


namespace {

class PerfMetric : public NoCopy, NoAssign {
 public:
  PerfMetric(int query_id, int counter_num, MetricSinkInterface *sink);
  ~PerfMetric() { delete m_grafips_desc; }
  void AppendDescription(MetricDescriptionSet *descriptions);
  bool Enable(int id);
  bool Disable(int id);
  void Publish(const std::vector<unsigned char> &data);
 private:
  const int m_query_id, m_counter_num;
  MetricSinkInterface *m_sink;
  GLuint m_offset, m_data_size, m_type,
    m_data_type;
  GLuint64 m_max_value;
  std::string m_name, m_description;
  MetricDescription *m_grafips_desc;
};

class PerfMetricGroup : public NoCopy, NoAssign {
 public:
  PerfMetricGroup(int query_id, MetricSinkInterface *sink);
  ~PerfMetricGroup();
  void AppendDescriptions(MetricDescriptionSet *descriptions);
  bool Enable(int id);
  bool Disable(int id);
  void SwapBuffers();

 private:
  const std::string m_query_name;
  const int m_query_id;
  unsigned int m_data_size;
  unsigned int m_number_counters;
  unsigned int m_capabilities_mask;
  std::vector<unsigned char> m_data_buf;

  std::vector<PerfMetric *> m_metrics;

  // indicates offset in m_metrics of enabled metrics
  std::vector<int> m_enabled_metric_indices;

  // represent queries that have not produced results
  std::vector<unsigned int> m_extant_query_handles;
  // represent query handles that can be reused
  std::vector<unsigned int> m_free_query_handles;
  unsigned int m_current_query_handle;
};
}  // namespace

class PerfMetricSet : public NoCopy, NoAssign {
 public:
  explicit PerfMetricSet(MetricSinkInterface *sink);
  ~PerfMetricSet();
  void GetDescriptions(MetricDescriptionSet *descriptions);
  void Enable(int id);
  void Disable(int id);
  void SwapBuffers();
 private:
  std::vector<PerfMetricGroup *> m_metric_groups;
  int m_enabled_group;
  int m_enable_count;
};


GpuPerfSource::GpuPerfSource()
    : m_sink(NULL), m_metrics(NULL) {
}

GpuPerfSource::~GpuPerfSource() {
  if (m_metrics)
    delete m_metrics;
}

void
GpuPerfSource::Subscribe(MetricSinkInterface *sink) {
  ScopedLock l(&m_protect);
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
  ScopedLock l(&m_protect);
  if (m_metrics)
    m_metrics->Enable(id);
}

void
GpuPerfSource::Disable(int id) {
  ScopedLock l(&m_protect);
  if (m_metrics)
    m_metrics->Disable(id);
}

void
GpuPerfSource::MakeContextCurrent() {
  ScopedLock l(&m_protect);
  PerfFunctions::Init();
  if (m_metrics != NULL)
    return;

  m_metrics = new PerfMetricSet(m_sink);

  MetricDescriptionSet descriptions;
  GetDescriptions(&descriptions);
  m_sink->OnDescriptions(descriptions);
}

void
GpuPerfSource::glSwapBuffers() {
  ScopedLock l(&m_protect);
  if (m_metrics)
    m_metrics->SwapBuffers();
}


PerfMetricSet::PerfMetricSet(MetricSinkInterface *sink)
    : m_enabled_group(-1), m_enable_count(0) {
  unsigned int query_id = 0;
  PerfFunctions::GetFirstQueryId(&query_id);

  if (query_id == 0)
    return;

  std::vector<unsigned int> query_ids;
  query_ids.push_back(query_id);
  while (true) {
    PerfFunctions::GetNextQueryId(query_id, &query_id);
    if (!query_id)
      break;
    query_ids.push_back(query_id);
  }

  for (auto i = query_ids.begin(); i != query_ids.end(); ++i) {
    m_metric_groups.push_back(new PerfMetricGroup(*i, sink));
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
  }
}

void
PerfMetricSet::SwapBuffers() {
  if (m_enabled_group != -1)
    m_metric_groups[m_enabled_group]->SwapBuffers();
}

void
PerfMetricSet::GetDescriptions(MetricDescriptionSet *desc) {
  for (auto i = m_metric_groups.begin(); i != m_metric_groups.end(); ++i)
    (*i)->AppendDescriptions(desc);
}

PerfMetricGroup::PerfMetricGroup(int query_id, MetricSinkInterface *sink)
    : m_query_id(query_id), m_current_query_handle(GL_INVALID_VALUE) {

  static GLint max_name_len = 0;
  if (max_name_len == 0)
    glGetIntegerv(GL_PERFQUERY_QUERY_NAME_LENGTH_MAX_INTEL, &max_name_len);

  std::vector<GLchar> query_name(max_name_len);
  unsigned int number_instances;
  PerfFunctions::GetQueryInfo(m_query_id,
                            query_name.size(), query_name.data(),
                            &m_data_size, &m_number_counters,
                            &number_instances, &m_capabilities_mask);
  m_data_buf.resize(m_data_size);
  for (unsigned int counter_num = 1; counter_num <= m_number_counters;
       ++counter_num) {
    m_metrics.push_back(new PerfMetric(m_query_id, counter_num, sink));
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
  int index = 0;
  for (auto i = m_metrics.begin(); i != m_metrics.end(); ++i, ++index) {
    {
      if ((*i)->Enable(id)) {
        m_enabled_metric_indices.push_back(index);
        return true;
      }
    }
  }
  return false;
}

bool
PerfMetricGroup::Disable(int id) {
  int index = 0;
  for (auto i = m_metrics.begin(); i != m_metrics.end(); ++i, ++index) {
    if ((*i)->Disable(id)) {
      // remove the index from the list of enabled indices
      for (auto j = m_enabled_metric_indices.begin();
           j != m_enabled_metric_indices.end(); ++j) {
        if (*j == index) {
          *j = m_enabled_metric_indices.back();
          m_enabled_metric_indices.pop_back();
          break;
        }
      }
      // If last metric is disabled, then flush queries and delete
      // them.  Also set m_current_query_handle to -1
      for (auto extant_query = m_extant_query_handles.rbegin();
           extant_query != m_extant_query_handles.rend(); ++extant_query) {
        GLuint bytes_written = 0;
        PerfFunctions::GetQueryData(*extant_query, GL_PERFQUERY_WAIT_INTEL,
                                  m_data_size, m_data_buf.data(),
                                  &bytes_written);
        // assert(bytes_written != 0);
        PerfFunctions::DeleteQuery(*extant_query);
      }
      m_extant_query_handles.clear();
      m_current_query_handle = GL_INVALID_VALUE;

      for (auto free_query =m_free_query_handles.begin();
           free_query != m_free_query_handles.end(); ++free_query)
        PerfFunctions::DeleteQuery(*free_query);
      m_free_query_handles.clear();

      return true;
    }
  }
  return false;
}

void
PerfMetricGroup::SwapBuffers() {
  if (m_enabled_metric_indices.empty())
    return;

  if (m_current_query_handle != GL_INVALID_VALUE) {
    PerfFunctions::EndQuery(m_current_query_handle);
    m_extant_query_handles.push_back(m_current_query_handle);
    m_current_query_handle = GL_INVALID_VALUE;
  }

#ifdef NDEBUG
#else
  // {
  //   static GLint max_name_len = 0;
  //   if (max_name_len == 0)
  //     glGetIntegerv(GL_PERFQUERY_QUERY_NAME_LENGTH_MAX_INTEL, &max_name_len);

  //   std::vector<GLchar> query_name(max_name_len);
  //   GLuint data_size, number_counters, number_instances, capabilities_mask;
  //   PerfFunctions::GetPerfQueryInfoINTEL(m_query_id,
  //                             query_name.size(), query_name.data(),
  //                             &data_size, &number_counters,
  //                             &number_instances, &capabilities_mask);
  //   assert(number_instances == m_extant_query_handles.size());
  // }
#endif

  // reverse iteration, so we can remove entries without invalidating
  // the iterator
  for (auto extant_query = m_extant_query_handles.rbegin();
       extant_query != m_extant_query_handles.rend(); ++extant_query) {
    uint bytes_written = 0;
    memset(m_data_buf.data(), 0, m_data_buf.size());
    PerfFunctions::GetQueryData(*extant_query, GL_PERFQUERY_DONOT_FLUSH_INTEL,
                              m_data_size, m_data_buf.data(),
                              &bytes_written);
    if (bytes_written == 0) {
      continue;
    }

    // TODO(majanes) pass bytes down to the metric for publication
    for (auto i = m_enabled_metric_indices.begin();
         i != m_enabled_metric_indices.end(); ++i) {
      m_metrics[*i]->Publish(m_data_buf);
    }

    m_free_query_handles.push_back(*extant_query);
    *extant_query = m_extant_query_handles.back();
    m_extant_query_handles.pop_back();
  }

  if (m_free_query_handles.empty()) {
    unsigned int query_handle;
    PerfFunctions::CreateQuery(m_query_id, &query_handle);
    assert(query_handle != GL_INVALID_VALUE);
    m_free_query_handles.push_back(query_handle);
  }

  m_current_query_handle = m_free_query_handles.back();
  m_free_query_handles.pop_back();
  PerfFunctions::BeginQuery(m_current_query_handle);
}

void
PerfMetricGroup::AppendDescriptions(MetricDescriptionSet *descriptions) {
  for (auto i = m_metrics.begin(); i != m_metrics.end(); ++i)
    (*i)->AppendDescription(descriptions);
}

PerfMetric::PerfMetric(int query_id, int counter_num, MetricSinkInterface *sink)
    : m_query_id(query_id), m_counter_num(counter_num),
      m_sink(sink) {
  static GLint max_name_len = 0, max_desc_len = 0;
  if (max_name_len == 0)
    glGetIntegerv(GL_PERFQUERY_COUNTER_NAME_LENGTH_MAX_INTEL, &max_name_len);

  if (max_desc_len == 0)
    glGetIntegerv(GL_PERFQUERY_COUNTER_DESC_LENGTH_MAX_INTEL, &max_desc_len);

  std::vector<GLchar> counter_name(max_name_len);
  std::vector<GLchar> counter_description(max_desc_len);

  PerfFunctions::GetCounterInfo(m_query_id, m_counter_num,
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
  if (strstr(m_name.c_str(), "HS") != NULL)
    return;
  if (strstr(m_name.c_str(), "DS") != NULL)
    return;
  if (strstr(m_name.c_str(), "GS") != NULL)
    return;
  if (strstr(m_name.c_str(), "CS") != NULL)
    return;
  if (strstr(m_name.c_str(), "CL") != NULL)
    return;
  if (strstr(m_name.c_str(), "SO") != NULL)
    return;
  desc->push_back(*m_grafips_desc);
  // std::cout << m_grafips_desc->path
  //           << " offset: " << m_offset
  //           << " size: " << m_data_size
  //           << " type: " << m_type
  //           << " data_type: " << m_data_type
  //           << " max: " << m_max_value
  //           << std::endl;
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

void
PerfMetric::Publish(const std::vector<unsigned char> &data) {
  DataSet d;
  float fval;
  const unsigned char *p_value = data.data() + m_offset;
  switch (m_data_type) {
    case GL_PERFQUERY_COUNTER_DATA_UINT32_INTEL:
      {
        assert(m_data_size == 4);
        const uint32_t val = *reinterpret_cast<const uint32_t *>(p_value);
        fval = static_cast<float>(val);
        break;
      }
    case GL_PERFQUERY_COUNTER_DATA_UINT64_INTEL: {
      assert(m_data_size == 8);
      const uint64_t val = *reinterpret_cast<const uint64_t *>(p_value);
      fval = static_cast<float>(val);
      break;
    }
    case GL_PERFQUERY_COUNTER_DATA_FLOAT_INTEL: {
      assert(m_data_size == 4);
      fval = *reinterpret_cast<const float *>(p_value);
      break;
    }
    case GL_PERFQUERY_COUNTER_DATA_DOUBLE_INTEL: {
      assert(m_data_size == 8);
      const double val = *reinterpret_cast<const double *>(p_value);
      fval = static_cast<float>(val);
      break;
    }
    case GL_PERFQUERY_COUNTER_DATA_BOOL32_INTEL:
      assert(m_data_size == 4);
      assert(false);
      break;
    default:
      assert(false);
  }
  d.push_back(DataPoint(get_ms_time(), m_grafips_desc->id(), fval));
  m_sink->OnMetric(d);
}
