#include "luad/modules/network/async_socket.h"
#include "luad/tests/mock_objects.h"

#include "common_utils/cpp14.h"
#include "test_utils/test_utils.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <cassert>
#include <memory>

#include <sys/socket.h>

/*

TESTPLAN

DONE    1. Async socket creation
DONE        1.1 Check internal socket gets created
DONE        1.2 Check each async socket has unique handle
DONE        1.3 Check internal socket gets created as non-blocking one
DONE        1.4 Check internal socket gets created as TCP one
DONE        1.5 Check internal socket gets created as UDP one
DONE        1.6 Check internal socket gets created in AF_INET family
TODO        1.7 Check internal socket gets created in AF_INET6 family
DONE        1.8 Check async socket's event handle is default initialized
DONE    2. Async socket connection
TODO        2.1 Check incorrect host in AF_INET family
TODO        2.2 Check incorrect host in AF_INET6 family
TODO        2.3 Check incorrect host name
DONE        2.4 Check connection gets invoked on internal socket
DONE        2.5 Check connection gets invoked on internal socket with EINPROGRESS
DONE        2.6 Check connection gets invoked on internal socket with zero
DONE        2.7 Check ConnectionEstablished event gets issued
DONE        2.8 Check throwing an exception in case of fail connection by local reason
DONE    3. Async socket events handler
DONE        3.1 Check setting of async socket events handler
TODO        3.2 Check keeping setting of async socket events handler while changing global event handler
DONE    4. Async socket sending
DONE        4.1 Check send gets invoked on internal socket
DONE        4.2 Check throwing exception in case of failure to send
DONE        4.3 Check throwing exception in case of failure to send by EWOULDBLOCK
DONE    5. Async socket receiving/inspecting
DONE        5.1 Check fetch on internal buffer gets invoked
DONE        5.2 Check inspect on internal buffer gets invoked
DONE        5.3 Check inspect data size on internal buffer gets invoked
DONE    6. Async socket closing
DONE        6.1 Check removing event handling
DONE        6.2 Check closing gets invoked on internal socket
DONE        6.3 Check async socket's event handle gets default initialized
DONE        6.4 Check order of operations during socket closing
DONE    7. Async socket destroying
DONE        7.1 Check removing event handling
DONE        7.2 Check closing gets invoked on internal socket
DONE    8. Async socket incoming data handler
DONE        8.1 Check IncommingData event gets issued
DONE        8.2 Check internal buffer gets data
DONE        8.3 Check ConnectionClosed event gets issued
DONE        8.4 Check removing event handling
DONE        8.5 Check closing gets invoked on internal socket
DONE        8.6 Check async socket's event handle gets default initialized
DONE    9. Async socket connection state changed handler
TODO        9.1 Check ConnectionEstablished event gets issued
TODO        9.2 Check IN event handler gets subscribed
TODO        9.3 Check FailedToConnect event gets issued
TODO        9.4 Check async socket gets shutdown
DONE   10. Asyn socket data source management
DONE       10.1 Check data source pausing
DONE       10.2 Check data source restoring

*/

namespace HubPlatform {
namespace UT {

using namespace LuaNetwork;

using ::testing::_;
using ::testing::AtLeast;
using ::testing::InSequence;
using ::testing::Return;
using ::testing::SetErrnoAndReturn;

class AsyncSocketTestSuite : public testing::Test {
protected:
    AsyncSocketTestSuite()
    : m_buffer( std::make_unique< MockBuffer >() )
    , m_socket( std::make_unique< MockSocket >() ) {}

    AsyncSocket& getAsyncSocket( const RemoteAddress& remoteAddress,
                                 const AsyncSocket::AsyncSocketEventsHandler& eventsHandler ) {
        assert( !m_asyncSocket );
        m_asyncSocket = makeAsyncSocket( remoteAddress, eventsHandler );

        return *m_asyncSocket;
    }

