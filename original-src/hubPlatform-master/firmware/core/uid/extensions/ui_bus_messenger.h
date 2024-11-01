#pragma once
#include "json_utils/json_fwd.h"
#include "bus/messagesclasses.h"

namespace HubPlatform {
namespace Uid {
namespace Extensions {

// Use it in your BusExtension to send messages to bus
class BusMessenger {
public:
    virtual ~BusMessenger() = default;

    virtual void sendMethodCall( const std::string& serviceName,
                                 MessageSubClasses requestType,
                                 const JsonObject& data ) = 0;
    virtual JsonObject sendRequest( const std::string& serviceName,
                                    MessageSubClasses requestType,
                                    const JsonObject& request ) = 0;
    virtual void handleEventFromBus( const JsonObject& event, MessageSubClasses requestType ) = 0;
};

}  // namespace Extensions
}  // namespace Uid
}  // namespace HubPlatform
