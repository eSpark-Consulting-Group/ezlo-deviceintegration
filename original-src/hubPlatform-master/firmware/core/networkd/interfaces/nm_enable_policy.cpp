#include "networkd/interfaces/nm_enable_policy.h"

namespace HubPlatform {
namespace Network {

EnablePolicy enablePolicyFromString( const std::string& policy, EnablePolicy defaultPolicy ) {
    if( policy == "auto" )
        return EnablePolicy::Auto;
    if( policy == "on" )
        return EnablePolicy::On;
    if( policy == "off" )
        return EnablePolicy::Off;

    return defaultPolicy;
}

std::string toString( EnablePolicy policy ) {
    switch( policy ) {
    case EnablePolicy::Auto:
        return "auto";
    case EnablePolicy::On:
        return "on";
    case EnablePolicy::Off:
        return "off";
    default:
        return "auto";
    }
}

}  // namespace Network
}  // namespace HubPlatform
