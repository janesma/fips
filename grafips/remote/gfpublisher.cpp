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

#include "remote/gfpublisher.h"

#include <vector>

#include "sources/gfimetric_source.h"
#include "remote/gfisubscriber.h"

using Grafips::PublisherImpl;

PublisherImpl::PublisherImpl() : m_subscriber(NULL) {}

PublisherImpl::~PublisherImpl() {}

void
PublisherImpl::RegisterSource(MetricSourceInterface *p) {
    std::vector<MetricDescription> desc;
    p->GetDescriptions(&desc);
    for (unsigned int i = 0; i < desc.size(); ++i) {
        m_sources_by_metric_id[desc[i].id()] = p;
    }

    m_sources.push_back(p);

    if (m_subscriber)
        // refresh metrics to the subscriber
        Subscribe(m_subscriber);
}

void
PublisherImpl::OnMetric(const DataSet &d) {
    if (m_subscriber)
        m_subscriber->OnMetric(d);
}

void
PublisherImpl::Enable(int id) {
    m_sources_by_metric_id[id]->Enable(id);
}

void
PublisherImpl::Disable(int id) {
    m_sources_by_metric_id[id]->Disable(id);
    m_subscriber->Clear(id);
}

void
PublisherImpl::Subscribe(SubscriberInterface *s) {
    m_subscriber = s;
    std::vector<MetricDescription> descriptions;
    for (std::vector<MetricSourceInterface *>::iterator i = m_sources.begin();
         i != m_sources.end(); ++i)
        (*i)->GetDescriptions(&descriptions);
    s->OnDescriptions(descriptions);
}
