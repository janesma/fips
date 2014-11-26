#include "gfsubscriber_stub.h"

#include <google/protobuf/io/zero_copy_stream_impl_lite.h>
#include <google/protobuf/io/coded_stream.h>

#include "gfsubscriber.pb.h"

using namespace Grafips;

SubscriberStub::SubscriberStub(const std::string &address,
                               int port)
    : m_socket(address, port)
{
}

SubscriberStub::~SubscriberStub()
{
}

void
SubscriberStub::Flush() const
{
    GrafipsProto::SubscriberInvocation m;
    m.set_method(GrafipsProto::SubscriberInvocation::kFlush);
    WriteMessage(m);

    uint32_t response;
    m_socket.Read(&response);
    assert(response == 0);
    m_protect.unlock();
}

void
SubscriberStub::Clear(int id)
{
    GrafipsProto::SubscriberInvocation m;
    m.set_method(GrafipsProto::SubscriberInvocation::kClear);
    GrafipsProto::SubscriberInvocation::ClearM * args = m.mutable_clearargs();
    args->set_id(id);

    WriteMessage(m);
    // asynchronous, no response
}

void
SubscriberStub::WriteMessage(const GrafipsProto::SubscriberInvocation &m) const
{
    const uint32_t write_size = m.ByteSize();
    m_protect.lock();
    m_socket.Write(write_size);
    
    m_buf.resize(write_size);
    google::protobuf::io::ArrayOutputStream array_out(m_buf.data(), write_size);
    google::protobuf::io::CodedOutputStream coded_out(&array_out);
    m.SerializeToCodedStream(&coded_out);
    m_socket.Write(m_buf.data(), write_size);
    m_protect.unlock();
}

// TODO
void
SubscriberStub::OnMetric(const DataSet &d)
{
    GrafipsProto::SubscriberInvocation m;
    m.set_method(GrafipsProto::SubscriberInvocation::kOnMetric);
    GrafipsProto::SubscriberInvocation::OnMetric * args = m.mutable_onmetricargs();
    for (DataSet::const_iterator i = d.begin(); i != d.end(); ++i)
    {
        ::GrafipsProto::DataPoint* data = args->add_data();
        data->set_time_val(i->time_val);
        data->set_id(i->id);
        data->set_data(i->data);
    }

    WriteMessage(m);
    // asynchronous, no response
}

void
SubscriberStub::OnDescriptions(const std::vector<MetricDescription> &descriptions)
{
    GrafipsProto::SubscriberInvocation m;
    m.set_method(GrafipsProto::SubscriberInvocation::kOnDescriptions);
    GrafipsProto::SubscriberInvocation::OnDescriptions * args = m.mutable_ondescriptionsargs();
    for (std::vector<MetricDescription>::const_iterator i = descriptions.begin();
         i != descriptions.end(); ++i)
    {
        ::GrafipsProto::MetricDescription *pdesc =  args->add_descriptions();
        pdesc->set_path(i->path);
        pdesc->set_help_text(i->help_text);
        pdesc->set_display_name(i->display_name);
        pdesc->set_type((::GrafipsProto::MetricType)i->type);
    }
    WriteMessage(m);
    // asynchronous, no response
}