    AsyncSocket& getAsyncSocket( const RemoteAddress& remoteAddress ) {
        assert( !m_asyncSocket );
        m_asyncSocket = makeAsyncSocket( remoteAddress, "" );

        return *m_asyncSocket;
    }

    AsyncSocket& getAsyncSocket( const AsyncSocket::AsyncSocketEventsHandler& eventsHandler ) {
        assert( !m_asyncSocket );
        m_asyncSocket = makeAsyncSocket( RemoteAddress{ "128.0.0.1", "8210", ConnectionType::TCP }, eventsHandler );

        return *m_asyncSocket;
    }

    AsyncSocket& getAsyncSocket() {
        assert( !m_asyncSocket );
        m_asyncSocket = makeAsyncSocket( RemoteAddress{ "128.0.0.1", "8210", ConnectionType::TCP }, "" );

        return *m_asyncSocket;
    }

    const AsyncSocket& getAsyncSocket() const {
        assert( m_asyncSocket );
        return *m_asyncSocket;
    }

    MockEventsLoop& getEventsLoop() {
        return m_eventsLoop;
    }

    MockEventsEmitter& getEventsEmitter() {
        return m_eventsEmitter;
    }

    MockBuffer& getBuffer() {
        assert( m_buffer );
        return *m_buffer;
    }

    MockSocket& getSocket() {
        assert( m_socket );
        return *m_socket;
    }

    int getAsyncSocketGlobalHandler() const {
        return AsyncSocket::m_handleCounter;
    }

    int getAsyncSocketEventHandle() const {
        assert( m_asyncSocket );
        return m_asyncSocket->m_eventSourceHandle;
    }

    void setAsyncSocketEventHandle( int eventHandle ) {
        assert( m_asyncSocket );
        m_asyncSocket->m_eventSourceHandle = eventHandle;
    }

    const std::string& getAsyncSocketEventsHandler() const {
        assert( m_asyncSocket );
        return m_asyncSocket->m_eventsHandler;
    }

    void pauseAsyncSocketEventHandle() {
        assert( m_asyncSocket );
        m_asyncSocket->pauseDataSource();
    }

    void resumeAsyncSocketEventHandle() {
        assert( m_asyncSocket );
        m_asyncSocket->resumeDataSource();
    }

private:
    std::unique_ptr< AsyncSocket > makeAsyncSocket( const RemoteAddress& remoteAddress,
                                                    const AsyncSocket::AsyncSocketEventsHandler& eventsHandler ) {
        return std::unique_ptr< AsyncSocket >{ new AsyncSocket( m_eventsLoop,
                                                                m_eventsEmitter,
                                                                remoteAddress,
                                                                eventsHandler,
                                                                std::move( m_buffer ),
                                                                std::move( m_socket ) ) };
    }

private:
    MockEventsLoop m_eventsLoop;
    MockEventsEmitter m_eventsEmitter;
    std::unique_ptr< MockBuffer > m_buffer;
    std::unique_ptr< MockSocket > m_socket;

