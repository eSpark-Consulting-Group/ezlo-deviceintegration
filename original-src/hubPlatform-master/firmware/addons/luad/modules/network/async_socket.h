#pragma once

#include "events_loop/events_loop.h"
#include "luad/modules/network/buffer.h"
#include "luad/modules/network/socket.h"
#include "json_utils/json_fwd.h"

#include <deque>
#include <string>
#include <tuple>
#include <mutex>
#include <memory>
#include <atomic>
#include <limits>
#include <stdexcept>
#include <netinet/in.h>

namespace HubPlatform {

namespace UT {
class AsyncSocketTestSuite;
}

namespace LuaNetwork {

using namespace EventsLoop;

using Host = std::string;
using Port = std::string;
enum class ConnectionType { NONE, TCP, UDP };

using RemoteAddress = std::tuple< Host, Port, ConnectionType >;
using AddressPort = std::pair< std::string, std::string >;
using DataAddressPort = std::tuple< std::string, std::string, std::string >;

class EventsEmitter;

class AsyncSocketError : public std::runtime_error {
public:
    AsyncSocketError( const std::string& errDescription )
    : runtime_error( "AsyncSocket: " + errDescription ) {}
};

class IAsyncSocket {
public:
    virtual ~IAsyncSocket();

    virtual void setRemoteAddress( const Host& address, const Port& port ) = 0;

    virtual void connect( int options, std::weak_ptr< IAsyncSocket > weakAsyncSocket ) = 0;
    virtual void connected( std::weak_ptr< IAsyncSocket > weakAsyncSocket ) = 0;
    virtual void bind( const Host& address, const Port& port, std::weak_ptr< IAsyncSocket > weakAsyncSocket ) = 0;
    virtual int setopt( int level, int name, int value ) = 0;
    virtual int setopt( int level, int name, const JsonObject& optionValue ) = 0;
    virtual void setEventsHandler( const std::string& eventsHandler ) = 0;
    virtual std::size_t send( const std::string& data ) = 0;
    virtual std::size_t sendto( const std::string& data, const Host& address, const Port& port ) = 0;
    virtual std::string receive( std::size_t amount = IBuffer::MaxCapacity ) = 0;
    virtual DataAddressPort receiveFrom( std::size_t amount = IBuffer::MaxCapacity ) = 0;
    virtual AddressPort getsockname() = 0;
    virtual void request( std::string callback, std::string userData, std::size_t amount ) = 0;
    virtual void requestFrom( std::string callback, std::string userData, std::size_t amount ) = 0;
    virtual std::string inspectData( std::size_t amount = IBuffer::MaxCapacity ) = 0;
    virtual std::size_t inspectDataSize() = 0;
    virtual int setTimeout( int socketLevel, int option, std::chrono::milliseconds timeoutMs ) = 0;
    virtual void close() = 0;
};

class AsyncSocket final : public IAsyncSocket, public IDataSourceManager {
    friend UT::AsyncSocketTestSuite;
    struct ReceiveRequest;

public:
    using AsyncSocketEventsHandler = std::string;
    using AsyncSocketHandle = int;

    AsyncSocket( IEventsLoop& eventsLoop,
                 EventsEmitter& eventsEmitter,
                 const RemoteAddress& remoteAddress,
                 const AsyncSocketEventsHandler& eventsHandler );
    ~AsyncSocket() override;

    AsyncSocket( const AsyncSocket& rhs ) = delete;
    AsyncSocket& operator=( const AsyncSocket& rhs ) = delete;

    AsyncSocketHandle getHandle() const;

    void setRemoteAddress( const Host& address, const Port& port ) override;

    void connect( int options, std::weak_ptr< IAsyncSocket > weakAsyncSocket ) override;
    void connected( std::weak_ptr< IAsyncSocket > weakAsyncSocket ) override;
    void bind( const Host& address, const Port& port, std::weak_ptr< IAsyncSocket > weakAsyncSocket ) override;
    int setopt( int level, int name, int value ) override;
    int setopt( int level, int name, const JsonObject& optionValue ) override;
    void setEventsHandler( const std::string& eventsHandler ) override;
    std::size_t send( const std::string& data ) override;
    std::size_t sendto( const std::string& data, const Host& address, const Port& port ) override;
    std::string receive( std::size_t amount = IBuffer::MaxCapacity ) override;
    DataAddressPort receiveFrom( std::size_t amount = IBuffer::MaxCapacity ) override;
    AddressPort getsockname() override;
    void request( std::string callback, std::string userData, std::size_t amount ) override;
    void requestFrom( std::string callback, std::string userData, std::size_t amount ) override;
    std::string inspectData( std::size_t amount = IBuffer::MaxCapacity ) override;
    std::size_t inspectDataSize() override;
    void close() override;
    int setTimeout( int socketLevel, int option, std::chrono::milliseconds timeoutMs ) override;

    void onIncommingData();
    void onConnectionStateChanged( const std::weak_ptr< IAsyncSocket >& weakAsyncSocket );

private:
    AsyncSocket( IEventsLoop& eventsLoop,
                 EventsEmitter& eventsEmitter,
                 const RemoteAddress& remoteAddress,
                 const AsyncSocketEventsHandler& eventsHandler,
                 std::unique_ptr< IBuffer > buffer,
                 std::unique_ptr< ISocket > socket );

    void shutDown();

    void pauseDataSource() override;
    void resumeDataSource() override;

    std::string receiveImpl( std::size_t amount );
    DataAddressPort receiveFromImpl( std::size_t amount );
    void handleReceiveRequest( ReceiveRequest request );
    void handleReceiveResponse( const ReceiveRequest& request );

private:
    IEventsLoop& m_eventsLoop;
    EventsEmitter& m_eventsEmitter;
    std::unique_ptr< IBuffer > m_buffer;
    std::unique_ptr< ISocket > m_socket;

    RemoteAddress m_remoteAddress;
    IEventsLoop::EventSourceHandle m_eventSourceHandle;
    AsyncSocketEventsHandler m_eventsHandler;

    struct ReceiveRequest {
        ReceiveRequest( std::string callback, std::string userData, std::size_t amount, bool needSenderInfo )
        : callback( std::move( callback ) )
        , userData( std::move( userData ) )
        , amount( amount )
        , needSenderInfo( needSenderInfo ) {}

        std::string callback;
        std::string userData;
        std::size_t amount;
        bool needSenderInfo;
    };
    std::deque< ReceiveRequest > m_receiveRequests;

    sockaddr_in m_remote;

    /* each async socket has a unique handle, we can't use fd 'cause async socket can be closed,
     * but not removed from async sockets client which may cause problems if kernel provides the
     * same fd for a new async socket;
     * this handle is propagated to an upper user (lua, js, ...) (connection handle) */
    const AsyncSocketHandle m_handle;

    mutable std::mutex m_mtx;

    /* async sockets can be created concurrently (at least we have an intention for this...) */
    static std::atomic< AsyncSocketHandle > m_handleCounter;
};

}  // namespace LuaNetwork
}  // namespace HubPlatform
