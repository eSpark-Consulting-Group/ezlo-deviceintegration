#pragma once

#include <sys/socket.h>
#include <unistd.h>
#include <string>
#include "luad/modules/network/network_address.h"

namespace HubPlatform {
namespace LuaNetwork {

class NetworkPing {
public:
    enum class Result { OK, SOCKET_NOT_OPENED, SOCKET_SET_OPTION_FAILED, SEND_FAILED, RECEIVE_FAILED, PING_ERROR };

    explicit NetworkPing( const IAddress& address );
    ~NetworkPing();
    Result sendPing();
    int getSequenceCounter() const;

private:
    static unsigned short checksum( void* b, int len );
    Result initPing();

private:
    int m_socket;
    const IAddress& m_address;
    int m_sequenceCounter{ 0 };
    bool m_isInitialized{ false };
};

}  // namespace LuaNetwork
}  // namespace HubPlatform
