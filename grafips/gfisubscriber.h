#pragma once

#include "gfmetric.h"

namespace Grafips
{
    // handles publications, distributes metric data to associated GraphSet
    class SubscriberInterface
    {
      public:
        virtual ~SubscriberInterface() {}
        virtual void Clear(int id) = 0;
        virtual void OnMetric(const DataSet &d) = 0;
        virtual void OnDescriptions(const std::vector<MetricDescription> &descriptions) = 0;
    };
}