    std::unique_ptr< AsyncSocket > m_asyncSocket;
};

TEST_F( AsyncSocketTestSuite, testCheckInternalSocketGetsCreated /* 1.1 */ ) {
    auto& socket = getSocket();

    EXPECT_CALL( socket, createSocket( _, _, _ ) ).Times( 1 );

    getAsyncSocket();
}

TEST_F( AsyncSocketTestSuite, testCheckEachAsyncSocketHasUniqueHandle /* 1.2 */ ) {
    const auto initialCounterValue = getAsyncSocketGlobalHandler();

    const auto& asyncSocket = getAsyncSocket();

    EXPECT_EQ( asyncSocket.getHandle(), initialCounterValue );
    EXPECT_NE( asyncSocket.getHandle(), getAsyncSocketGlobalHandler() );
}

TEST_F( AsyncSocketTestSuite, testCheckInternalSocketGetsCreatedNoBlocking /* 1.3 */ ) {
    auto& socket = getSocket();

    EXPECT_CALL( socket, createSocket( _, SOCK_STREAM | SOCK_NONBLOCK, _ ) );

    getAsyncSocket();
}

TEST_F( AsyncSocketTestSuite, testCheckInternalSocketGetsCreatedAsTCP /* 1.4 */ ) {
    auto& socket = getSocket();

    EXPECT_CALL( socket, createSocket( _, SOCK_STREAM | SOCK_NONBLOCK, _ ) );

    getAsyncSocket();
}

TEST_F( AsyncSocketTestSuite, testCheckInternalSocketGetsCreatedAsUDP /* 1.5 */ ) {
    auto& socket = getSocket();

    EXPECT_CALL( socket, createSocket( _, SOCK_DGRAM | SOCK_NONBLOCK, _ ) );

    getAsyncSocket( RemoteAddress{ "", "", ConnectionType::UDP } );
}

TEST_F( AsyncSocketTestSuite, testCheckInternalSocketGetsCreatedInAFINETFamily /* 1.6 */ ) {
    auto& socket = getSocket();

    EXPECT_CALL( socket, createSocket( AF_INET, _, _ ) );

    getAsyncSocket();
}

TEST_F( AsyncSocketTestSuite, testCheckSocketEventHandlerIsDefaultInitialized /* 1.8 */ ) {
    getAsyncSocket();

    EXPECT_EQ( getAsyncSocketEventHandle(), -1 );
}

TEST_F( AsyncSocketTestSuite, testCheckInternalSocketConnectionGetsInvoked /* 2.4 */ ) {
    auto& socket = getSocket();

    EXPECT_CALL( socket, connect( _, _ ) );

    auto& asyncSocket = getAsyncSocket();
    asyncSocket.connect( 0, std::weak_ptr< AsyncSocket >{} );
}

TEST_F( AsyncSocketTestSuite, testCheckInternalSocketConnectionGetsInvokedEINPROGRESS /* 2.5 */ ) {
    auto& socket = getSocket();
    auto& eventsLoop = getEventsLoop();

    {
        InSequence dummy;
        EXPECT_CALL( socket, connect( _, _ ) ).WillOnce( SetErrnoAndReturn( EINPROGRESS, -1 ) );
        EXPECT_CALL( eventsLoop, addEventSource( _, EventType::OUT, _ ) ).WillOnce( Return( 28 ) );
    }

    auto& asyncSocket = getAsyncSocket();
    asyncSocket.connect( 0, std::weak_ptr< AsyncSocket >{} );

    EXPECT_EQ( getAsyncSocketEventHandle(), 28 );
}

TEST_F( AsyncSocketTestSuite, testCheckInternalSocketConnectionGetsInvokedZero /* 2.6 */ ) {
    auto& socket = getSocket();
    auto& eventsLoop = getEventsLoop();
    auto& eventsEmitter = getEventsEmitter();

    {
        InSequence dummy;
        EXPECT_CALL( socket, connect( _, _ ) ).WillOnce( Return( 0 ) );
        EXPECT_CALL( eventsLoop, addEventSource( _, EventType::IN, _ ) ).WillOnce( Return( 28 ) );
        EXPECT_CALL( eventsEmitter, emitConnectionEstablishedEvent( "2810", _ ) );
    }

    auto& asyncSocket = getAsyncSocket( "2810" );
    asyncSocket.connect( 0, std::weak_ptr< AsyncSocket >{} );

    EXPECT_EQ( getAsyncSocketEventHandle(), 28 );
}

TEST_F( AsyncSocketTestSuite, testCheckConnectionEstablishedEventGetsIssued /* 2.7 */ ) {
    auto& eventsEmitter = getEventsEmitter();

    EXPECT_CALL( eventsEmitter, emitConnectionEstablishedEvent( "2810", _ ) );

    auto& asyncSocket = getAsyncSocket( "2810" );
    asyncSocket.connect( 0, std::weak_ptr< AsyncSocket >{} );
}

TEST_F( AsyncSocketTestSuite, testCheckThrowingExceptionForFailureConnection /* 2.8 */ ) {
    auto& socket = getSocket();

    EXPECT_CALL( socket, connect( _, _ ) ).WillOnce( SetErrnoAndReturn( ENOMEDIUM, -1 ) );

    auto& asyncSocket = getAsyncSocket();

    ASSERT_THROW_WITH_DESCRIPTION( [&asyncSocket]() { asyncSocket.connect( 0, std::weak_ptr< AsyncSocket >{} ); },
                                   AsyncSocketError,
                                   "AsyncSocket: fail to establish the connection by some local reason" );
}

TEST_F( AsyncSocketTestSuite, testCheckSettingAsyncSocketEventsHandler /* 3.1 */ ) {
    getAsyncSocket( "2810" );

    EXPECT_EQ( getAsyncSocketEventsHandler(), "2810" );
}

TEST_F( AsyncSocketTestSuite, testCheckSendGetsInvokedForInternalSocket /* 4.1 */ ) {
    auto& socket = getSocket();
    const std::string data = "wassup";

    EXPECT_CALL( socket, send( data.c_str(), data.size(), MSG_NOSIGNAL ) );

    auto& asyncSocket = getAsyncSocket();
    asyncSocket.send( data );
}

TEST_F( AsyncSocketTestSuite, testCheckThrowingExceptionForFailureSending /* 4.2 */ ) {
    auto& socket = getSocket();

    EXPECT_CALL( socket, send( _, _, _ ) ).WillOnce( SetErrnoAndReturn( ENOMEDIUM, -1 ) );

    auto& asyncSocket = getAsyncSocket();

    ASSERT_THROW_WITH_DESCRIPTION(
        [&asyncSocket]() { asyncSocket.send( "wassup" ); }, AsyncSocketError, "AsyncSocket: No medium found" );
}

TEST_F( AsyncSocketTestSuite, testCheckThrowingExceptionForFailureSendingEWOULDBLOCK /* 4.3 */ ) {
    auto& socket = getSocket();

    EXPECT_CALL( socket, send( _, _, _ ) ).WillOnce( SetErrnoAndReturn( EWOULDBLOCK, -1 ) );

    auto& asyncSocket = getAsyncSocket();

    ASSERT_THROW_WITH_DESCRIPTION( [&asyncSocket]() { asyncSocket.send( "wassup" ); },
                                   AsyncSocketError,
                                   "AsyncSocket: Resource temporarily unavailable" );
}

TEST_F( AsyncSocketTestSuite, testCheckThrowingExceptionForFailureSendingEAGAIN /* 5.1 */ ) {
    auto& socket = getSocket();

    EXPECT_CALL( socket, send( _, _, _ ) ).WillOnce( SetErrnoAndReturn( EAGAIN, -1 ) );

    auto& asyncSocket = getAsyncSocket();
    ASSERT_THROW_WITH_DESCRIPTION( [&asyncSocket]() { asyncSocket.send( "wassup" ); },
                                   AsyncSocketError,
                                   "AsyncSocket: Resource temporarily unavailable" );
}

TEST_F( AsyncSocketTestSuite, testCheckFetchOnInternalBufferGetsInvoked /* 5.2 */ ) {
    auto& buffer = getBuffer();

    EXPECT_CALL( buffer, fetchData( 2810 ) ).WillOnce( Return( "2810" ) );

    auto& asyncSocket = getAsyncSocket();
    EXPECT_EQ( asyncSocket.receive( 2810 ), "2810" );
}

TEST_F( AsyncSocketTestSuite, testCheckInspectOnInternalBufferGetsInvoked /* 5.3 */ ) {
    auto& buffer = getBuffer();

    EXPECT_CALL( buffer, inspectData( 2810 ) ).WillOnce( Return( "2810" ) );

    auto& asyncSocket = getAsyncSocket();
    EXPECT_EQ( asyncSocket.inspectData( 2810 ), "2810" );
}

TEST_F( AsyncSocketTestSuite, testCheckInspectDataSizeOnInternalBufferGetsInvoked /* 6.1 */ ) {
    auto& buffer = getBuffer();

    EXPECT_CALL( buffer, inspectDataSize() ).WillOnce( Return( 28 ) );

    auto& asyncSocket = getAsyncSocket();
    EXPECT_EQ( asyncSocket.inspectDataSize(), 28 );
}

TEST_F( AsyncSocketTestSuite, testCheckRemovingEventHandling /* 6.2 */ ) {
    auto& eventsLoop = getEventsLoop();

    EXPECT_CALL( eventsLoop, removeEventSource( _ ) ); /* an asyn socket dtor calls close() too */
    EXPECT_CALL( eventsLoop, removeEventSource( 2810 ) );

    auto& asyncSocket = getAsyncSocket();
    setAsyncSocketEventHandle( 2810 );
    asyncSocket.close();
}

TEST_F( AsyncSocketTestSuite, testCheckClosingGetsInvokedOnInternalSocket /* 6.3 */ ) {
    auto& socket = getSocket();

    EXPECT_CALL( socket, close() ).Times( 2 ); /* an asyn socket dtor calls close() too */

    getAsyncSocket().close();
}

TEST_F( AsyncSocketTestSuite, testCheckSocketEventHandlerIsDefaultInitializedDuringClosing /* 1.1 */ ) {
    getAsyncSocket().close();

    EXPECT_EQ( getAsyncSocketEventHandle(), -1 );
}

TEST_F( AsyncSocketTestSuite, testCheckOrderOfOperationsDuringSocketClosing /* 6.4 */ ) {
    auto& eventsLoop = getEventsLoop();
    auto& socket = getSocket();

    {
        InSequence dummy;

        EXPECT_CALL( eventsLoop, removeEventSource( _ ) );
        EXPECT_CALL( socket, close() );
        EXPECT_CALL( eventsLoop, removeEventSource( _ ) ); /* an asyn socket dtor calls close() too */
        EXPECT_CALL( socket, close() );
    }

    getAsyncSocket().close();
}

TEST_F( AsyncSocketTestSuite, testCheckRemovingEventHandlingDuringDestroying /* 7.1 */ ) {
    auto& eventsLoop = getEventsLoop();

    EXPECT_CALL( eventsLoop, removeEventSource( _ ) );
    getAsyncSocket();
}

TEST_F( AsyncSocketTestSuite, testCheckClosingInternalSocketDuringDestroying /* 7.2 */ ) {
    auto& socket = getSocket();

    EXPECT_CALL( socket, close() );
    getAsyncSocket();
}

TEST_F( AsyncSocketTestSuite, testCheckIncomingDataEventGetsIssued /* 8.1 */ ) {
    auto& eventsEmitter = getEventsEmitter();

    EXPECT_CALL( eventsEmitter, emitIncomingDataEvent( "2810", _ ) );

    auto& asyncSocket = getAsyncSocket();
    asyncSocket.setEventsHandler( "2810" );
    asyncSocket.onIncommingData();
}

TEST_F( AsyncSocketTestSuite, testCheckInternalBufferGetsData /* 8.2 */ ) {
    auto& buffer = getBuffer();
    auto& socket = getSocket();

    EXPECT_CALL( buffer, front() );
    EXPECT_CALL( buffer, size() );

    {
        InSequence dummy;

        EXPECT_CALL( socket, recv( _, _, _ ) ).WillOnce( Return( 28 ) );
        EXPECT_CALL( buffer, commit( 28 ) );
    }

    getAsyncSocket().onIncommingData();
}

TEST_F( AsyncSocketTestSuite, testCheckConnectionClosedEventGetsIssued /* 8.3 */ ) {
    auto& socket = getSocket();
    auto& eventsEmitter = getEventsEmitter();

    EXPECT_CALL( socket, recv( _, _, _ ) ).WillOnce( Return( 0 ) );
    EXPECT_CALL( eventsEmitter, emitConnectionClosedEvent( "2810", _ ) );

    auto& asyncSocket = getAsyncSocket();
    asyncSocket.setEventsHandler( "2810" );
    asyncSocket.onIncommingData();
}

TEST_F( AsyncSocketTestSuite, testCheckRemovingEventHandlingDuringIncommingDataHandling /* 8.4 */ ) {
    auto& socket = getSocket();
    auto& eventsLoop = getEventsLoop();

    EXPECT_CALL( eventsLoop, removeEventSource( _ ) ); /* an asyn socket dtor calls close() too */
    EXPECT_CALL( eventsLoop, removeEventSource( 2810 ) );
    EXPECT_CALL( socket, recv( _, _, _ ) ).WillOnce( Return( 0 ) );

    auto& asyncSocket = getAsyncSocket();
    setAsyncSocketEventHandle( 2810 );
    asyncSocket.onIncommingData();
}

TEST_F( AsyncSocketTestSuite, testCheckClosingGetsInvokedOnInternalSocketDuringIncommingDataHandling /* 8.5 */ ) {
    auto& socket = getSocket();

    EXPECT_CALL( socket, close() ).Times( 2 ); /* an asyn socket dtor calls close() too */
    EXPECT_CALL( socket, recv( _, _, _ ) ).WillOnce( Return( 0 ) );

    getAsyncSocket().onIncommingData();
}

TEST_F( AsyncSocketTestSuite, testSocketEventHandlerIsDefaultInitializedDuringIncommingDataHandling /* 8.6 */ ) {
    auto& socket = getSocket();

    EXPECT_CALL( socket, recv( _, _, _ ) ).WillOnce( Return( 0 ) );

    getAsyncSocket().onIncommingData();
    EXPECT_EQ( getAsyncSocketEventHandle(), -1 );
}

TEST_F( AsyncSocketTestSuite, DISABLED_testCheckConnectionEstablishedEventGetsIssued_1 /* 9.1 */ ) {
    auto& eventsEmitter = getEventsEmitter();

    EXPECT_CALL( eventsEmitter, emitConnectionEstablishedEvent( "2810", _ ) );

    auto& asyncSocket = getAsyncSocket();
    asyncSocket.setEventsHandler( "2810" );
    asyncSocket.onConnectionStateChanged( std::weak_ptr< AsyncSocket >{} );
}

TEST_F( AsyncSocketTestSuite, DISABLED_testCheckInEventHandlerGetsSubscribed /* 9.2 */ ) {
    auto& eventsLoop = getEventsLoop();

    EXPECT_CALL( eventsLoop, removeEventSource( 2502 ) );  // an asyn socket dtor calls close() too

    {
        InSequence dummy;

        EXPECT_CALL( eventsLoop, removeEventSource( 2810 ) );
        EXPECT_CALL( eventsLoop, addEventSource( _, EventType::IN, _ ) ).WillOnce( Return( 2502 ) );
    }

    auto& asyncSocket = getAsyncSocket();
    setAsyncSocketEventHandle( 2810 );
    asyncSocket.onConnectionStateChanged( std::weak_ptr< AsyncSocket >{} );
}

TEST_F( AsyncSocketTestSuite, testCheckDataSourcePausing /* 10.1 */ ) {
    auto& eventsLoop = getEventsLoop();

    EXPECT_CALL( eventsLoop, pauseEventSource( 2810 ) );

    auto& asyncSocket = getAsyncSocket();
    setAsyncSocketEventHandle( 2810 );
    pauseAsyncSocketEventHandle();
}

TEST_F( AsyncSocketTestSuite, testCheckDataSourceRestoring /* 10.2 */ ) {
    auto& eventsLoop = getEventsLoop();

    EXPECT_CALL( eventsLoop, resumeEventSource( 2810 ) );

    auto& asyncSocket = getAsyncSocket();
    setAsyncSocketEventHandle( 2810 );
    resumeAsyncSocketEventHandle();
}

}  // namespace UT
}  // namespace HubPlatform
