#pragma once

#include <vector>
#include <map>

#include "gfmetric.h"

namespace Grafips
{

    class Provider;
    class SubscriberInterface;

// collates metrics from providers, distributes to subscriber (which may be
// off-proc or off-machine)
    class PublisherInterface
    {
      public:
        virtual ~PublisherInterface() {}
        virtual void RegisterProvider(Provider *p) = 0;
        virtual void OnMetric(const DataSet &d) = 0;
        virtual void Enable(int id) = 0;
        virtual void Disable(int id) = 0;
        virtual void GetDescriptions(std::vector<MetricDescription> *descriptions) const = 0;
        virtual void Subscribe(SubscriberInterface *) = 0;
    };
}
