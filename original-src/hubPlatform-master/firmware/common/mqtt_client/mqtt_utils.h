#pragma once

#include "mqtt_client/mqtt_message.h"

namespace HubPlatform {
namespace Common {
namespace MQTTUtils {
std::vector< uint8_t > strToBytes( const std::string& str );
std::string bytesToStr( const std::vector< uint8_t >& bytes );

MQTTMessage createMessage( const std::string& topic, const std::string& payload );

std::string getStrPayload( const MQTTMessage& msg );

}  // namespace MQTTUtils
}  // namespace Common
}  // namespace HubPlatform
