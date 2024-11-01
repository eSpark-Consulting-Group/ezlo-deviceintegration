#pragma once

#include <string>

namespace HubPlatform {
namespace Network {

enum class EnablePolicy { Auto, On, Off };

EnablePolicy enablePolicyFromString( const std::string& policy, EnablePolicy defaultPolicy );
std::string toString( EnablePolicy policy );

}  // namespace Network
}  // namespace HubPlatform
