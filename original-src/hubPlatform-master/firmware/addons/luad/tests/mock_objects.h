#pragma once

#include "events_loop/events_loop.h"
#include "luad/modules/network/events_emitter.h"
#include "luad/modules/network/buffer.h"
#include "luad/modules/network/socket.h"

#include <gmock/gmock.h>

namespace HubPlatform {
namespace UT {

using namespace LuaNetwork;
using namespace EventsLoop;

class MockEventsLoop final : public IEventsLoop {
public:
    MOCK_METHOD3( addEventSource,
                  EventSourceHandle( int fileDescriptor, EventType eventType, EventHandler&& eventHandler ) );
    MOCK_METHOD1( removeEventSource, void( EventSourceHandle eventSourceHandle ) );

    MOCK_METHOD1( pauseEventSource, void( EventSourceHandle eventSourceHandle ) );
    MOCK_METHOD1( resumeEventSource, void( EventSourceHandle eventSourceHandle ) );
};

class MockEventsEmitter final : public EventsEmitter {
public:
    MOCK_METHOD2( emitConnectionEstablishedEvent, void( const std::string& eventHandler, int connectionHandle ) );
    MOCK_METHOD2( emitFailedToConnectEvent, void( const std::string& eventHandler, int connectionHandle ) );
    MOCK_METHOD2( emitIncomingDataEvent, void( const std::string& eventHandler, int connectionHandle ) );
    MOCK_METHOD2( emitConnectionClosedEvent, void( const std::string& eventHandler, int connectionHandle ) );
    MOCK_METHOD4( emitScanningResult,
                  void( const std::string& eventHandler,
                        int scanId,
                        const std::string& status,
                        ScanningOptionResult&& optionResult ) );
    MOCK_METHOD3( emitDhcpEvent,
                  void( const std::string& eventHandler, const std::string& eventType, const Json& eventData ) );
    MOCK_METHOD6( emitReceivedFromEvent,
                  void( std::string eventHandler,
                        int connectionHandle,
                        std::string userData,
                        std::string data,
                        std::string address,
                        std::string port ) );
    MOCK_METHOD4( emitReceivedEvent,
                  void( std::string eventHandler, int connectionHandle, std::string userData, std::string data ) );
};

class MockBuffer final : public IBuffer {
public:
    MOCK_METHOD0( front, char*() );
    MOCK_CONST_METHOD0( size, std::size_t() );
    MOCK_METHOD1( commit, void( std::size_t providedDataAmount ) );

    MOCK_METHOD1( fetchData, std::string( std::size_t amount ) );
    MOCK_CONST_METHOD1( inspectData, std::string( std::size_t amount ) );
    MOCK_CONST_METHOD0( inspectDataSize, std::size_t() );
};

class MockSocket final : public ISocket {
public:
    MOCK_METHOD3( createSocket, void( int domain, int type, int protocol ) );
    MOCK_METHOD2( connect, int( const sockaddr* addr, socklen_t len ) );
    MOCK_METHOD3( connect, int( const std::string& address, const std::string& port, int addressFamily ) );
    MOCK_METHOD3( send, ssize_t( const void* buf, size_t n, int flags ) );
    MOCK_METHOD2( bind, int( const sockaddr* addr, socklen_t len ) );
    MOCK_METHOD3( setopt, int( int level, int name, int value ) );
    MOCK_METHOD3( setopt, int( int level, int name, const JsonObject& multicastData ) );
    MOCK_METHOD3( recv, ssize_t( void* buf, size_t n, int flags ) );
    MOCK_METHOD5( sendto, ssize_t( const void* buf, size_t n, int flags, const sockaddr* addr, socklen_t addrLen ) );
    MOCK_METHOD5( recvfrom, ssize_t( void* buf, size_t n, int flags, sockaddr* addr, socklen_t addrLen ) );
    MOCK_METHOD2( getsockname, int( sockaddr* addr, socklen_t addrLen ) );
    MOCK_METHOD0( close, int() );
    MOCK_METHOD1( shutdown, int( unsigned flags ) );
    MOCK_METHOD3( setTimeout, int( int socketLevel, int option, std::chrono::milliseconds timeoutMs ) );

    MOCK_METHOD0( valid, bool() );
};

}  // namespace UT
}  // namespace HubPlatform
