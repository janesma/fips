#pragma once

#include <vector>
#include <map>

#include "gfmetric.h"

namespace Grafips
{

    class SubscriberInterface;

    // collates metrics from sources, distributes to subscriber (which may be
    // off-proc or off-machine)
    class PublisherInterface
    {
      public:
        virtual ~PublisherInterface() {}
        virtual void Enable(int id) = 0;
        virtual void Disable(int id) = 0;
        virtual void Subscribe(SubscriberInterface *) = 0;
    };
}
