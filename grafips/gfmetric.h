#pragma once

#include <time.h>
#include <string>
#include <vector>

namespace Grafips
{
    
    enum MetricType
    {
        GR_METRIC_COUNT = 0,
        GR_METRIC_RATE,
        GR_METRIC_PERCENT
    };

    class MetricDescription
    {
      public:
        MetricDescription(const MetricDescription &o);
        MetricDescription(const std::string &_path,
                          const std::string &_help_text,
                          const std::string &_display_name,
                          MetricType _type);
        int id() const;
        std::string path;
        std::string help_text;
        std::string display_name;
        MetricType type;
    };

    inline unsigned int 
    get_ms_time()
    {
        struct timespec t;
        clock_gettime(CLOCK_REALTIME, &t);
        unsigned int ms = t.tv_sec * 1000;
        ms += (t.tv_nsec / 1000000);
        return ms;
    }

    struct DataPoint
    {
        DataPoint(unsigned int t, int i, float d) : time_val(t), id(i), data(d) {}
        unsigned int   time_val;
        int   id;
        float data;
    };

    typedef std::vector<MetricDescription> MetricDescriptionSet;

    typedef std::vector<DataPoint> DataSet;

}
