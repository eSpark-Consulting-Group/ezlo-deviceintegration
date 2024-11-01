#pragma once

#include "http_bridge/endpoint.h"
#include "http_bridge/endpoint_handlers.h"

namespace HubPlatform {
namespace HttpBridge {

class ApiBridge;

class BroadcastsEndpoint : public Endpoint, public EndpointHandlers< BroadcastsEndpoint > {
public:
    explicit BroadcastsEndpoint( ApiBridge& apiBridge );
    static EndpointPtr makeEndpoint( ApiBridge& apiBridge );

    ~BroadcastsEndpoint() override = default;
    int endpointCallback( struct lws* wsi,
                          enum lws_callback_reasons reason,
                          void* user,
                          void* in,
                          size_t len ) override;

protected:
    virtual int httpDummy( CallbackParams params );
    virtual int onHttp( CallbackParams params, BufferPosition bufPos );
    virtual int onHttpWriteable( CallbackParams params, BufferPosition bufPos );
    virtual int onClosedHttp( CallbackParams params, BufferPosition bufPos );

protected:
    ApiBridge& m_api;
};

}  // namespace HttpBridge
}  // namespace HubPlatform