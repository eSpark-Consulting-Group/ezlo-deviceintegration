#pragma once

#include "luad/modules/network/async_socket.h"

#include <map>
#include <string>
#include <memory>

namespace HubPlatform {

namespace EventsLoop {
class IEventsLoop;
}

namespace LuaNetwork {

class EventsEmitter;

/* @brief represents a storage for a client's (plugin's) async sockets */
class AsyncSocketsClient final {
public:
    using AsyncSocketHandle = AsyncSocket::AsyncSocketHandle;
    using SocketsProcessCall = std::function< void( int connectionHandle ) >;

    AsyncSocketHandle makeAsyncSocket( IEventsLoop& eventsLoop,
                                       EventsEmitter& eventsEmitter,
                                       const RemoteAddress& remoteAddress );
    std::shared_ptr< AsyncSocket >& getAsyncSocket( AsyncSocketHandle handle );
    void removeAsyncSocket( AsyncSocketHandle handle );

    void setEventsHandler( const std::string& eventsHandler );

    void processOpenedSockets( SocketsProcessCall call );

    void reset();

private:
    std::map< AsyncSocketHandle, std::shared_ptr< AsyncSocket > > m_asyncSockets;

    /* it's normal to have m_eventsHandler empty, cause client (plugin),
     * by default, has no subscription */
    std::string m_eventsHandler;
};

}  // namespace LuaNetwork
}  // namespace HubPlatform
