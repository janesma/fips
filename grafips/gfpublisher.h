#pragma once

#include <vector>
#include <map>

#include "gfmetric.h"
#include "gfipublisher.h"
#include "gfimetric_sink.h"
#include "gftraits.h"

namespace Grafips
{
    class MetricSourceInterface;
    class SubscriberInterface;
    class PublisherImpl : public PublisherInterface,
                          public MetricSinkInterface,
                          NoCopy, NoAssign, NoMove
    {
      public:
        void Subscribe(SubscriberInterface *);

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

// handles off-machine publication
    class PublisherProxy
    {
    };
}
