#pragma once

#include "http_bridge/endpoint.h"
#include "http_bridge/endpoint_handlers.h"
#include "http_bridge/command.h"
#include "http_bridge/session.h"

namespace HubPlatform {

namespace HttpBridge {
class ISessionManager;
class Session;
}  // namespace HttpBridge

namespace Tests {

class ReflectEndpoint : public HttpBridge::Endpoint, public HttpBridge::EndpointHandlers< ReflectEndpoint > {
public:
    explicit ReflectEndpoint( HttpBridge::ISessionManager& sessions );
    static HttpBridge::EndpointPtr makeEndpoint( HttpBridge::ISessionManager& sessions );

public:
    ~ReflectEndpoint() override = default;
    int endpointCallback( struct lws* wsi,
                          enum lws_callback_reasons reason,
                          void* user,
                          void* in,
                          size_t len ) override;

protected:
    virtual int httpDummy( HttpBridge::CallbackParams params );
    virtual int onHttp( HttpBridge::CallbackParams params, HttpBridge::BufferPosition bufPos );
    virtual int onHttpWriteable( HttpBridge::CallbackParams params, HttpBridge::BufferPosition bufPos );
    virtual int onHttpBody( HttpBridge::CallbackParams params, HttpBridge::BufferPosition bufPos );
    virtual int onHttpBodyCompletion( HttpBridge::CallbackParams params, HttpBridge::BufferPosition bufPos );
    virtual int onTimer( HttpBridge::CallbackParams params, HttpBridge::BufferPosition bufPos );
    virtual int onClosedHttp( HttpBridge::CallbackParams params, HttpBridge::BufferPosition bufPos );

private:
    void emitMessage( const std::string& httpMethod, HttpBridge::SessionPtr session );

    static void appendMultipleHeaders( HttpBridge::CallbackParams params,
                                       HttpBridge::BufferPosition& bufPos,
                                       std::string headerName,
                                       int repeatCount );
    static int httpRedirect( HttpBridge::CallbackParams params,
                             HttpBridge::BufferPosition& bufPos,
                             const std::string& location,
                             int redirectCode = HTTP_STATUS_MOVED_PERMANENTLY );
    static std::string makeRedirectLocation( const HttpBridge::HttpBridgeCommand::Params& params,
                                             const std::string& urlBase,
                                             int redirectCounter );

private:
    HttpBridge::ISessionManager& m_sessions;
};

}  // namespace Tests
}  // namespace HubPlatform