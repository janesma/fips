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

#ifndef CONTROLS_GFCPU_FREQ_CONTROL_H_
#define CONTROLS_GFCPU_FREQ_CONTROL_H_

#include <string>
#include <vector>

#include "controls/gficontrol.h"

// TODO(majanes) this control uses sysfs, which is apparently
// unreliable for accessing cpu frequency configuration, especially on
// new platforms.  Kristin Accardi recommended using the MSRs to get
// and modify this information.  See turbostat in the linux sources.

namespace Grafips {

// Handles I/O and parsing of data in sysfs, including stripping
// newlines, etc.
class FreqSysParser {
 public:
  FreqSysParser();
  ~FreqSysParser();
  const std::string Governor() const;
  const std::string MaxFreq() const;
  const std::string MinFreq() const;
  bool IsValid() const;
  void SetMaxFreq(const std::string &max);
  void SetMinFreq(const std::string &min);
  void SetGovernor(const std::string &policy);
 private:
  void ReadToBuffer(int fh) const;
  void Write(int fh, const std::string&value);
  int m_governor_fh;
  int m_max_fh;
  int m_min_fh;
  mutable std::vector<char> m_buf;
};

class CpuFreqControl : public ControlInterface {
 public:
  CpuFreqControl();
  ~CpuFreqControl();
  void Set(const std::string &key, const std::string &value);
  void Subscribe(ControlSubscriberInterface *sub);
  bool IsValid() const;
 private:
  void Publish();

  std::string m_current_setting;
  FreqSysParser m_parser;
  ControlSubscriberInterface *m_subscriber;

  // to restore sane state after termination
  std::string m_orig_scaling_governor;
  std::string m_orig_min_freq;
  std::string m_orig_max_freq;
};

}  // namespace Grafips

#endif  // CONTROLS_GFCPU_FREQ_CONTROL_H_
