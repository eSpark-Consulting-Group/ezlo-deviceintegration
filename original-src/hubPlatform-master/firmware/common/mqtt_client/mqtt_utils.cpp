#include "mqtt_client/mqtt_utils.h"

namespace HubPlatform {
namespace Common {
namespace MQTTUtils {

std::vector< uint8_t > strToBytes( const std::string& str ) {
    if( str.empty() )
        return { '\0' };
    return std::vector< uint8_t >( str.c_str(), str.c_str() + str.size() + 1 );
}

std::string bytesToStr( const std::vector< uint8_t >& bytes ) {
    if( bytes.empty() )
        return {};
    if( bytes.back() == '\0' )
        return std::string( (const char*) ( bytes.data() ) );
    return std::string( bytes.cbegin(), bytes.cend() );
}

MQTTMessage createMessage( const std::string& topic, const std::string& payload ) {
    return MQTTMessage{ topic, strToBytes( payload ) };
}

std::string getStrPayload( const MQTTMessage& msg ) {
    return bytesToStr( msg.payload );
}

}  // namespace MQTTUtils
}  // namespace Common
}  // namespace HubPlatform
