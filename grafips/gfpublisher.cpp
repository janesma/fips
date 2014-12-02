#include "gfpublisher.h"
#include "gfimetric_source.h"
#include "gfisubscriber.h"

using namespace Grafips;

PublisherImpl::PublisherImpl() : m_subscriber(NULL)
{}

PublisherImpl::~PublisherImpl() 
{
}

void 
PublisherImpl::RegisterSource(MetricSourceInterface *p)
{
    std::vector<MetricDescription> desc;
    p->GetDescriptions(&desc);
    for (unsigned int i = 0; i < desc.size(); ++i)
    {
        m_sources_by_metric_id[desc[i].id()] = p;
    }

    m_sources.push_back(p);

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
    m_sources_by_metric_id[id]->Enable(id);
}

void 
PublisherImpl::Disable(int id)
{
    m_sources_by_metric_id[id]->Disable(id);
    m_subscriber->Clear(id);
}

void 
PublisherImpl::Subscribe(SubscriberInterface *s)
{
    m_subscriber = s;
    std::vector<MetricDescription> descriptions;
    for (std::vector<MetricSourceInterface *>::const_iterator i = m_sources.begin(); i != m_sources.end(); ++i)
        (*i)->GetDescriptions(&descriptions);
    s->OnDescriptions(descriptions);
}
