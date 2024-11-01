#pragma once

#include "http/event_emitter.h"
#include "json_utils/json_fwd.h"
#include <string>
#include <vector>
#include <map>
#include <mutex>

namespace HubPlatform {
namespace CommonHttp {

class HttpEventEmitterBase : public HttpEventEmitter {
public:
    void verbose( bool value );

    void dataSendEvent( const std::string& eventHandler, EventPtr eventPtr ) override;
    void dataReceived( const std::string& eventHandler, EventPtr eventPtr ) override;
    void connectionClosed( const std::string& eventHandler, EventPtr eventData ) override;
    void progressEvent( const std::string& eventHandler, EventPtr eventData ) override;
};

}  // namespace CommonHttp
}  // namespace HubPlatform