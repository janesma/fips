#include "gfpublisher.h"
#include "gfprovider.h"
#include "gfisubscriber.h"

using namespace Grafips;

PublisherImpl::PublisherImpl() : m_subscriber(NULL)
{}

PublisherImpl::~PublisherImpl() 
{
}

void 
PublisherImpl::RegisterProvider(Provider *p)
{
    std::vector<MetricDescription> desc;
    p->GetDescriptions(&desc);
    for (unsigned int i = 0; i < desc.size(); ++i)
    {
        m_providersByMetricId[desc[i].id()] = p;
    }

    m_providers.push_back(p);

    if (m_subscriber)
        // refresh metrics to the subscriber
        Subscribe(m_subscriber);
}

void 
PublisherImpl::OnMetric(const DataSet &d)
{
    if (m_subscriber)
        m_subscriber->OnMetric(d);
}

void 
PublisherImpl::Enable(int id)
{
    m_providersByMetricId[id]->Enable(id);
}

void 
PublisherImpl::Disable(int id)
{
    m_providersByMetricId[id]->Disable(id);
    m_subscriber->Clear(id);
}

void 
PublisherImpl::GetDescriptions(std::vector<MetricDescription> *descriptions) const
{
    for (std::vector<Provider *>::const_iterator i = m_providers.begin(); i != m_providers.end(); ++i)
        (*i)->GetDescriptions(descriptions);
}

void 
PublisherImpl::Subscribe(SubscriberInterface *s)
{
    m_subscriber = s;
    std::vector<MetricDescription> descriptions;
    GetDescriptions(&descriptions);
    s->OnDescriptions(descriptions);
}
