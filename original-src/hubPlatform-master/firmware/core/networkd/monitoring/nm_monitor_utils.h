#pragma once

#include "json_utils/json_fwd.h"

namespace HubPlatform {
namespace Network {

std::pair< uint32_t, uint32_t > parseIpCidr( const std::string& ipCidr );
std::string generateMaskString( int bits );

std::string getInterfaceMacAddress( const std::string& interfaceName );

Json getActualNetworkIfaceConfig( const std::string& name );

Json parseVirtualInterfaceStatus( const std::string& status );

std::string checkAndFixRoutes( const std::vector< Json >& interfaces );

Json getNetworkInterfaceStatus( const std::string& name );

}  // namespace Network
}  // namespace HubPlatform
