#pragma once

#include <string>
#include <netinet/in.h>

namespace HubPlatform {
namespace Metrics {

class UdpSocket final {
public:
    UdpSocket();
    ~UdpSocket();
    bool sendTo( const void* data, size_t dataLen, const struct sockaddr* addr, socklen_t addrLen ) const;
    static sockaddr_in getServerAddr( const std::string& serverUrl, size_t port );

private:
    int m_socket{ -1 };
};

bool sendUdpReport( const void* data, size_t dataLen, const struct sockaddr_in& servAddr );

}  // namespace Metrics
}  // namespace HubPlatform