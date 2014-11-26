#pragma once

#include <string>
#include <vector>
#include <mutex>

#include "gfsocket.h"
#include "gfisubscriber.h"

namespace GrafipsProto
{
    class SubscriberInvocation;
}

namespace Grafips
{

    class SubscriberStub : public SubscriberInterface
    {
      public:
        SubscriberStub(const std::string &address, int port);
        ~SubscriberStub();
        void Clear(int id);
        void OnMetric(const DataSet &d);
        void OnDescriptions(const std::vector<MetricDescription> &descriptions);
        void Flush() const;
      private:
        void WriteMessage(const GrafipsProto::SubscriberInvocation&m) const;
        mutable Socket m_socket;
        mutable std::vector<unsigned char> m_buf;
        mutable std::mutex m_protect;
    };

}
