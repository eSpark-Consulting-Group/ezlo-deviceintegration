#pragma once

#include <libwebsockets.h>
#include <memory>

namespace HubPlatform {
namespace HttpBridge {

class Endpoint;
using EndpointPtr = std::unique_ptr< Endpoint >;

/** Make static function pointing to non-static object */
class Endpoint {
public:
    Endpoint() = default;
    Endpoint( const Endpoint& ) = delete;
    Endpoint& operator=( const Endpoint& ) = delete;
    Endpoint( Endpoint&& ) = delete;
    Endpoint& operator=( const Endpoint&& ) = delete;

    virtual ~Endpoint() = default;

    virtual int endpointCallback( struct lws* wsi,
                                  enum lws_callback_reasons reason,
                                  void* user,
                                  void* in,
                                  size_t len ) = 0;
};

}  // namespace HttpBridge
}  // namespace HubPlatform
