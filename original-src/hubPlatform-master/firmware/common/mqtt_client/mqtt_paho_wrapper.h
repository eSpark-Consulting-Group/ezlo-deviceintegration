#pragma once

#include "mqtt_client/mqtt_message.h"

#include "mqtt/async_client.h"

#include <memory>
#include <string>
#include <chrono>

namespace HubPlatform {
namespace Common {
namespace MQTTPahoWrapper {

MQTTMessage convertMessage( mqtt::const_message_ptr msg );

std::unique_ptr< mqtt::async_client > createClient( std::string serverURI, std::string clientID );

void setClientCallback( mqtt::async_client& client, mqtt::callback& callback );
void setMessageCallback( mqtt::async_client& client, std::function< void( mqtt::const_message_ptr msg ) > callback );

void connect( mqtt::async_client& client,
              std::string username,
              std::string password,
              std::chrono::milliseconds timeout );

void disconnect( mqtt::async_client& client, std::chrono::milliseconds timeout );

void subscribe( mqtt::async_client& client, const std::string& topic, std::chrono::milliseconds timeout );

void unsubscribe( mqtt::async_client& client, const std::string& topic, std::chrono::milliseconds timeout );

void publish( mqtt::async_client& client, const MQTTMessage& message );

bool isConnected( mqtt::async_client& client );

}  // namespace MQTTPahoWrapper
}  // namespace Common
}  // namespace HubPlatform
