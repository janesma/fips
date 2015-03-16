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

#include "sources/gfgl_source.h"

#include <assert.h>

#include <string>

#include "remote/gfpublisher.h"
#include "remote/gfimetric_sink.h"

using Grafips::GlSource;
using Grafips::MetricDescriptionSet;
using Grafips::MetricDescription;
using Grafips::MetricType;

const int NANO_SECONDS_PER_SEC = 1000000000;
const int NANO_SECONDS_PER_MS = 1000000;
static const MetricDescriptionSet k_metrics = {
  MetricDescription("gl/fps",
                    "measures the current frame rate in frames per second",
                    "FPS",
                    Grafips::GR_METRIC_RATE),
  MetricDescription("gl/frame_time",
                    "measures the time spent rendering the "
                    "frame in milliseconds",
                    "Frame Time",
                    Grafips::GR_METRIC_COUNT)
};

static const int kfps_id = k_metrics[0].id();
static const int kframe_time_id = k_metrics[1].id();

GlSource::GlSource(int ms_interval)
    : m_sink(NULL), m_last_time_ns(0), m_frame_count(0),
      m_ms_interval(ms_interval) {
}

GlSource::~GlSource() {
}

void
GlSource::Subscribe(MetricSinkInterface *sink) {
  m_sink = sink;

  MetricDescriptionSet desc;
  GetDescriptions(&desc);
  sink->OnDescriptions(desc);
}

void
GlSource::GetDescriptions(MetricDescriptionSet *descriptions) {
  for (MetricDescriptionSet::const_iterator i = k_metrics.begin();
       i != k_metrics.end(); ++i) {
    descriptions->push_back(*i);
  }
}

void
GlSource::Activate(int id) {
  m_active_ids.insert(id);
}

void
GlSource::Deactivate(int id) {
  m_active_ids.erase(id);
}

void
GlSource::glSwapBuffers() {
  if (m_active_ids.empty())
    return;

  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
  const uint64_t current_time_ns = ts.tv_sec * NANO_SECONDS_PER_SEC
                                   + ts.tv_nsec;
  if (!m_last_time_ns) {
    m_last_time_ns = current_time_ns;
    return;
  }

  ++m_frame_count;

  assert(current_time_ns > m_last_time_ns);
  const float frame_time_ns = current_time_ns - m_last_time_ns;
  if (frame_time_ns < m_ms_interval * NANO_SECONDS_PER_MS)
    return;

  const float frame_time_ms = frame_time_ns / NANO_SECONDS_PER_MS /
                              m_frame_count;
  m_frame_count = 0;

  DataSet d;
  const unsigned int ms = get_ms_time();
  if (m_active_ids.find(kfps_id) !=  m_active_ids.end()) {
    const float fps = 1000.0 / frame_time_ms;
    d.push_back(DataPoint(ms, kfps_id, fps));
  }
  if (m_active_ids.find(kframe_time_id) !=  m_active_ids.end()) {
    d.push_back(DataPoint(ms, kframe_time_id, frame_time_ms));
  }

  m_sink->OnMetric(d);
  m_last_time_ns = current_time_ns;
}
