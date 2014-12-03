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

#include "remote/gfsubscriber_stub.h"

#include <google/protobuf/io/zero_copy_stream_impl_lite.h>
#include <google/protobuf/io/coded_stream.h>

#include <string>
#include <vector>

#include "./gfsubscriber.pb.h"

using Grafips::SubscriberStub;

SubscriberStub::SubscriberStub(const std::string &address,
                               int port)
    : m_socket(address, port) {
}

SubscriberStub::~SubscriberStub() {
}

void
SubscriberStub::Flush() const {
    GrafipsProto::SubscriberInvocation m;
    m.set_method(GrafipsProto::SubscriberInvocation::kFlush);
    WriteMessage(m);

    uint32_t response;
    m_socket.Read(&response);
    assert(response == 0);
}

void
SubscriberStub::Clear(int id) {
    GrafipsProto::SubscriberInvocation m;
    m.set_method(GrafipsProto::SubscriberInvocation::kClear);
    GrafipsProto::SubscriberInvocation::ClearM * args = m.mutable_clearargs();
    args->set_id(id);

    WriteMessage(m);
    // asynchronous, no response
}

typedef GrafipsProto::SubscriberInvocation GSubInv;
void
SubscriberStub::WriteMessage(const GSubInv &m) const {
    const uint32_t write_size = m.ByteSize();
    m_protect.Lock();
    m_socket.Write(write_size);

    m_buf.resize(write_size);
    google::protobuf::io::ArrayOutputStream array_out(m_buf.data(), write_size);
    google::protobuf::io::CodedOutputStream coded_out(&array_out);
    m.SerializeToCodedStream(&coded_out);
    m_socket.Write(m_buf.data(), write_size);
    m_protect.Unlock();
}

typedef GrafipsProto::SubscriberInvocation::OnMetric GOnMet;
void
SubscriberStub::OnMetric(const DataSet &d) {
    GrafipsProto::SubscriberInvocation m;
    m.set_method(GrafipsProto::SubscriberInvocation::kOnMetric);
    GOnMet * args = m.mutable_onmetricargs();
    for (DataSet::const_iterator i = d.begin(); i != d.end(); ++i) {
        ::GrafipsProto::DataPoint* data = args->add_data();
        data->set_time_val(i->time_val);
        data->set_id(i->id);
        data->set_data(i->data);
    }

    WriteMessage(m);
    // asynchronous, no response
}

typedef GrafipsProto::SubscriberInvocation::OnDescriptions GOnDesc;
void
SubscriberStub::OnDescriptions(const std::vector<MetricDescription> &desc) {
    GrafipsProto::SubscriberInvocation m;
    m.set_method(GrafipsProto::SubscriberInvocation::kOnDescriptions);
    GOnDesc * args = m.mutable_ondescriptionsargs();
    for (std::vector<MetricDescription>::const_iterator i = desc.begin();
         i != desc.end(); ++i) {
        ::GrafipsProto::MetricDescription *pdesc =  args->add_descriptions();
        pdesc->set_path(i->path);
        pdesc->set_help_text(i->help_text);
        pdesc->set_display_name(i->display_name);
        pdesc->set_type((::GrafipsProto::MetricType)i->type);
    }
    WriteMessage(m);
    // asynchronous, no response
}
