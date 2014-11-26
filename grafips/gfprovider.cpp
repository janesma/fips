#include "gfprovider.h"

using namespace Grafips;

MetricDescription::MetricDescription(const MetricDescription &o) 
    : path(o.path), help_text(o.help_text), 
       display_name(o.display_name), type(o.type)
{}

MetricDescription::MetricDescription(const std::string &_path,
                                     const std::string &_help_text,
                                     const std::string &_display_name,
                                     MetricType _type)
    : path(_path), help_text(_help_text),
      display_name(_display_name), type(_type)
{}

int 
MetricDescription::id() const
{
    // hash the path
    int hash = 0;
    for (unsigned int i = 0; i < path.length(); ++i)
    {
        const char c = path[i];
        hash = 31 * hash + c;
    }
    return hash;
}
