#include "hardware_manager/button/netlink_istream_buf.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

const char* BadStreamBuf::what() const noexcept {
    return "Failed to open netlink socket";
}

NetlinkIStreamBuf::NetlinkIStreamBuf() {
    init();
}

int NetlinkIStreamBuf::underflow() {
    if( gptr() < egptr() ) {
        return traits_type::to_int_type( *gptr() );
    }
    if( gptr() == egptr() && gptr() != m_buf ) {
        setg( m_buf, m_buf, m_buf );
        return traits_type::eof();
    }
    int rc = recv( m_fd, gptr(), NetlinkIStreamBuf::size, 0 );
    if( rc <= 0 ) {
        return traits_type::eof();
    }
    setg( m_buf, gptr(), gptr() + rc );
    return traits_type::to_int_type( *gptr() );
}

void NetlinkIStreamBuf::init() {
    m_fd = socket( AF_NETLINK, SOCK_DGRAM, NETLINK_KOBJECT_UEVENT );
    if( m_fd == -1 ) {
        throw BadStreamBuf();
    }
    m_addr.nl_family = AF_NETLINK;
    m_addr.nl_pid = getpid();
    m_addr.nl_groups = -1;
    bind( m_fd, reinterpret_cast< struct sockaddr* >( &( m_addr ) ), sizeof( m_addr ) );
    setg( m_buf, m_buf, m_buf );
}

NetlinkIStreamBuf::~NetlinkIStreamBuf() {
    close( m_fd );
}