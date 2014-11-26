#include "gfcpu_provider.h"

#include <assert.h>
#include <fcntl.h>
#include <string.h>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>
#include <thread>

#include "gfpublisher.h"

using namespace Grafips;

static const int READ_BUF_SIZE = 4096;

CpuProvider::CpuProvider() : m_publisher(NULL), m_running(false)
{
    m_cpu_info_handle = open("/proc/stat", O_RDONLY);
    m_buf.resize(READ_BUF_SIZE);
    Refresh();
}

void 
CpuProvider::setPublisher(PublisherInterface *p) 
{
    m_publisher = p; 
    m_publisher->RegisterProvider(this);
}

void 
CpuProvider::start()
{
    m_running = true;
    m_thread = new std::thread(&CpuProvider::Run, this);
}

void 
CpuProvider::stop()
{
    m_running = false;
    m_thread->join();
    delete m_thread;
    m_thread = NULL;
}

CpuProvider::~CpuProvider()
{
    close(m_cpu_info_handle);
}

void
CpuProvider::Refresh()
{
    // seek will refresh the data in the proc file
    lseek(m_cpu_info_handle, 0, SEEK_SET);

    const ssize_t bytes = read(m_cpu_info_handle, m_buf.data(), m_buf.size());
    assert(bytes > 0);
    m_buf[bytes] = '\0';

    char *linesaveptr, *wordsaveptr;
    char *line = strtok_r(m_buf.data(), "\n", &linesaveptr);
    assert(line != NULL);
    while (true)
    {
        // parse line
        char *word = strtok_r(line, " ", &wordsaveptr);
        assert(word != NULL);
        if (0 != strncmp("cpu", word, 3))
            // cpu lines expected at front of /proc/stat
            break;

        if (0 == strncmp("cpu", word, 5))
        {
            ParseCpuLine(&m_systemStats, &wordsaveptr);
        }
        else
        {
            unsigned int core = atoi(&(word[3]));
            if (core >= m_core_stats.size())
                m_core_stats.resize(core + 1);
            ParseCpuLine(&(m_core_stats[core]), &wordsaveptr);
        }

        // advance to next line
        line = strtok_r(NULL, "\n", &linesaveptr);
    }
}


void 
CpuProvider::ParseCpuLine(CpuLine *dest, char **savePtr)
{
    static const int COUNT_STAT_ITEMS = 10;
    CpuLine current;
    int *current_item = &(current.user);

    for (int i = 0; i < COUNT_STAT_ITEMS; ++i,++current_item)
    {
        const char *num = strtok_r(NULL, " ", savePtr);
        assert(num != NULL);
        *current_item = atoi(num);
    }
    
    CpuLine delta;
    int *prev_item = &(dest->user);
    int *delta_item = &(delta.user);
    current_item = &(current.user);

    for (int i = 0; i < COUNT_STAT_ITEMS; ++i,++current_item, ++prev_item, ++delta_item)
    {
        *delta_item = *current_item - *prev_item;
    }

    const float active = delta.user + delta.nice + delta.system + delta.irq + delta.softirq + delta.guest + delta.guest_nice;
    const float total = active + delta.idle + delta.iowait + delta.steal;
    if (total == 0)
        current.utilization = 0;
    else
    {
        current.utilization = active / total;
        assert (current.utilization < 100);
        assert (current.utilization >= 0);
    }
    memcpy(dest, &current, sizeof(CpuLine));
}

bool
CpuProvider::IsEnabled() const
{
    return ! m_enabled_cores.empty();
}

void 
CpuProvider::GetDescriptions(std::vector<MetricDescription> *descriptions)
{
    std::lock_guard<std::mutex> l(m_protect);
    descriptions->push_back(MetricDescription("/cpu/system/utilization", 
                                              "Displays percent cpu activity for the system",
                                              "CPU Busy", GR_METRIC_PERCENT));
    m_sysId = descriptions->back().id();

    for (unsigned int i = 0; i < m_core_stats.size(); ++i)
    {
        std::stringstream s;
        s << "/cpu/core" << i << "/utilization";
        descriptions->push_back(MetricDescription(s.str(), 
                                                  "Displays percent cpu activity for the core",
                                                  "CPU Core Busy", GR_METRIC_PERCENT));
        if (m_ids.size() <= i)
            m_ids.push_back(descriptions->back().id());
    }
}

void 
CpuProvider::Enable(int id)
{
    std::lock_guard<std::mutex> l(m_protect);
    if (id == m_sysId)
    {
        m_enabled_cores.insert(-1);
        return;
    }
    for (unsigned int i = 0; i < m_ids.size(); ++i)
    {
        if (m_ids[i] == id)
        {
            m_enabled_cores.insert(i);
            return;
        }
    }
    assert(false);
}

void 
CpuProvider::Disable(int id)
{
    std::lock_guard<std::mutex> l(m_protect);
    if (id == m_sysId)
    {
        m_enabled_cores.erase(-1);
        return;
    }
    for (unsigned int i = 0; i < m_ids.size(); ++i)
    {
        if (m_ids[i] == id)
        {
            m_enabled_cores.erase(i);
            return;
        }
    }
    assert(false);
}

void 
CpuProvider::Poll()
{
    std::lock_guard<std::mutex> l(m_protect);
    if (! IsEnabled())
        return;

    Refresh();
    Publish();
}

void 
CpuProvider::Publish()
{
    if (!m_publisher)
        return;

    DataSet d;
    const unsigned int ms = get_ms_time();

    if (m_enabled_cores.count(-1) != 0)
        d.push_back(DataPoint(ms, m_sysId, m_systemStats.utilization));

    for (unsigned int i = 0; i < m_ids.size(); ++i)
    {
        if (m_enabled_cores.count(i) == 0)
            continue;
        d.push_back(DataPoint(ms, m_ids[i], m_core_stats[i].utilization));
    }
    m_publisher->OnMetric(d);
}


void CpuProvider::Run()
{
    while (m_running)
    {
        Poll();
        usleep(1000000);
    }
}
