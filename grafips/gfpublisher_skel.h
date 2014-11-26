#pragma once

#include "gfthread.h"

namespace GrafipsProto
{
    class PublisherInvocation;
}

namespace Grafips
{
    class ServerSocket;
    class Socket;
    class PublisherInterface;
    class SubscriberStub;
    
    class PublisherSkeleton : public Thread
    {
      public:
        PublisherSkeleton(int port, PublisherInterface *target);
        ~PublisherSkeleton();
        void Run();
        void Flush() const;
        int GetPort() const;
      private:
        void WriteMessage(const GrafipsProto::PublisherInvocation &m);
        ServerSocket *m_server;
        Socket *m_socket;
        PublisherInterface *m_target;

        // on Subscribe(), this member is created to send publications remotely
        SubscriberStub *m_subscriber;

     };
}
