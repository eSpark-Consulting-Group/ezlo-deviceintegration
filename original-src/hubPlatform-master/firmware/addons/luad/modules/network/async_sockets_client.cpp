#include "luad/modules/network/async_sockets_client.h"

namespace HubPlatform {
namespace LuaNetwork {

class AsyncSocketsClientError : public std::runtime_error {
public:
    AsyncSocketsClientError( const std::string& errDesription )
    : runtime_error( "AsyncSocketsClient: " + errDesription ) {}
};

AsyncSocketsClient::AsyncSocketHandle AsyncSocketsClient::makeAsyncSocket( IEventsLoop& eventsLoop,
                                                                           EventsEmitter& eventsEmitter,
                                                                           const RemoteAddress& remoteAddress ) {
    auto socket = std::make_shared< AsyncSocket >( eventsLoop, eventsEmitter, remoteAddress, m_eventsHandler );

    auto result = m_asyncSockets.emplace( std::make_pair( socket->getHandle(), std::move( socket ) ) );
    if( !result.second )
        throw AsyncSocketsClientError{ "fail to register a new AsyncSocket" };

    return result.first->first;
}

std::shared_ptr< AsyncSocket >& AsyncSocketsClient::getAsyncSocket( AsyncSocketHandle handle ) {
    auto it = m_asyncSockets.find( handle );
    if( it != m_asyncSockets.end() )
        return it->second;

    throw AsyncSocketsClientError{ "fail to get an AsyncSocketsClient, handle: " + std::to_string( handle ) };
}

void AsyncSocketsClient::removeAsyncSocket( AsyncSocketHandle handle ) {
    m_asyncSockets.erase( handle );
}

void AsyncSocketsClient::setEventsHandler( const std::string& eventsHandler ) {
    m_eventsHandler = eventsHandler;

    for( auto& socket : m_asyncSockets )
        socket.second->setEventsHandler( m_eventsHandler );
}

void AsyncSocketsClient::reset() {
    for( auto& socket : m_asyncSockets ) {
        socket.second->close();
    }
}

void AsyncSocketsClient::processOpenedSockets( SocketsProcessCall call ) {
    for( auto& socket : m_asyncSockets ) {
        call( socket.first );
    }
}

}  // namespace LuaNetwork
}  // namespace HubPlatform
