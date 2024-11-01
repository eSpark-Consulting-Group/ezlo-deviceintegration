#pragma once

#include "mqtt_client/mqtt_message.h"

#include <functional>

namespace HubPlatform {
namespace Common {

using MQTTMessageCallback = std::function< void( MQTTMessage&& message ) >;

class MQTTClientInterface {
public:
    virtual ~MQTTClientInterface() = default;
    virtual void connect( std::string clientID,
                          std::string serverURI,
                          std::string username,
                          std::string password,
                          MQTTMessageCallback messageCallback ) = 0;
    virtual void disconnect() = 0;
    virtual void subscribe( const std::string& topic ) = 0;
    virtual void unsubscribe( const std::string& topic ) = 0;
    virtual void publish( const MQTTMessage& message ) = 0;
    virtual void publish( const std::string& topic, const std::string& payload ) = 0;
    virtual bool isConnected() = 0;
};

}  // namespace Common
}  // namespace HubPlatform
