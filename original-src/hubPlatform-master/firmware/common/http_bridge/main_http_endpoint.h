#pragma once

#include "http_bridge/endpoint.h"
#include "http_bridge/endpoint_handlers.h"

namespace HubPlatform {
namespace HttpBridge {

class ApiBridge;
class SessionData;

class MainHttpEndpoint : public Endpoint, public EndpointHandlers< MainHttpEndpoint > {
public:
    explicit MainHttpEndpoint( ApiBridge& apiBridge );
    static EndpointPtr makeEndpoint( ApiBridge& apiBridge );

public:
    ~MainHttpEndpoint() override = default;
    int endpointCallback( struct lws* wsi,
                          enum lws_callback_reasons reason,
                          void* user,
                          void* in,
                          size_t len ) override;

protected:
    virtual int httpDummy( CallbackParams params );
    virtual int onHttp( CallbackParams params, BufferPosition bufPos );
    virtual int onHttpWriteable( CallbackParams params, BufferPosition bufPos );
    virtual int onHttpBody( CallbackParams params, BufferPosition bufPos );
    virtual int onHttpBodyCompletion( CallbackParams params, BufferPosition bufPos );
    virtual int onTimer( CallbackParams params, BufferPosition bufPos );
    virtual int onClosedHttp( CallbackParams params, BufferPosition bufPos );

private:
    void setEtagHeaderIfNeeded( SessionData& sessionData ) const;

protected:
    ApiBridge& m_api;
};

}  // namespace HttpBridge
}  // namespace HubPlatform