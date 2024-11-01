#pragma once

#include "bus/messagesclasses.h"
#include "json_utils/json_fwd.h"

#include <string>

namespace HubPlatform {
namespace ExternalControllers {

class Messenger {
public:
    virtual ~Messenger() = default;

    virtual void requestItemsList( const std::string& controllerId ) = 0;
    virtual void requestDevicesList( const std::string& controllerId ) = 0;

    virtual void sendInternalBroadcast( MessageSubClasses eventType, const JsonObject& broadcast ) = 0;

    virtual void registerControllerConnection( const std::string& controllerId ) = 0;
    virtual void unregisterControllerConnection( const std::string& controllerId ) = 0;
};

}  // namespace ExternalControllers
}  // namespace HubPlatform
