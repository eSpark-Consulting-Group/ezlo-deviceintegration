#pragma once

#include <map>

namespace HubPlatform {
namespace HttpBridge {

struct CallbackParams {
    CallbackParams( struct lws* wsi, enum lws_callback_reasons reason, void* user, void* in, size_t len )
    : m_wsi( wsi )
    , m_reason( reason )
    , m_user( user )
    , m_in( in )
    , m_len( len ){};

public:
    struct lws* m_wsi;
    enum lws_callback_reasons m_reason;
    void* m_user;
    void* m_in;
    size_t m_len;
};

struct BufferPosition {
    BufferPosition( uint8_t* start, uint8_t*& cur, uint8_t* end )
    : m_start( start )
    , m_p( cur )
    , m_end( end ) {}

public:
    uint8_t* m_start{ nullptr };
    uint8_t*& m_p;
    uint8_t* m_end{ nullptr };
};

int httpDummy( CallbackParams& params );

template < class EndpointImpl >
class EndpointHandlers {
public:
    using ReasonHandler = int ( EndpointImpl::* )( HubPlatform::HttpBridge::CallbackParams params,
                                                   HubPlatform::HttpBridge::BufferPosition bufPos );
    using MethodHandlers = std::map< lws_callback_reasons, ReasonHandler >;
    explicit EndpointHandlers( MethodHandlers&& handlers )
    : m_handlers{ std::move( handlers ) } {}

protected:
    MethodHandlers m_handlers;
};

}  // namespace HttpBridge
}  // namespace HubPlatform