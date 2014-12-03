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

#ifndef REMOTE_GFMETRIC_H_
#define REMOTE_GFMETRIC_H_

#include <time.h>
#include <string>
#include <vector>

namespace Grafips {
enum MetricType {
  GR_METRIC_COUNT = 0,
  GR_METRIC_RATE,
  GR_METRIC_PERCENT
};

class MetricDescription {
 public:
  MetricDescription(const MetricDescription &o);
  MetricDescription(const std::string &_path,
                    const std::string &_help_text,
                    const std::string &_display_name,
                    MetricType _type);
  int id() const;
  std::string path;
  std::string help_text;
  std::string display_name;
  MetricType type;
};

inline unsigned int
get_ms_time() {
  struct timespec t;
  clock_gettime(CLOCK_REALTIME, &t);
  unsigned int ms = t.tv_sec * 1000;
  ms += (t.tv_nsec / 1000000);
  return ms;
}

struct DataPoint {
  DataPoint(unsigned int t, int i, float d) : time_val(t), id(i), data(d) {}
  unsigned int   time_val;
  int   id;
  float data;
};

typedef std::vector<MetricDescription> MetricDescriptionSet;

typedef std::vector<DataPoint> DataSet;

}  // namespace Grafips
#endif  // REMOTE_GFMETRIC_H_
