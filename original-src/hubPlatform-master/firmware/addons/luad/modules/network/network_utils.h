#pragma once

#include "luad/modules/network/network_address.h"

namespace HubPlatform {
namespace LuaNetwork {

// TODO: does not work on H2 hub, possible cause is libc
std::string getHostName( const IAddress& address );
std::string getMacAddress( const IAddress& address, const std::string& networkInterface );

}  // namespace LuaNetwork
}  // namespace HubPlatform
