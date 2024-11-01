#pragma once
#include "http_bridge/ws_api_bridge.h"

namespace HubPlatform {
namespace Services {
class TaskLoop;
}

namespace HttpBridge {

class ApiBridgeHandlersTaskLoopBase : public HttpBridge::ApiBridgeHandlers {
public:
    using SessionPtr = HttpBridge::SessionPtr;
    using TaskLoop = Services::TaskLoop;
    explicit ApiBridgeHandlersTaskLoopBase( TaskLoop& taskLoop );

public:
    bool authorize( const std::string& user, const std::string& token ) override;
    bool processCommand( const std::string& handler, SessionPtr session, Json& answer ) override;

public:
    void authorization( const std::string&, SessionPtr ) override;
    void message( const std::string&, SessionPtr ) override;
    void listenBroadcasts( const std::string&, SessionPtr ) override;

private:
    Services::TaskLoop& m_service;
};

/* Case 1: Uid */
/*
 * if( !m_service.getNmaClient() )
        return false;

    bool isExecuted = m_service.getNmaClient()->getUiProto().processCmd( message );

    answer = message.getAnswer();
*/
template < class Service >
class ExecuteAsUidTask {
public:
    static bool runTask( Service& service, rpc::Message& message ) {
        if( !service.getNmaClient() )
            return false;

        return service.getNmaClient()->getUiProto().processCmd( message );
    }
};

/* Case 2: NmaTestApplication */
/*
 *     answer = m_service.sendtoHubAndGetAnswer( message.getMessage() );
 * */
template < class Service >
class ExecuteAsNmaTestAppTask {
public:
    static bool runTask( Service& service, rpc::Message& message ) {
        Json answer = service.sendtoHubAndGetAnswer( message.getMessage() );
        message.setAnswer( answer );
        return true;
    }
};

template < class Service, class OfflineAuth, class ExecuteTask = ExecuteAsUidTask< Service > >
class UidApiBridgeHandlers : public HttpBridge::ApiBridgeHandlersTaskLoopBase {
public:
    UidApiBridgeHandlers( Service& uidService );

    using SessionPtr = HttpBridge::SessionPtr;

public:
    bool authorize( const std::string& user, const std::string& token ) override;
    bool processCommand( const std::string& handler, SessionPtr session, Json& answer ) override;

private:
    Service& m_service;
};

template < class Service, class OfflineAuth, class ExecuteTask >
inline UidApiBridgeHandlers< Service, OfflineAuth, ExecuteTask >::UidApiBridgeHandlers( Service& uidService )
: ApiBridgeHandlersTaskLoopBase( uidService )
, m_service( uidService ) {}

template < class Service, class OfflineAuth, class ExecuteTask >
inline bool UidApiBridgeHandlers< Service, OfflineAuth, ExecuteTask >::authorize( const std::string& user,
                                                                                  const std::string& token ) {
    if( !user.empty() && !token.empty() )
        return OfflineAuth::authenticateUser( user, token );
    return false;
}

template < class Service, class OfflineAuth, class ExecuteTask >
inline bool UidApiBridgeHandlers< Service, OfflineAuth, ExecuteTask >::processCommand( const std::string& handler,
                                                                                       SessionPtr session,
                                                                                       Json& answer ) {
    using namespace HubPlatform::HttpBridge;
    if( !session.get() || !session->getData() )
        return false;

    auto& sessionData = *session->getData();
    const Json& jsonRequest = sessionData.getRequestByHandler( handler );

    HttpBridge::GenericSenderPtr peer = ApiBridge::createBridgePeer( session );
    ApiBridgeMessage message( peer );

    message.setMessage( jsonRequest );
    //    if( !m_service.getNmaClient() )
    //        return false;
    //
    //    bool isExecuted = m_service.getNmaClient()->getUiProto().processCmd( message );
    bool isExecuted = ExecuteTask::runTask( m_service, message );

    answer = message.getAnswer();
    return isExecuted;
}

template < class Service, class OfflineAuth, class ExecuteTask = ExecuteAsNmaTestAppTask< Service > >
class NmaTestAppApiBridgeHandlers : public UidApiBridgeHandlers< Service, OfflineAuth, ExecuteTask > {
public:
    NmaTestAppApiBridgeHandlers( Service& uidService );

    using SessionPtr = HttpBridge::SessionPtr;

public:
    bool processCommand( const std::string& handler, SessionPtr session, Json& answer ) override;

private:
    Service& m_service;
};

template < class Service, class OfflineAuth, class ExecuteTask >
inline NmaTestAppApiBridgeHandlers< Service, OfflineAuth, ExecuteTask >::NmaTestAppApiBridgeHandlers(
    Service& uidService )
: UidApiBridgeHandlers< Service, OfflineAuth, ExecuteTask >( uidService )
, m_service( uidService ) {}

template < class Service, class OfflineAuth, class ExecuteTask >
inline bool NmaTestAppApiBridgeHandlers< Service, OfflineAuth, ExecuteTask >::processCommand(
    const std::string& handler,
    SessionPtr session,
    Json& answer ) {
    //    return UidApiBridgeHandlers::processCommand(handler, session, answer);
    using namespace HubPlatform::HttpBridge;
    if( !session.get() || !session->getData() )
        return false;

    auto& sessionData = *session->getData();
    const Json& jsonRequest = sessionData.getRequestByHandler( handler );

    HttpBridge::GenericSenderPtr peer = ApiBridge::createBridgePeer( session );
    peer->ref_retain();  // manual ref management
    ApiBridgeMessage message( peer );

    message.setMessage( jsonRequest );
    // answer = m_service.sendtoHubAndGetAnswer( message.getMessage() );
    ExecuteTask::runTask( m_service, message );
    answer = message.getAnswer();

    LOGGER_TRACE_RAW( cc::trace( "Http Answer: " ) + cc::j( answer ) );

    if( !answer.empty() ) {
        sessionData.setAnswer( answer );
        sessionData.isPending( false );
        sessionData.notify();
    }
    peer->ref_release();  // manual ref management
    return true;
}

}  // namespace HttpBridge
}  // namespace HubPlatform