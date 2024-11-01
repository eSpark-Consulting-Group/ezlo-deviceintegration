#include "luad/modules/network/async_socket.h"
#include "luad/modules/network/events_emitter.h"

#include "log_utils/ezlo_log.h"
#include "tiny/tiny_consolecolors.h"

#include <string>
#include <cstring>
#include <cassert>

#include <sys/socket.h>
#include <unistd.h>
#include <netinet/ip.h> /* IPv4 */
#include <arpa/inet.h>
#include <netdb.h>

namespace HubPlatform {
namespace LuaNetwork {

std::atomic< AsyncSocket::AsyncSocketHandle > AsyncSocket::m_handleCounter;

namespace {

std::shared_ptr< AsyncSocket > getAsyncSocket( const std::weak_ptr< IAsyncSocket >& weakAsyncSocket,
                                               const std::string& funcName ) {
    auto asyncSocket = weakAsyncSocket.lock();
    if( !asyncSocket ) {
        Log::debug( std::string{ "AsyncSocket: " } + funcName + ": an async socket has been closed already" );
        return nullptr;
    }

    Log::debug( "AsyncSocket: " + funcName );

    return std::dynamic_pointer_cast< AsyncSocket >( asyncSocket );
}

void incomingDataEventHandler( std::weak_ptr< IAsyncSocket > weakAsyncSocket,
                               EventType eventType,
                               IEventsLoop::EventSourceHandle eventSourceHandler ) {
    auto asyncSocket = getAsyncSocket( weakAsyncSocket, __func__ );
    if( !asyncSocket )
        return;

    asyncSocket->onIncommingData();
}

void checkConnectionEventHandler( std::weak_ptr< IAsyncSocket > weakAsyncSocket,
                                  EventType eventType,
                                  IEventsLoop::EventSourceHandle eventSourceHandler ) {
    auto asyncSocket = getAsyncSocket( weakAsyncSocket, __func__ );
    if( !asyncSocket )
        return;

    asyncSocket->onConnectionStateChanged( weakAsyncSocket );
}

int getConnectionType( const RemoteAddress& remoteAddress ) {
    auto connectionType = std::get< 2 >( remoteAddress );

    switch( connectionType ) {
    case ConnectionType::TCP:
        return SOCK_STREAM;
    case ConnectionType::UDP:
        return SOCK_DGRAM;
    default:
        assert( false );
        break;
    }

    return 0;
}

}  // namespace

IAsyncSocket::~IAsyncSocket() = default;

AsyncSocket::AsyncSocket( IEventsLoop& eventsLoop,
                          EventsEmitter& eventsEmitter,
                          const RemoteAddress& remoteAddress,
                          const AsyncSocketEventsHandler& eventsHandler )
: AsyncSocket( eventsLoop,
               eventsEmitter,
               remoteAddress,
               eventsHandler,
               makeCycleBuffer( *this ),
               makeNetworkSocket() ) {}

AsyncSocket::AsyncSocket( IEventsLoop& eventsLoop,
                          EventsEmitter& eventsEmitter,
                          const RemoteAddress& remoteAddress,
                          const AsyncSocketEventsHandler& eventsHandler,
                          std::unique_ptr< IBuffer > buffer,
                          std::unique_ptr< ISocket > socket )
: m_eventsLoop( eventsLoop )
, m_eventSourceHandle( -1 )
, m_eventsEmitter( eventsEmitter )
, m_remoteAddress( remoteAddress )
, m_eventsHandler( eventsHandler )
, m_handle( m_handleCounter++ )
, m_buffer( std::move( buffer ) )
, m_socket( std::move( socket ) ) {
    /* we don't support blocking sockets anyway */
    int socketType = getConnectionType( remoteAddress );
    m_socket->createSocket( AF_INET, socketType | SOCK_NONBLOCK, 0 );
}

AsyncSocket::~AsyncSocket() {
    close();
}

AsyncSocket::AsyncSocketHandle AsyncSocket::getHandle() const {
    return m_handle;
}

void AsyncSocket::setRemoteAddress( const Host& address, const Port& port ) {
    std::get< 0 >( m_remoteAddress ) = address;
    std::get< 1 >( m_remoteAddress ) = port;
}

void AsyncSocket::connect( int options, std::weak_ptr< IAsyncSocket > weakAsyncSocket ) {
    using EventSourceHandle = IEventsLoop::EventSourceHandle;

    std::lock_guard< std::mutex > lock( m_mtx );

    std::string host, port;
    ConnectionType connectType;

    std::tie( host, port, connectType ) = m_remoteAddress;
    /* TODO: parse a remote address (IPv4/IPv6/host name)... */

    if( connectType == ConnectionType::UDP && ( host.empty() || host == "*" ) ) {
        std::memset( &m_remote, 0, sizeof( m_remote ) );
        m_remote.sin_family = AF_UNSPEC;
        m_remote.sin_addr.s_addr = INADDR_ANY;
        m_remote.sin_port = htons( std::stoi( port ) );
    }
    else {
        std::memset( &m_remote, 0, sizeof( m_remote ) );
        m_remote.sin_family = AF_INET;
        inet_pton( AF_INET, host.c_str(), &m_remote.sin_addr );
        m_remote.sin_port = htons( std::stoi( port ) );
    }

    while( true ) {
        auto res = m_socket->connect( reinterpret_cast< sockaddr* >( &m_remote ), sizeof( m_remote ) );
        if( res < 0 ) {
            if( connectType == ConnectionType::UDP ) {
                throw AsyncSocketError{ strerror( errno ) };
            }
            else if( errno == EINPROGRESS ) {
                m_eventSourceHandle = m_eventsLoop.addEventSource(
                    m_socket->getSocket(),
                    EventType::OUT,
                    [weakAsyncSocket]( EventType eventType, EventSourceHandle eventSourceHandler ) {
                        checkConnectionEventHandler( weakAsyncSocket, eventType, eventSourceHandler );
                    } );
                return;
            }
            else if( errno == EINTR )
                continue;
        }

        /* documentation states that if we're trying to establish a local connection connect may
         * perform a connection immediately;
         * also don't forget about connections for udp sockets */
        if( res == 0 ) {
            if( connectType == ConnectionType::TCP ) {
                m_eventSourceHandle =
                    m_eventsLoop.addEventSource( m_socket->getSocket(),
                                                 EventType::IN, /* TODO: we need both 'in' and 'out' events */
                                                 [weakAsyncSocket]( EventType eventType, EventSourceHandle evSrcHndl ) {
                                                     incomingDataEventHandler( weakAsyncSocket, eventType, evSrcHndl );
                                                 } );
                m_eventsEmitter.emitConnectionEstablishedEvent( m_eventsHandler, m_handle );
            }
            return;
        }

        break;
    }
    LOGGER_ERROR( cc::error( "AsyncSocketError for network.connect() = " ) + cc::str( strerror( errno ) ) );
    throw AsyncSocketError{ "fail to establish the connection by some local reason" };
}

void AsyncSocket::connected( std::weak_ptr< IAsyncSocket > weakAsyncSocket ) {
    if( std::get< 2 >( m_remoteAddress ) == ConnectionType::UDP ) {
        std::lock_guard< std::mutex > lock( m_mtx );
        using EventSourceHandle = IEventsLoop::EventSourceHandle;
        m_eventSourceHandle =
            m_eventsLoop.addEventSource( m_socket->getSocket(),
                                         EventType::IN,
                                         [weakAsyncSocket]( EventType eventType, EventSourceHandle evSrcHndl ) {
                                             incomingDataEventHandler( weakAsyncSocket, eventType, evSrcHndl );
                                         } );
        m_eventsEmitter.emitConnectionEstablishedEvent( m_eventsHandler, m_handle );
    }
}

void AsyncSocket::bind( const Host& address, const Port& port, std::weak_ptr< IAsyncSocket > weakAsyncSocket ) {
    struct addrinfo hints;
    memset( &hints, 0, sizeof( hints ) );
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_family = AF_INET;
    hints.ai_flags = AI_PASSIVE;

    struct addrinfo *iterator = nullptr, *resolved = nullptr;
    const auto errorCode = getaddrinfo( address != "*" ? address.c_str() : nullptr, port.c_str(), &hints, &resolved );
    if( errorCode != 0 ) {
        if( resolved )
            freeaddrinfo( resolved );

        LOGGER_WARN(
            cc::warn( "Cannot get address info for socket(" ) + cc::u( address ) + cc::warn( ":" ) + cc::note( port ) +
            cc::warn( "): " ) +
            cc::note( gai_strerror( errorCode ) + cc::warn( "[" ) + cc::num10( errorCode ) + cc::warn( "]" ) ) );
        throw AsyncSocketError{ "fail to get address info" };
    }

    for( iterator = resolved; iterator; iterator = iterator->ai_next ) {
        if( m_socket->bind( iterator->ai_addr, iterator->ai_addrlen ) == 0 ) {
            break;
        }
    }

    freeaddrinfo( resolved );

    if( !iterator ) {
        throw AsyncSocketError{ "fail to bind" };
    }
}

int AsyncSocket::setopt( int level, int name, int value ) {
    return m_socket->setopt( level, name, value );
}

int AsyncSocket::setopt( int level, int name, const JsonObject& optionValue ) {
    return m_socket->setopt( level, name, optionValue );
}

int AsyncSocket::setTimeout( int socketLevel, int option, std::chrono::milliseconds timeoutMs ) {
    return m_socket->setTimeout( socketLevel, option, timeoutMs );
}

void AsyncSocket::setEventsHandler( const std::string& eventsHandler ) {
    std::lock_guard< std::mutex > lock( m_mtx );

    /* TODO: make some logic to prevent changing of the event handler,
     *       specified for this async socket by using 'network.set_handler',
     *       when global events handler changes by using 'network.subscribe' */
    m_eventsHandler = eventsHandler;
}

std::size_t AsyncSocket::send( const std::string& data ) {
    std::lock_guard< std::mutex > lock( m_mtx );

    ConnectionType connectType;
    std::tie( std::ignore, std::ignore, connectType ) = m_remoteAddress;

    int ret;
    while( true ) {
        ret = m_socket->send( data.c_str(), data.size(), MSG_NOSIGNAL );
        if( ret >= 0 ) {
            break;
        }

        if( ret < 0 && errno == EINTR )
            continue;

        if( ret < 0 && ( errno == EAGAIN || errno == EWOULDBLOCK ) )
            throw AsyncSocketError{ strerror( errno ) }; /* TODO: handle this */

        if( ret < 0 )
            throw AsyncSocketError{ strerror( errno ) };
    }

    return ret;
}

std::size_t AsyncSocket::sendto( const std::string& data, const Host& address, const Port& port ) {
    std::lock_guard< std::mutex > lock( m_mtx );

    struct addrinfo hints;
    memset( &hints, 0, sizeof( hints ) );
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_family = AF_INET;
    hints.ai_flags = AI_NUMERICHOST | AI_NUMERICSERV;

    struct addrinfo* ai;
    if( getaddrinfo( address.c_str(), port.c_str(), &hints, &ai ) ) {
        throw AsyncSocketError{ "fail to get address" };
    }

    int ret;
    while( true ) {
        ret = m_socket->sendto( data.c_str(), data.size(), 0, ai->ai_addr, ai->ai_addrlen );
        if( ret >= 0 ) {
            break;
        }

        if( ret < 0 && errno == EINTR )
            continue;

        if( ret < 0 && ( errno == EAGAIN || errno == EWOULDBLOCK ) )
            throw AsyncSocketError{ strerror( errno ) }; /* TODO: handle this */

        if( ret < 0 )
            throw AsyncSocketError{ strerror( errno ) };
    }

    freeaddrinfo( ai );
    return ret;
}

std::string AsyncSocket::receive( std::size_t amount ) {
    std::lock_guard< std::mutex > lock( m_mtx );
    return receiveImpl( amount );
}

DataAddressPort AsyncSocket::receiveFrom( std::size_t amount ) {
    std::lock_guard< std::mutex > lock( m_mtx );
    return receiveFromImpl( amount );
}

std::string AsyncSocket::receiveImpl( std::size_t amount ) {
    if( std::get< 2 >( m_remoteAddress ) == ConnectionType::UDP ) {
        std::vector< char > buffer( amount );
        auto ret = m_socket->recv( &buffer[ 0 ], buffer.size(), 0 );
        resumeDataSource();
        if( ret < 0 ) {
            return {};
        }

        return std::string( buffer.cbegin(), buffer.cbegin() + ret );
    }
    else {
        return m_buffer->fetchData( amount );
    }
}

DataAddressPort AsyncSocket::receiveFromImpl( std::size_t amount ) {
    if( std::get< 2 >( m_remoteAddress ) == ConnectionType::UDP ) {
        struct sockaddr_storage addr;
        socklen_t addr_len = sizeof( addr );

        char addressBuffer[ INET6_ADDRSTRLEN ];
        char portBuffer[ 6 ];
        std::vector< char > buffer( amount );
        auto ret =
            m_socket->recvfrom( &buffer[ 0 ], buffer.size(), 0, reinterpret_cast< sockaddr* >( &addr ), addr_len );
        resumeDataSource();
        if( ret < 0 ) {
            return {};
        }

        if( getnameinfo( (struct sockaddr*) &addr,
                         addr_len,
                         addressBuffer,
                         INET6_ADDRSTRLEN,
                         portBuffer,
                         6,
                         NI_NUMERICHOST | NI_NUMERICSERV ) ) {
            return {};
        }

        return std::make_tuple( std::string( buffer.cbegin(), buffer.cbegin() + ret ),
                                std::string( addressBuffer ),
                                std::string( portBuffer ) );
    }
    else {
        return std::make_tuple(
            m_buffer->fetchData( amount ), std::get< 0 >( m_remoteAddress ), std::get< 0 >( m_remoteAddress ) );
    }
}

AddressPort AsyncSocket::getsockname() {
    std::lock_guard< std::mutex > lock( m_mtx );
    if( std::get< 2 >( m_remoteAddress ) == ConnectionType::UDP ) {
        int err;
        struct sockaddr_storage peer;
        socklen_t peer_len = sizeof( peer );
        char addressBuffer[ INET6_ADDRSTRLEN ];
        char portBuffer[ 6 ]; /* 65535 = 5 bytes + 0 to terminate it */
        if( m_socket->getsockname( reinterpret_cast< sockaddr* >( &peer ), peer_len ) < 0 ) {
            return {};
        }

        if( getnameinfo( (struct sockaddr*) &peer,
                         peer_len,
                         addressBuffer,
                         INET6_ADDRSTRLEN,
                         portBuffer,
                         6,
                         NI_NUMERICHOST | NI_NUMERICSERV ) ) {
            return {};
        }

        return std::make_pair( std::string( addressBuffer ), std::string( portBuffer ) );
    }
    else {
        return {};
    }
}

void AsyncSocket::request( std::string callback, std::string userData, std::size_t amount ) {
    std::lock_guard< std::mutex > lock( m_mtx );
    handleReceiveRequest( { std::move( callback ), std::move( userData ), amount, false } );
}

void AsyncSocket::requestFrom( std::string callback, std::string userData, std::size_t amount ) {
    std::lock_guard< std::mutex > lock( m_mtx );
    handleReceiveRequest( { std::move( callback ), std::move( userData ), amount, true } );
}

void AsyncSocket::handleReceiveRequest( ReceiveRequest request ) {
    if( std::get< 2 >( m_remoteAddress ) == ConnectionType::UDP ) {
        static std::vector< char > buffer( 16 );
        auto ret = m_socket->recv( &buffer[ 0 ], buffer.size(), MSG_PEEK );
        if( ret >= 0 ) {
            handleReceiveResponse( request );
            return;
        }
    }
    else {
        if( m_buffer->inspectDataSize() > 0 ) {
            handleReceiveResponse( request );
            return;
        }
    }

    m_receiveRequests.emplace_back( std::move( request ) );
}

std::string AsyncSocket::inspectData( std::size_t amount ) {
    std::lock_guard< std::mutex > lock( m_mtx );

    if( std::get< 2 >( m_remoteAddress ) == ConnectionType::UDP ) {
        std::vector< char > buffer( amount );
        auto ret = m_socket->recv( &buffer[ 0 ], buffer.size(), MSG_PEEK );
        if( ret < 0 ) {
            return {};
        }

        return std::string( buffer.cbegin(), buffer.cbegin() + ret );
    }

    return m_buffer->inspectData( amount );
}

std::size_t AsyncSocket::inspectDataSize() {
    std::lock_guard< std::mutex > lock( m_mtx );

    if( std::get< 2 >( m_remoteAddress ) == ConnectionType::UDP ) {
        static std::vector< char > buffer( 1024 );
        auto ret = m_socket->recv( &buffer[ 0 ], buffer.size(), MSG_PEEK );
        if( ret < 0 ) {
            return 0;
        }

        return ret;
    }

    return m_buffer->inspectDataSize();
}

void AsyncSocket::close() {
    std::lock_guard< std::mutex > lock( m_mtx );

    shutDown();
}

void AsyncSocket::handleReceiveResponse( const ReceiveRequest& request ) {
    if( request.needSenderInfo ) {
        std::string data, address, port;
        std::tie( data, address, port ) = receiveFromImpl( request.amount );
        m_eventsEmitter.emitReceivedFromEvent( request.callback, m_handle, request.userData, data, address, port );
    }
    else {
        std::string data = receiveImpl( request.amount );
        m_eventsEmitter.emitReceivedEvent( request.callback, m_handle, request.userData, data );
    }
}

void AsyncSocket::onIncommingData() {
    std::lock_guard< std::mutex > lock( m_mtx );

    if( !m_receiveRequests.empty() ) {
        auto request = std::move( m_receiveRequests.front() );
        m_receiveRequests.pop_front();

        handleReceiveResponse( request );
        return;
    }

    /* schedule a lua interpreter to handle an event concurrently with reading data */
    /* TODO: if a remote side closes a connection two events are issued,
     *       (IN and CONNECTION_CLOSED) it won't cause any problem except
     *       that at lua side we get two events which may confuse plugins'
     *       authors; such situation is normal for UNIX, but is it appropriate
     *       for our lua network api? */
    m_eventsEmitter.emitIncomingDataEvent( m_eventsHandler, m_handle );

    if( std::get< 2 >( m_remoteAddress ) == ConnectionType::UDP ) {
        pauseDataSource();
        return;
    }

    auto ret = m_socket->recv( m_buffer->front(), m_buffer->size(), 0 );
    if( ret == 0 ) {
        /* remote side closed a connection */
        m_eventsEmitter.emitConnectionClosedEvent( m_eventsHandler, m_handle );

        shutDown();
        return;
    }
    if( ret < 0 ) {
        return;
    }

    m_buffer->commit( ret );
}

void AsyncSocket::onConnectionStateChanged( const std::weak_ptr< IAsyncSocket >& weakAsyncSocket ) {
    using EventSourceHandle = IEventsLoop::EventSourceHandle;

    std::lock_guard< std::mutex > lock( m_mtx );

    int soError;
    socklen_t soErrorLength = sizeof( soError );

    getsockopt( m_socket->getSocket(), SOL_SOCKET, SO_ERROR, &soError, &soErrorLength );

    /* connection has been established successfully */
    if( soError == 0 ) {
        m_eventsEmitter.emitConnectionEstablishedEvent( m_eventsHandler, m_handle );

        m_eventsLoop.removeEventSource( m_eventSourceHandle );

        m_eventSourceHandle =
            m_eventsLoop.addEventSource( m_socket->getSocket(),
                                         EventType::IN, /* TODO: we need both 'in' and 'out' events */
                                         [weakAsyncSocket]( EventType eventType, EventSourceHandle evSrcHndl ) {
                                             incomingDataEventHandler( weakAsyncSocket, eventType, evSrcHndl );
                                         } );

        Log::debug( std::string{ "AsyncSocket: " } + __func__ + ": connection has been established" );
    }
    else {
        m_eventsEmitter.emitFailedToConnectEvent( m_eventsHandler, m_handle );

        /* Linux marked the socket as in a HUP state, so we have no choice but to unregister it from
         * polling interface and close it
         * Note: user still has to perform a 'network.close()' request to free memory occupied by an asyncSocket */
        shutDown();

        Log::debug( std::string{ "AsyncSocket: " } + __func__ + ": fail to establish a connection" );
    }
}

void AsyncSocket::shutDown() {
    m_eventsLoop.removeEventSource( m_eventSourceHandle );
    m_socket->close();

    m_eventSourceHandle = -1;
}

void AsyncSocket::pauseDataSource() {
    m_eventsLoop.pauseEventSource( m_eventSourceHandle );
}

void AsyncSocket::resumeDataSource() {
    m_eventsLoop.resumeEventSource( m_eventSourceHandle );
}

}  // namespace LuaNetwork
}  // namespace HubPlatform
