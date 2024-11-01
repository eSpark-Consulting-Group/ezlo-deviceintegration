#include "http_bridge/taskloop_handlers.h"
#include "http_bridge/resources.h"
#include "service_interfaces/task_loop.h"

#include "base_utils/ezlodb.h"
#include "database_client/db_flags_names.h"

namespace HubPlatform {
namespace HttpBridge {

ApiBridgeHandlersTaskLoopBase::ApiBridgeHandlersTaskLoopBase( TaskLoop& taskLoop )
: m_service( taskLoop ) {}

bool ApiBridgeHandlersTaskLoopBase::authorize( const std::string& user, const std::string& token ) {
    return false;
}

bool ApiBridgeHandlersTaskLoopBase::processCommand( const std::string& handler, SessionPtr session, Json& answer ) {
    return false;
}

void ApiBridgeHandlersTaskLoopBase::authorization( const std::string& handler, SessionPtr session ) {
    using namespace HubPlatform::HttpBridge;

    if( !session.get() || !session->getData() )
        return;

    auto& sessionData = *session->getData();

    std::string user;
    std::string token;
    bool isLoggedIn = false;

    if( sessionData.findAuthHeaders( user, token ) ) {
        isLoggedIn = authorize( user, token );
#ifdef _DEBUG
        LOGGER_TRACE_PRINT( "DBG ONLY: auth: authenticateUser returns %s for (user: '%s', token: '%s')",
                            isLoggedIn ? "true" : "false",
                            user.c_str(),
                            token.c_str() );
#else
        LOGGER_TRACE_PRINT( "DBG ONLY: auth: authenticateUser returns %s", isLoggedIn ? "true" : "false" );
#endif
    }

    sessionData.isLoggedIn( isLoggedIn );
}

void ApiBridgeHandlersTaskLoopBase::message( const std::string& handler, SessionPtr session ) {
    using namespace HubPlatform::HttpBridge;

    if( !session.get() || !session->getData() )
        return;

    auto& sessionData = *session->getData();
    sessionData.isPending( true );

    Json jsonRequest = sessionData.getRequestByHandler( handler );
    LOGGER_TRACE_RAW( cc::trace( "apiBridge::onMessage: method " ) + handler + ": " + cc::j( jsonRequest ) );

    if( jsonRequest.empty() )
        return;

    auto transferRequest = [this, jsonRequest, session, handler]() {
        if( !session || !session->getData() )
            return;

        SessionData& sessionData = *session->getData();
        using SetIsPending = void ( * )( SessionData* );
        std::unique_ptr< SessionData, SetIsPending > deleter( &sessionData, []( SessionData* sessionData ) {
            if( sessionData )
                sessionData->isPending( false );
        } );

        auto& apiBridge = getApiBridge();
        apiBridge.emit( Resources::eventAuthorization, handler, session );

        if( !sessionData.isLoggedIn() && !Db::getFlag( OfflineAnonymousAccessFlagName ) ) {
            // answer should not be empty!
            sessionData.setAnswer( Json{ "error", "Authorization failed" } );
            sessionData.isPending( false );
            sessionData.notify();  // 401 Authorization failed
            return;
        }

        Json answer;
        bool isExecuted = processCommand( handler, session, answer );

        if( ( isExecuted && !answer.empty() ) || !isExecuted ) {
            sessionData.setAnswer( answer );
            sessionData.isPending( false );
            sessionData.notify();
        }
    };

    const bool isPeriodic = false;
    if( sessionData.delayed() > 0 ) {
        auto task = m_service.createPostponedTask( nullptr );
        task->setCallback( [transferRequest]( Services::PostponedTask& ) { transferRequest(); } );
        task->start( std::chrono::milliseconds( sessionData.delayed() ), isPeriodic );
    }
    else {
        m_service.addTask( std::move( transferRequest ), isPeriodic );
    }
}

void ApiBridgeHandlersTaskLoopBase::listenBroadcasts( const std::string& handler, SessionPtr session ) {
    using namespace HubPlatform::HttpBridge;
    LOGGER_TRACE_RAW( cc::trace( "http_bridge listen_broadcasts received: " ) );

    if( !session.get() || !session->getData() )
        return;

    auto& sessionData = *session->getData();
    sessionData.isPending( true );

    auto authRequest = [session, handler]() {
        if( !session.get() || !session->getData() )
            return;
        auto& sessionData = *session->getData();

        using SetIsPending = void ( * )( SessionData* );
        std::unique_ptr< SessionData, SetIsPending > deleter( &sessionData, []( SessionData* sessionData ) {
            if( sessionData )
                sessionData->isPending( false );
        } );

        auto& apiBridge = getApiBridge();
        apiBridge.emit( Resources::eventAuthorization, handler, session );

        if( !sessionData.isLoggedIn() && !Db::getFlag( OfflineAnonymousAccessFlagName ) ) {
            // answer should not be empty!
            sessionData.setAnswer( Json{ "error", "Authorization failed" } );
            sessionData.isPending( false );
        }
        sessionData.notify();  // 401 Authorization failed
    };

    m_service.addTask( std::move( authRequest ) );
}

}  // namespace HttpBridge
}  // namespace HubPlatform