#pragma once

#include <thread>
#include <set>
#include <mutex>

#include "gfprovider.h"
#include "gfpublisher.h"

namespace Grafips
{

    class CpuProvider : public Provider
    {
      public:
         void start();
         void stop();

        CpuProvider();
        ~CpuProvider();
        void GetDescriptions(std::vector<MetricDescription> *descriptions);
        void Enable(int id);
        void Disable(int id);
        void Poll();
        void Run();

        friend class CpuProviderFixture;

        PublisherInterface *publisher() { return m_publisher; }
        void setPublisher(PublisherInterface *p);

      private:
        struct CpuLine
        {
            CpuLine() : user(0), nice(0), system(0), idle(0),
                        iowait(0), irq(0), softirq(0), steal(0),
                        guest(0), guest_nice(0), utilization(0) {}

            int user;
            int nice;
            int system;
            int idle;
            int iowait;
            int irq;
            int softirq;
            int steal;
            int guest;
            int guest_nice;
            float utilization;
        };

        bool IsEnabled() const;
        void Refresh();
        void ParseCpuLine(CpuLine *dest, char **savePtr);
        void Publish();

        // file handle for /proc/stat
        int m_cpu_info_handle;

        // data structures to store the parsed line
        CpuLine m_systemStats;
        std::vector<CpuLine> m_core_stats;

        // stable buffer for reading
        std::vector<char> m_buf;

        // receives updates
        PublisherInterface *m_publisher;

        // tracks subscriptions
        std::set<int> m_enabled_cores;

        // translates metric ids to offsets
        int m_sysId;
        std::vector<int> m_ids;

        std::thread *m_thread;
        bool m_running;
        std::mutex m_protect;
    };
}
