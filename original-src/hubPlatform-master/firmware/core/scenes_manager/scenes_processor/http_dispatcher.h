#pragma once

#include "http/http.h"
#include "http/event_emitter.h"
#include "jsonrpc/jsonrpc.h"

namespace HubPlatform {
namespace ScenesManager {

class HttpDispatcher {
public:
    using RequestId = CommonHttp::Http::RequestId;
    using EventHandler = CommonHttp::HttpEventEmitter::EventHandler;
    HttpDispatcher( EventHandler&& receiveHandler, EventHandler&& timeoutHandler );
    ~HttpDispatcher();

    RequestId request( const CommonHttp::HttpOptions& options );

private:
    CommonHttp::Http m_http;
    static std::atomic_bool s_initialized;
};

}  // namespace ScenesManager
}  // namespace HubPlatform
