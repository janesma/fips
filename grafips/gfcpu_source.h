#pragma once

#include <thread>
#include <set>
#include <mutex>

#include "gfimetric_source.h"
#include "gfpublisher.h"

namespace Grafips
{

    class CpuSource : public MetricSourceInterface
    {
      public:
         void start();
         void stop();

        CpuSource();
        ~CpuSource();
        void GetDescriptions(std::vector<MetricDescription> *descriptions);
        void Enable(int id);
        void Disable(int id);
        void Poll();
        void Run();

        friend class CpuSourceFixture;

        MetricSinkInterface *MetricSink() { return m_metric_sink; }
        void SetMetricSink(MetricSinkInterface *p);

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
        void Publish(unsigned int ms);

        // file handle for /proc/stat
        int m_cpu_info_handle;

        // data structures to store the parsed line
        CpuLine m_systemStats;
        std::vector<CpuLine> m_core_stats;

        // stable buffer for reading
        std::vector<char> m_buf;

        // receives updates
        MetricSinkInterface *m_metric_sink;

        // tracks subscriptions
        std::set<int> m_enabled_cores;

        // translates metric ids to offsets
        int m_sysId;
        std::vector<int> m_ids;

        // rate limits publication.  cpu metrics in sysfs are not accurate when
        // polled faster than 500ms interval
        unsigned int m_last_publish_ms;
        
        std::thread *m_thread;
        bool m_running;
        std::mutex m_protect;
    };
}
