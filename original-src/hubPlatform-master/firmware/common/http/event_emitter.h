#pragma once
#include "base_utils/event_emitter.h"
#include "http/http_event.h"
#include "json_utils/json_fwd.h"
#include <string>
#include <functional>

namespace HubPlatform {
namespace CommonHttp {

using EventHandler = std::function< void( const std::string& eventHandler, const EventPtr& ) >;

class HttpEventEmitter : public Events::EventEmitter< EventHandler, EventPtr > {
public:
    using EventHandler = HubPlatform::CommonHttp::EventHandler;
    HttpEventEmitter() = default;

public:
    ~HttpEventEmitter() override = default;

public:
    virtual void dataSendEvent( const std::string& eventHandler, EventPtr eventData ) = 0;
    virtual void dataReceived( const std::string& eventHandler, EventPtr eventData ) = 0;
    virtual void connectionClosed( const std::string& eventHandler, EventPtr eventData ) = 0;
    virtual void progressEvent( const std::string& eventHandler, EventPtr eventData ) = 0;
};

}  // namespace CommonHttp
}  // namespace HubPlatform