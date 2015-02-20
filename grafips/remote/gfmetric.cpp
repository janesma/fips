// Copyright (C) Intel Corp.  2014.  All Rights Reserved.

// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:

// The above copyright notice and this permission notice (including the
// next paragraph) shall be included in all copies or substantial
// portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

//  **********************************************************************/
//  * Authors:
//  *   Mark Janes <mark.a.janes@intel.com>
//  **********************************************************************/

#include "remote/gfmetric.h"

#include <string>

using Grafips::MetricDescription;

MetricDescription::MetricDescription(const MetricDescription &o)
    : path(o.path), help_text(o.help_text),
      display_name(o.display_name), type(o.type) {}

MetricDescription::MetricDescription(const std::string &_path,
                                     const std::string &_help_text,
                                     const std::string &_display_name,
                                     MetricType _type)
    : path(_path), help_text(_help_text),
      display_name(_display_name), type(_type) {}

int
MetricDescription::id() const {
    // hash the path
    int hash = 0;
    for (unsigned int i = 0; i < path.length(); ++i) {
        const char c = path[i];
        hash = 31 * hash + c;
    }
    return hash;
}

MetricDescription::MetricDescription() { }

MetricDescription &
MetricDescription::operator=(const MetricDescription &o) {
  path = o.path;
  help_text = o.help_text;
  display_name = o.display_name;
  type = type;
  return *this;
}
