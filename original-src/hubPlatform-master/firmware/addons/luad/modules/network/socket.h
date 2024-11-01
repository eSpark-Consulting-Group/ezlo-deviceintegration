#pragma once

#include <memory>

#include <sys/socket.h>
#include <unistd.h>
#include <string>
#include <tiny/tiny_consolecolors.h>
#include "json_utils/json_fwd.h"

namespace HubPlatform {
namespace LuaNetwork {

class ISocket {
public:
    ISocket();
    virtual ~ISocket() = default;

    virtual void createSocket( int domain, int type, int protocol ) = 0;
    virtual int connect( const sockaddr* addr, socklen_t len ) = 0;
    virtual int connect( const std::string& address, const std::string& port, int addressFamily ) = 0;
    virtual int bind( const sockaddr* addr, socklen_t len ) = 0;
    virtual int setopt( int level, int name, int value ) = 0;
    virtual int setopt( int level, int name, const JsonObject& optionValue ) = 0;
    virtual ssize_t send( const void* buf, size_t n, int flags ) = 0;
    virtual ssize_t recv( void* buf, size_t n, int flags ) = 0;
    virtual ssize_t sendto( const void* buf, size_t n, int flags, const sockaddr* addr, socklen_t addrLen ) = 0;
    virtual ssize_t recvfrom( void* buf, size_t n, int flags, sockaddr* addr, socklen_t addrLen ) = 0;
    virtual int getsockname( sockaddr* addr, socklen_t addrLen ) = 0;
    virtual int close() = 0;
    virtual int shutdown( unsigned flags ) = 0;
    virtual int setTimeout( int socketLevel, int option, std::chrono::milliseconds timeoutMs ) = 0;

    virtual bool valid() const;
    int getSocket() const;

protected:
    int m_socket;
};

std::unique_ptr< ISocket > makeNetworkSocket();

}  // namespace LuaNetwork
}  // namespace HubPlatform
