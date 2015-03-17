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

#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "error/gflog.h"
#include "os/gftraits.h"
#include "remote/gfimetric_sink.h"
#include "sources/gfgpu_perf_functions.h"

using Grafips::DataPoint;
using Grafips::DataSet;
using Grafips::GpuPerfSource;
using Grafips::MetricDescription;
using Grafips::MetricDescriptionSet;
using Grafips::MetricSinkInterface;
using Grafips::MetricType;
using Grafips::NoAssign;
using Grafips::NoCopy;
using Grafips::PerfFunctions;
using Grafips::PerfMetricSet;
using Grafips::ScopedLock;
using Grafips::get_ms_time;

namespace {

class PerfMetric : public NoCopy, NoAssign {
 public:
  PerfMetric(int query_id, int counter_num, MetricSinkInterface *sink);
  ~PerfMetric() { delete m_grafips_desc; }
  void AppendDescription(MetricDescriptionSet *descriptions, bool enabled);
  bool Activate(int id);
  bool Deactivate(int id);
  void Publish(const std::vector<unsigned char> &data, int frame_count);
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
  void AppendDescriptions(MetricDescriptionSet *descriptions, bool enabled);
  bool Activate(int id);
  bool Deactivate(int id);
  void SwapBuffers();

 private:
  const std::string m_query_name;
  const int m_query_id;
  unsigned int m_data_size;
  unsigned int m_number_counters;
  unsigned int m_capabilities_mask;
  std::vector<unsigned char> m_data_buf;

  std::vector<PerfMetric *> m_metrics;

  // indicates offset in m_metrics of active metrics
  std::vector<int> m_active_metric_indices;

  // represent queries that have not produced results
  struct ExtantQuery {
    ExtantQuery(int _h, int _f) : handle(_h), frames(_f) {}
    unsigned int handle;
    int frames;
  };
  std::vector<ExtantQuery> m_extant_query_handles;
  // represent query handles that can be reused
  std::vector<unsigned int> m_free_query_handles;
  unsigned int m_current_query_handle;
  int m_frame_count;
  int m_last_publish_ms;
};
}  // namespace

class PerfMetricSet : public NoCopy, NoAssign {
 public:
  explicit PerfMetricSet(MetricSinkInterface *sink);
  ~PerfMetricSet();
  void GetDescriptions(MetricDescriptionSet *descriptions);
  void Activate(int id, MetricSinkInterface *sink);
  void Deactivate(int id, MetricSinkInterface *sink);
  void SwapBuffers();
 private:
  std::vector<PerfMetricGroup *> m_metric_groups;
  int m_active_group;
  int m_active_count;
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
  if (!m_metrics)
    return;
  m_metrics->GetDescriptions(descriptions);
}

void
GpuPerfSource::Activate(int id) {
  ScopedLock l(&m_protect);
  if (m_metrics)
    m_metrics->Activate(id, m_sink);
}

