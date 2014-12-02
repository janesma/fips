#pragma once

#include "gfmetric.h"

namespace Grafips
{
    class MetricSourceInterface;
    class MetricSinkInterface
    {
      public:
        virtual ~MetricSinkInterface() {}
        virtual void RegisterSource(MetricSourceInterface *p) = 0;
        virtual void OnMetric(const DataSet &d) = 0;
    };
}
