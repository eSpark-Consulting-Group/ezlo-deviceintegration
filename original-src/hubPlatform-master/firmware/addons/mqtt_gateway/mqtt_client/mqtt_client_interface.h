#pragma once

#include <string>
#include <functional>
#include "json_utils/json_fwd.h"

namespace HubPlatform {
namespace MQTTGateway {

class MQTTClientInterface {
public:
    using MessageCallback = std::function< void( JsonObject&& ) >;
    virtual void setUp() = 0;
    virtual void tearDown() = 0;
    virtual void sendMessage( const std::string& topic, const JsonObject& msg ) = 0;
    virtual ~MQTTClientInterface() = default;
};

}  // namespace MQTTGateway
}  // namespace HubPlatform
