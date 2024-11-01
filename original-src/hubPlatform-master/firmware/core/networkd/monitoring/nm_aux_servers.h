#pragma once

#include "json_utils/json_wrapper.h"
#include <chrono>

namespace HubPlatform {
namespace Network {

void setAdditionalServers( std::vector< std::string > servers );
std::vector< std::string > getAdditionalServers();
void setPingTimeout( std::chrono::seconds timeout );
std::chrono::seconds getPingTimeout();

Json& namedAuxServers();
void registerAdditionalServers( const std::string& role, const std::string& server, const std::string& fullUrl );

}  // namespace Network
}  // namespace HubPlatform