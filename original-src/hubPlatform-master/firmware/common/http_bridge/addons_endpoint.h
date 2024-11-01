#pragma once

#include <libwebsockets.h>
#include <memory>

namespace HubPlatform {
namespace HttpBridge {

class AddonsEndpoint {
public:
    virtual ~AddonsEndpoint() = default;

    virtual int endpointCallback( struct lws* wsi,
                                  enum lws_callback_reasons reason,
                                  void* user,
                                  void* in,
                                  size_t len ) = 0;
};

}  // namespace HttpBridge
}  // namespace HubPlatform
