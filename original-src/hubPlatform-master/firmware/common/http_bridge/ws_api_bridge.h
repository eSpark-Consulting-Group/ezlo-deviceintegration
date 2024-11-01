#pragma once

#include "http_bridge/endpoint.h"
#include "http_bridge/session.h"
#include "isession_manager.h"
#include "base_utils/event_emitter.h"
#include "tiny/tiny_ws.h"
#include "jsonrpc/message.h"
#include <libwebsockets.h>
#include <string>
#include <vector>
#include <memory>
#include <functional>

namespace HubPlatform {
namespace HttpBridge {

class MetricsBase;

class ISessionManager;
class AddonsEndpoint;
class ApiBridge;
class ApiBridgePeer : public tiny::ws::generic_sender {
    friend class ApiBridge;
    explicit ApiBridgePeer( SessionPtr ptr );

public:
    ~ApiBridgePeer() override;
    bool sendMessage( const std::string& msg, int nOpCode ) override;
    bool sendMessageText( const std::string& msg ) override;
    bool sendMessageBinary( const std::string& msg ) override;

protected:
    SessionPtr m_session;
};

using GenericSenderPtr = rpc::Message::GenericSenderPtr;

class ApiBridgeMessage : public rpc::Message {
public:
    explicit ApiBridgeMessage( GenericSenderPtr );
    ~ApiBridgeMessage() override;
    GenericSenderPtr getNetworkObject() const override;

public:
    ApiBridgeMessage() = delete;
    ApiBridgeMessage( const ApiBridgeMessage& ) = delete;
    ApiBridgeMessage( ApiBridgeMessage&& ) = delete;
    ApiBridgeMessage& operator=( const ApiBridgeMessage& ) = delete;
    ApiBridgeMessage& operator=( ApiBridgeMessage&& ) = delete;

private:
    GenericSenderPtr m_sender;
};

using ApiBridgeEventHandler = std::function< void( std::string, SessionPtr ) >;

/** Groups event handlers in a single class and allows to set them all with a single call to resetHandlers() */
class ApiBridgeHandlers {
public:
    virtual ~ApiBridgeHandlers() = default;

public:
    virtual bool authorize( const std::string& user, const std::string& token ) = 0;
    virtual bool processCommand( const std::string& handler, SessionPtr session, Json& answer ) = 0;

public:
    virtual void authorization( const std::string& handler, SessionPtr session ) = 0;
    virtual void message( const std::string& handler, SessionPtr session ) = 0;
    virtual void listenBroadcasts( const std::string& handler, SessionPtr session ) = 0;
};
using ApiBridgeHandlersPtr = std::unique_ptr< ApiBridgeHandlers >;

/** ApiBridge is an implementation of HTTP-mirror for web-sockets API.
 *  Emits events: message, listen_broadcasts, authorization.
 */
class ApiBridge : public Events::EventEmitter< ApiBridgeEventHandler, SessionPtr > {
public:
    using EventHandler = ApiBridgeEventHandler;
    ApiBridge();

    static int httpCallback( struct lws* wsi, enum lws_callback_reasons reason, void* user, void* in, size_t len );
    static int httpEventStreamCallback( struct lws* wsi,
                                        enum lws_callback_reasons reason,
                                        void* user,
                                        void* in,
                                        size_t len );
    static int httpCamerasCallback( struct lws* wsi,
                                    enum lws_callback_reasons reason,
                                    void* user,
                                    void* in,
                                    size_t len );

    static int httpAddonsCallback( struct lws* wsi,
                                   enum lws_callback_reasons reason,
                                   void* user,
                                   void* in,
                                   size_t len );

    static const lws_protocols* getProtocols( unsigned& length );
    static const lws_http_mount& getHttpMount();

    SessionPtr startSession( lws* wsi, const char* type );
    void finishSession( lws* wsi );
    SessionPtr findSession( lws* wsi ) const;

    bool registerExternalSession();
    void unregisterExternalSession();

    void sendBroadcast( const std::string& msg );
    size_t sessionCount() const;
    static int& parallelSessionsLimit();
    static int& requestDurationLimit();

    static GenericSenderPtr createBridgePeer( SessionPtr );
    void resetHandlers( ApiBridgeHandlersPtr );
    void resetMetricsHandlers();

    void resetAddonsEndpoint( AddonsEndpoint* addonsEndpoint = nullptr );

    Json getMetrics() const;

public:
    EndpointPtr& mainHttpEndpoint();
    EndpointPtr& broadcastsEndpoint();
    EndpointPtr& camerasAddonEndpoint();
    AddonsEndpoint* addonsEndpoint();

public:
    static int writeHttpStatusHeaders( SessionData& sessionData,
                                       int httpStatus,
                                       const char* contentType,
                                       uint8_t* start,
                                       uint8_t*& p,
                                       uint8_t* end,
                                       bool closeSession = false );
    static int writeHttpExpect100Response( lws* wsi );
    static int errorParallelSessionLimitReached( SessionData& sessionData,
                                                 size_t sessionCount,
                                                 uint8_t* start,
                                                 uint8_t*& p,
                                                 uint8_t* end,
                                                 bool closeSession = false );
    static bool getResponseError( const Json& answer, Json& error );

private:
    std::unique_ptr< ISessionManager > m_sessionManager;
    ApiBridgeHandlersPtr m_handlers;
    using MetricsBaseUPtr = std::unique_ptr< MetricsBase >;
    MetricsBaseUPtr m_metrics;

    EndpointPtr m_mainHttp{ nullptr };
    EndpointPtr m_broadcasts{ nullptr };
    EndpointPtr m_camerasAddon{ nullptr };
    AddonsEndpoint* m_addons{ nullptr };
};

ApiBridge& getApiBridge();

}  // namespace HttpBridge
}  // namespace HubPlatform
