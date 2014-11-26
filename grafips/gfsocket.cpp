#include "gfsocket.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <netinet/tcp.h>

using namespace Grafips;

bool
Socket::Read(void * buf, int size)
{
    int bytes_remaining = size;
    void *curPtr = buf;
    while (bytes_remaining > 0)
    {
        int bytes_read = ::read(m_socket_fd, curPtr, bytes_remaining);
        if (bytes_read <= 0)
            return false;
        bytes_remaining -= bytes_read;
        curPtr = ((char*)curPtr) + bytes_read;
    }
    return true;
}

bool
Socket::Write(const void * buf, int size)
{
    int bytes_remaining = size;
    const void *curPtr = buf;
    while (bytes_remaining > 0)
    {
        ssize_t bytes_written = ::send(m_socket_fd, curPtr,
                                      bytes_remaining,
                                      0);  // default flags
        
        if (bytes_written < 0)
        {
            if (errno == EINTR)
                continue;
            return false;
        }
        bytes_remaining -= bytes_written;
        curPtr = ((char*)curPtr) + bytes_written;
        // ASKME: Should we sleep here if bytes_remaining > 0?
    }
    return true;
}

class FreeAddrInfo
{
  public:
    FreeAddrInfo(addrinfo *p) : m_p(p){}
    ~FreeAddrInfo() { freeaddrinfo(m_p); }
  private:
    addrinfo *m_p;
};

Socket::Socket(const std::string &address, int port)
{
    struct addrinfo hints;
    memset(&hints, 0, sizeof (hints));
	hints.ai_family = AF_INET;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_socktype = SOCK_STREAM;
    struct addrinfo * resolved_address = NULL;

    int result = 0;
    result = getaddrinfo(address.c_str(), NULL, &hints, &resolved_address);

    assert(result == 0);

    FreeAddrInfo a(resolved_address);

    int fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    assert(fd);

    const int nodelay_flag = 1;
    setsockopt( fd, IPPROTO_TCP, TCP_NODELAY, &nodelay_flag, sizeof(nodelay_flag) );

    struct sockaddr_in *ip_address = (sockaddr_in *)resolved_address->ai_addr;
    ip_address->sin_port = htons(port);
    
    result = ::connect(fd, resolved_address->ai_addr, (int)resolved_address->ai_addrlen);

    assert(0 == result);

    // TODO need to raise here if couldn't connect

    m_socket_fd = fd;
}

Socket::~Socket()
{
    close(m_socket_fd);
}

ServerSocket::ServerSocket(int port)
{
    m_server_fd = socket(PF_INET, SOCK_STREAM, 0);
    assert (m_server_fd != -1);

    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    const int flag = 1;
    setsockopt( m_server_fd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag) );

    int bind_result;
    for (int retry = 0; retry < 5; ++retry)
    {
        bind_result = bind(m_server_fd, (struct sockaddr *) &address,
                                     sizeof(struct sockaddr_in));
        if (bind_result != -1)
            break;

        // delay, retry
       usleep(100000);
       perror("bind failure: ");
    }
    assert (bind_result != -1);
    // todo raise on error

    const int backlog = 1; // single connection
    const int listen_result = listen(m_server_fd, backlog);
    assert (listen_result != -1);
    // todo raise on error
}

Socket *
ServerSocket::Accept()
{
    socklen_t client_addr_size = sizeof(struct sockaddr_in);
    struct sockaddr_in client_address;
    int socket_fd = accept(m_server_fd, (struct sockaddr *) &client_address,
                         &client_addr_size);

    assert(socket_fd != -1);
    // todo raise on error

    // now that we have a connected server socket, we don't need to listen for
    // subsequent connections.
    return new Socket(socket_fd);
}

ServerSocket::~ServerSocket()
{
    close(m_server_fd);
}

int
ServerSocket::GetPort() const
{
    struct sockaddr_in addr;
    socklen_t l = sizeof(addr);
    const int result = getsockname(m_server_fd, (sockaddr*)&addr, &l);
    assert(result == 0);
    return ntohs(addr.sin_port);
}
