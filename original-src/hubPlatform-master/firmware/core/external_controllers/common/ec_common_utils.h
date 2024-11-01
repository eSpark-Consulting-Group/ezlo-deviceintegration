#pragma once

#include <string>

namespace HubPlatform {
namespace ExternalControllers {
namespace Utils {

inline std::string generateDbId( const std::string& controllerId, const std::string& id ) {
    return controllerId + "|" + id;
}

inline std::string extractControllerId( const std::string& dbId ) {
    std::string result;
    auto delimiter = dbId.find( '|' );
    if( delimiter != std::string::npos ) {
        result = dbId.substr( 0, delimiter );
    }
    return result;
}

inline std::string extractId( const std::string& dbId ) {
    std::string result;
    auto delimiter = dbId.find( '|' );
    if( delimiter != std::string::npos ) {
        result = dbId.substr( delimiter + 1 );
    }
    return result;
}

}  // namespace Utils
}  // namespace ExternalControllers
}  // namespace HubPlatform
