#pragma once

#include <pthread.h>
#include <string>

namespace Grafips
{
    class Thread
    {
      public:
        Thread(const std::string &name);
        virtual void Run() = 0;
        void Start();
        void Join();
      private:
        const std::string m_name;
        pthread_t m_thread;
    };
}
