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

#ifndef REMOTE_GFPUBLISHER_H_
#define REMOTE_GFPUBLISHER_H_

#include <vector>
#include <map>

#include "remote/gfmetric.h"
#include "remote/gfipublisher.h"
#include "remote/gfimetric_sink.h"
#include "os/gftraits.h"

namespace Grafips {
class MetricSourceInterface;
class SubscriberInterface;
class PublisherImpl : public PublisherInterface,
                      public MetricSinkInterface,
                      NoCopy, NoAssign, NoMove {
 public:
  void Subscribe(SubscriberInterface *s);

  PublisherImpl();
  ~PublisherImpl();
  void RegisterSource(MetricSourceInterface *p);
  void OnMetric(const DataSet &d);
  void Enable(int id);
  void Disable(int id);
 private:
  SubscriberInterface *m_subscriber;
  typedef std::map <int, MetricSourceInterface*> MetricSourceMap;
  MetricSourceMap m_sources_by_metric_id;
  std::vector<MetricSourceInterface *> m_sources;
};

}  // namespace Grafips

#endif  // REMOTE_GFPUBLISHER_H_