void
GpuPerfSource::Deactivate(int id) {
  ScopedLock l(&m_protect);
  if (m_metrics)
    m_metrics->Deactivate(id, m_sink);
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
    : m_active_group(-1), m_active_count(0) {
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
PerfMetricSet::Activate(int id, MetricSinkInterface *sink) {
  if (m_active_group != -1) {
    if (m_metric_groups[m_active_group]->Activate(id)) {
      GFLOGF("Activated metric: %d in group: %d", id, m_active_group);
      ++m_active_count;
    } else {
      GFLOGF("Could not activate metric %d, it is not in the enabled group %d",
             id, m_active_group);
    }
    return;
  }

  for (unsigned int i = 0; i < m_metric_groups.size(); ++i) {
    if (!m_metric_groups[i]->Activate(id))
      continue;

    // else
    m_active_group = i;
    m_active_count = 1;
    GFLOGF("Enabling group %d for metric: %d", m_active_group, id);

    assert(sink);
    MetricDescriptionSet desc;
    GetDescriptions(&desc);
    sink->OnDescriptions(desc);

    return;
  }
}

void
PerfMetricSet::Deactivate(int id, MetricSinkInterface *sink) {
  if (m_active_group == -1)
    return;
  if (m_metric_groups[m_active_group]->Deactivate(id) == true) {
    --m_active_count;
    if (0 == m_active_count) {
      m_active_group = -1;

      assert(sink);
      MetricDescriptionSet desc;
      GetDescriptions(&desc);
      sink->OnDescriptions(desc);
    }
  }
}

void
PerfMetricSet::SwapBuffers() {
  if (m_active_group != -1)
    m_metric_groups[m_active_group]->SwapBuffers();
}

void
PerfMetricSet::GetDescriptions(MetricDescriptionSet *desc) {
  MetricDescriptionSet all_descriptions;

  for (int i = 0; i < (int)m_metric_groups.size(); ++i) {
    bool group_enabled = true;
    if (m_active_group != -1) {
      // only metrics in the active group are enabled
      group_enabled = m_active_group == i;
    }
    m_metric_groups[i]->AppendDescriptions(&all_descriptions,
                                           group_enabled);
  }

  // filter out duplicates, preferring the duplicate which is enabled.
  // Several QueryIds may provide a single metric, but only one of
  // them may be enabled.  The metric should be enabled if it can be
  // activated by the enabled group.
  std::map<std::string, MetricDescription*> filter;
  for (auto i = all_descriptions.begin(); i != all_descriptions.end(); ++i) {
    auto duplicate = filter.find(i->path);
    if (duplicate == filter.end()) {
      // new description, save the description in the filter
      filter[i->path] = &(*i);
      continue;
    }
    if (duplicate->second->enabled) {
      // this metric is already enabled
      continue;
    }
    if (i->enabled) {
      duplicate->second->enabled = true;
    }
  }

  for (auto i = filter.begin(); i != filter.end(); ++i) {
    desc->push_back(*(i->second));
  }
}

PerfMetricGroup::PerfMetricGroup(int query_id, MetricSinkInterface *sink)
    : m_query_id(query_id), m_current_query_handle(GL_INVALID_VALUE),
      m_frame_count(0), m_last_publish_ms(0) {

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
PerfMetricGroup::Activate(int id) {
  int index = 0;
  for (auto i = m_metrics.begin(); i != m_metrics.end(); ++i, ++index) {
    {
      if ((*i)->Activate(id)) {
        m_active_metric_indices.push_back(index);
        return true;
      }
    }
  }
  return false;
}

bool
PerfMetricGroup::Deactivate(int id) {
  int index = 0;
  for (auto i = m_metrics.begin(); i != m_metrics.end(); ++i, ++index) {
    if ((*i)->Deactivate(id)) {
      // remove the index from the list of active indices
      for (auto j = m_active_metric_indices.begin();
           j != m_active_metric_indices.end(); ++j) {
        if (*j == index) {
          *j = m_active_metric_indices.back();
          m_active_metric_indices.pop_back();
          break;
        }
      }
      // If last metric is disabled, then flush queries and delete
      // them.  Also set m_current_query_handle to -1
      for (auto extant_query = m_extant_query_handles.rbegin();
           extant_query != m_extant_query_handles.rend(); ++extant_query) {
        GLuint bytes_written = 0;
        PerfFunctions::GetQueryData(extant_query->handle, GL_PERFQUERY_WAIT_INTEL,
                                  m_data_size, m_data_buf.data(),
                                  &bytes_written);
        // assert(bytes_written != 0);
        PerfFunctions::DeleteQuery(extant_query->handle);
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
  if (m_active_metric_indices.empty())
    return;

  if (!m_last_publish_ms) {
    m_last_publish_ms = get_ms_time();
    return;
  }

  const unsigned int ms = get_ms_time();
  if (ms - m_last_publish_ms < 300) {
    ++m_frame_count;
    return;
  }

  m_last_publish_ms = ms;
  
  if (m_current_query_handle != GL_INVALID_VALUE) {
    PerfFunctions::EndQuery(m_current_query_handle);
    m_extant_query_handles.push_back(ExtantQuery(m_current_query_handle,
                                                 m_frame_count));
    m_frame_count = 0;
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
    PerfFunctions::GetQueryData(extant_query->handle, GL_PERFQUERY_DONOT_FLUSH_INTEL,
                              m_data_size, m_data_buf.data(),
                              &bytes_written);
    if (bytes_written == 0) {
      continue;
    }

    // TODO(majanes) pass bytes down to the metric for publication
    for (auto i = m_active_metric_indices.begin();
         i != m_active_metric_indices.end(); ++i) {
      m_metrics[*i]->Publish(m_data_buf, extant_query->frames);
    }

    m_free_query_handles.push_back(extant_query->handle);
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
PerfMetricGroup::AppendDescriptions(MetricDescriptionSet *descriptions,
                                    bool enabled) {
  for (auto i = m_metrics.begin(); i != m_metrics.end(); ++i)
    (*i)->AppendDescription(descriptions, enabled);
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

  MetricType t = Grafips::GR_METRIC_COUNT;
  if (strcasestr("average", counter_description.data()))
    t = Grafips::GR_METRIC_AVERAGE;
  else if (strcasestr("percent", counter_description.data()))
    t = Grafips::GR_METRIC_PERCENT;

  std::stringstream path;
  path << "gpu/intel/" << m_name;
  m_grafips_desc = new MetricDescription(path.str(),
                                         m_description, m_name,
                                         t);
  GFLOGF("GPU Metric:%s Id:%d QueryId:%d CounterNum:%d", m_name.c_str(),
         m_grafips_desc->id(), m_query_id, m_counter_num);
}

void
PerfMetric::AppendDescription(MetricDescriptionSet *desc,
                              bool enabled) {
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
  desc->back().enabled = enabled;
  // std::cout << m_grafips_desc->path
  //           << " offset: " << m_offset
  //           << " size: " << m_data_size
  //           << " type: " << m_type
  //           << " data_type: " << m_data_type
  //           << " max: " << m_max_value
  //           << std::endl;
}

bool
PerfMetric::Activate(int id) {
  if (id != m_grafips_desc->id())
    return false;
  return true;
}

bool
PerfMetric::Deactivate(int id) {
  if (id != m_grafips_desc->id())
    return false;
  return true;
}

void
PerfMetric::Publish(const std::vector<unsigned char> &data,
                    int frame_count) {
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

  if (m_grafips_desc->type == Grafips::GR_METRIC_COUNT)
    // count metrics are per frame
    fval = fval / frame_count;
  
  d.push_back(DataPoint(get_ms_time(), m_grafips_desc->id(), fval));
  m_sink->OnMetric(d);
}
