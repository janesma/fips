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

#include "controls/gfcpu_freq_control.h"

#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <assert.h>
#include <unistd.h>

#include <string>
#include <vector>

#include "error/gflog.h"

using Grafips::CpuFreqControl;
using Grafips::FreqSysParser;

CpuFreqControl::CpuFreqControl() : m_subscriber(NULL) {
  m_orig_scaling_governor = m_parser.Governor();
  // GFLOGF("CpuFreqControl m_orig_scaling_governor %s",
  // m_orig_scaling_governor.c_str());
  m_current_setting = m_orig_scaling_governor;
  m_orig_max_freq = m_parser.MaxFreq();
  // GFLOGF("CpuFreqControl m_orig_max_freq %s", m_orig_max_freq.c_str());
  m_orig_min_freq = m_parser.MinFreq();
  // GFLOGF("CpuFreqControl m_orig_min_freq %s", m_orig_min_freq.c_str());
}

CpuFreqControl::~CpuFreqControl() {
  // if filehandles were successfully opened, re-write them with their
  // original values.
  m_parser.SetGovernor(m_orig_scaling_governor);
  m_parser.SetMinFreq(m_orig_min_freq);
  m_parser.SetMaxFreq(m_orig_max_freq);
}

void
CpuFreqControl::Set(const std::string &key, const std::string &value) {
  if (key != "CpuFrequencyPolicy")
    return;
  if (!IsValid())
    return;

  // GFLOGF("CpuFreqControl::Set %s", value.c_str());

  if ((value == "performance") || (value == "powersave")) {
    m_parser.SetMinFreq(m_orig_min_freq);
    m_parser.SetMaxFreq(m_orig_max_freq);
    m_parser.SetGovernor(value);
    m_current_setting = value;
    Publish();
    return;
  }

  if (value == "max_freq") {
    // to clip frequency range to max, increase the minimum limit
    m_parser.SetMaxFreq(m_orig_max_freq);
    m_parser.SetMinFreq(m_orig_max_freq);
    m_parser.SetGovernor("performance");
    m_current_setting = value;
    Publish();
    return;
  }

  if (value == "min_freq") {
    // to clip frequency range to min, decrease the maximum limit
    m_parser.SetMinFreq(m_orig_min_freq);
    m_parser.SetMaxFreq(m_orig_min_freq);
    m_parser.SetGovernor("powersave");
    m_current_setting = value;
    Publish();
    return;
  }

  GFLOGF("CpuFreqControl::Set invalid %s", value.c_str());
  // else value is not one of the supported options
  return;
}

bool
CpuFreqControl::IsValid() const {
  return m_parser.IsValid();
}

void
CpuFreqControl::Subscribe(ControlSubscriberInterface *sub) {
  if (!m_parser.IsValid())
    return;
  m_subscriber = sub;
  sub->OnControlChanged("CpuFrequencyPolicy", m_current_setting);
}

void
CpuFreqControl::Publish() {
  if (!m_subscriber)
    return;
  m_subscriber->OnControlChanged("CpuFrequencyPolicy", m_current_setting);
}

static const int LINE_LEN = 100;

FreqSysParser::FreqSysParser()
    : m_governor_fh(-1),
      m_max_fh(-1),
      m_min_fh(-1),
      m_buf(LINE_LEN + 1) {
  m_buf[LINE_LEN] = '\0';

  static const char *gov_path = "/sys/devices/system/cpu/cpu0/cpufreq/"
                                "scaling_governor";
  m_governor_fh = open(gov_path, O_RDWR);
  if (m_governor_fh == -1) {
    assert(errno == EACCES);
    return;
  }

  static const char *max_path = "/sys/devices/system/cpu/cpu0/cpufreq/"
                                "scaling_max_freq";
  m_max_fh = open(max_path, O_RDWR);
  assert(m_max_fh);

  static const char * min_path = "/sys/devices/system/cpu/cpu0/cpufreq/"
                                 "scaling_min_freq";
  m_min_fh = open(min_path, O_RDWR);
  assert(m_min_fh);
}

FreqSysParser::~FreqSysParser() {
  if (m_governor_fh == -1)
    return;
  close(m_governor_fh);
  close(m_max_fh);
  close(m_min_fh);
}

const std::string
FreqSysParser::Governor() const {
  if (!IsValid())
    return "";
  ReadToBuffer(m_governor_fh);
  return m_buf.data();
}

const std::string
FreqSysParser::MaxFreq() const {
  if (!IsValid())
    return "";
  ReadToBuffer(m_max_fh);
  return m_buf.data();
}
const std::string
FreqSysParser::MinFreq() const {
  if (!IsValid())
    return "";
  ReadToBuffer(m_min_fh);
  return m_buf.data();
}

void
FreqSysParser::SetMaxFreq(const std::string &max) {
  if (!IsValid())
    return;
  Write(m_max_fh, max);
}

void
FreqSysParser::SetMinFreq(const std::string &min) {
  if (!IsValid())
    return;
  Write(m_min_fh, min);
}

void
FreqSysParser::SetGovernor(const std::string &policy) {
  if (!IsValid())
    return;
  Write(m_governor_fh, policy);
}

bool
FreqSysParser::IsValid() const {
  return (m_governor_fh != -1);
}

// utility function for reading the sysfs data safely, which needs to
// be done in several places
void
FreqSysParser::ReadToBuffer(int fh) const {
  lseek(fh, 0, SEEK_SET);
  ssize_t nbytes = read(fh, m_buf.data(), LINE_LEN);
  assert(nbytes > 0);
  m_buf[nbytes] = '\0';

  // contents of sysfs files are newline terminated, but we never want
  // the newline in our state.
  for (int i = 0; i < nbytes; ++i) {
    if (m_buf[i] == '\n') {
      m_buf[i] = '\0';
      break;
    }
  }
}

void
FreqSysParser::Write(int fh, const std::string&value) {
  lseek(fh, 0, SEEK_SET);

  // extra byte for null termination
  const ssize_t bytes = write(fh, value.c_str(), value.size() + 1);
  assert(bytes == static_cast<ssize_t>(value.size() + 1));
}
