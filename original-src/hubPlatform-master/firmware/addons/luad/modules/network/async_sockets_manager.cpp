#include "luad/modules/network/async_sockets_manager.h"

#include "log_utils/ezlo_log.h"
#include "tiny/tiny_consolecolors.h"

#include <stdexcept>

namespace HubPlatform {
namespace LuaNetwork {

class AsyncSocketsManagerError : public std::runtime_error {
public:
    AsyncSocketsManagerError( const std::string& errDesription )
    : runtime_error( "AsyncSocketsManage: " + errDesription ) {}
};

AsyncSocketsClient& AsyncSocketsManager::makeAsyncSocketsClient( const ClientName& name ) {
    auto it = m_asyncSocketsClients.find( name );
    if( it != m_asyncSocketsClients.end() )
        return it->second;

    auto result = m_asyncSocketsClients.emplace( std::make_pair( name, AsyncSocketsClient{} ) );
    if( !result.second )
        throw AsyncSocketsManagerError{ "fail to make a new AsyncSocketsClient" };

    return result.first->second;
}

void AsyncSocketsManager::removeAsyncSocketsClient( const ClientName& clientName ) {
    auto it = m_asyncSocketsClients.find( clientName );
    if( it != m_asyncSocketsClients.end() ) {
        it->second.reset();
    }

    m_asyncSocketsClients.erase( clientName );
}

}  // namespace LuaNetwork
}  // namespace HubPlatform
