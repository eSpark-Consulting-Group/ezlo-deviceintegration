#pragma once
#include "json_utils/json_wrapper.h"
#include "bus/messagesclasses.h"

namespace HubPlatform {
namespace Uid {
namespace Extensions {

// Provide BusExtension if you want to communicate with other services via bus
class BusExtension {
public:
    virtual ~BusExtension() = default;

    virtual void handleMethodCall( const JsonObject& json,
                                   MessageSubClasses requestType,
                                   const std::string& sender ) = 0;
    virtual JsonObject handleRequestFromBus( const JsonObject& request, MessageSubClasses requestType ) = 0;
    virtual void handleEventFromBus( const JsonObject& event, MessageSubClasses requestType ) = 0;

public:
    static const Json& unhandledRequestAnswer() {
        static const Json answer = JSON( "unhandled" << true );
        return answer;
    }
};

}  // namespace Extensions
}  // namespace Uid
}  // namespace HubPlatform
