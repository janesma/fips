#include "gfpublisher_skel.h"

#include <google/protobuf/io/zero_copy_stream_impl_lite.h>
#include <google/protobuf/io/coded_stream.h>

#include "gfsocket.h"
#include "gfpublisher.pb.h"
#include "gfipublisher.h"
#include "gfsubscriber_stub.h"

using namespace Grafips;

PublisherSkeleton::PublisherSkeleton(int port, PublisherInterface *target)
    : Thread("PublisherSkeleton"), m_server(new ServerSocket(port)), 
      m_socket(NULL), m_target(target), m_subscriber(NULL)
{
}

PublisherSkeleton::~PublisherSkeleton()
{
    if (m_socket)
        delete m_socket;
    if (m_server)
        delete m_server;
}

void
PublisherSkeleton::Run()
{
    m_socket = m_server->Accept();
    delete m_server;
    m_server = NULL;

    std::vector<unsigned char> buf;
    bool running = true;
    while (running)
    {
        uint32_t msg_len;
        if (! m_socket->Read(&msg_len))
            break;

        // std::cout << "read len: " << msg_len << std::endl;
        
        buf.resize(msg_len);
        if (! m_socket->ReadVec(&buf))
            break;
        
        // for (int i = 0; i < msg_len; ++i)
            // std::cout << " " << (int) buf[i] << " ";
        // std::cout << std::endl;

        const size_t buf_size = buf.size();
        google::protobuf::io::ArrayInputStream array_in(buf.data(), buf_size);
        google::protobuf::io::CodedInputStream coded_in(&array_in);
        
        GrafipsProto::PublisherInvocation m;
        google::protobuf::io::CodedInputStream::Limit msg_limit = coded_in.PushLimit(buf_size);
        m.ParseFromCodedStream(&coded_in);
        coded_in.PopLimit(msg_limit);

        switch (m.method())
        {
            case GrafipsProto::PublisherInvocation::kFlush:
                {
                    m_socket->Write((uint32_t)0);
                    break;
                }
            case GrafipsProto::PublisherInvocation::kEnable:
                {
                    const GrafipsProto::PublisherInvocation_Enable& args= m.enableargs();
                    m_target->Enable(args.id());
                    break;
                }
            case GrafipsProto::PublisherInvocation::kDisable:
                {
                    const GrafipsProto::PublisherInvocation_Disable& args= m.disableargs();
                    m_target->Disable(args.id());
                    break;
                }
            case GrafipsProto::PublisherInvocation::kGetDescriptions:
                {
                    std::vector<MetricDescription> d;
                    m_target->GetDescriptions(&d);

                    GrafipsProto::PublisherInvocation response;
                    response.set_method(GrafipsProto::PublisherInvocation::kGetDescriptions);
                    GrafipsProto::PublisherInvocation::GetDescriptions * args = response.mutable_getdescriptionsargs();
                    for (std::vector<MetricDescription>::const_iterator i = d.begin();
                         i != d.end(); ++i)
                    {
                        ::GrafipsProto::MetricDescription *pdesc =  args->add_descriptions();
                        pdesc->set_path(i->path);
                        pdesc->set_help_text(i->help_text);
                        pdesc->set_display_name(i->display_name);
                        pdesc->set_type((::GrafipsProto::MetricType)i->type);
                    }
                    WriteMessage(response);
                    break;
                }
            case GrafipsProto::PublisherInvocation::kSubscribe:
                {
                    assert (m_subscriber == NULL);
                    const GrafipsProto::PublisherInvocation_Subscribe& args = m.subscribeargs();
                    m_subscriber = new SubscriberStub(args.address(), args.port());
                    m_target->Subscribe(m_subscriber);
                    break;
                }
            default:
                {
                    assert(false);
                    running = false;
                    break;
                }
        }
    }

    // clean up subscriber stub
    if (m_subscriber)
    {
        delete m_subscriber;
        m_subscriber = NULL;
    }
}

void
PublisherSkeleton::WriteMessage(const GrafipsProto::PublisherInvocation &m)
{
    const uint32_t write_size = m.ByteSize();
    m_socket->Write(write_size);
    
    std::vector<unsigned char> buf;
    buf.resize(write_size);
    google::protobuf::io::ArrayOutputStream array_out(buf.data(), write_size);
    google::protobuf::io::CodedOutputStream coded_out(&array_out);
    m.SerializeToCodedStream(&coded_out);
    m_socket->Write(buf.data(), write_size);
}

void
PublisherSkeleton::Flush() const
{
    if (m_subscriber)
        m_subscriber->Flush();
}

int
PublisherSkeleton::GetPort() const
{
    return m_server->GetPort();
}
