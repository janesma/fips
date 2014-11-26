#pragma once

#include <vector>
#include <map>

#include "gfmetric.h"
#include "gfipublisher.h"

namespace Grafips
{
    class SubscriberInterface;
    class PublisherImpl : public PublisherInterface
    {
      public:
        void Subscribe(SubscriberInterface *);

        PublisherImpl();
        ~PublisherImpl();
        void RegisterProvider(Provider *p);
        void OnMetric(const DataSet &d);
         void Enable(int id);
         void Disable(int id);
        void GetDescriptions(std::vector<MetricDescription> *descriptions) const;
      private:
        SubscriberInterface *m_subscriber;
        typedef std::map <int, Provider*> ProviderMap;
        ProviderMap m_providersByMetricId;
        std::vector<Provider *> m_providers;
    };

// handles off-machine publication
    class PublisherProxy
    {
    };
}
