#pragma once

#include "luad/modules/network/async_sockets_client.h"

#include <string>
#include <map>

namespace HubPlatform {
namespace LuaNetwork {

/* @brief represents a storage for all clients (plugins) */
class AsyncSocketsManager final {
public:
    using ClientName = std::string;

    AsyncSocketsClient& makeAsyncSocketsClient( const ClientName& clientName );
    void removeAsyncSocketsClient( const ClientName& clientName );

private:
    std::map< ClientName, AsyncSocketsClient > m_asyncSocketsClients;
};

}  // namespace LuaNetwork
}  // namespace HubPlatform
