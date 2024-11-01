#include "cored/house_modes/protect_service_enum.h"
#include "cored/house_modes/resources/house_mode_resources.h"

namespace HubPlatform {
namespace CoreDHouseModes {

int toInt( ProtectServiceEnum value ) {
    return static_cast< int >( value );
}

std::string toString( ProtectServiceEnum value ) {
    std::string result;
    switch( value ) {
    case ProtectServiceEnum::POLICE:
        return Resources::ProtectService::Police;
    case ProtectServiceEnum::MEDICAL:
        return Resources::ProtectService::Medical;
    case ProtectServiceEnum::FIRE:
        return Resources::ProtectService::Fire;
    default:;
    }
    return std::string{};
}

ProtectServiceEnum toProtectService( const std::string& service ) {
    using namespace Resources;
    if( service == ProtectService::Police )
        return ProtectServiceEnum::POLICE;
    else if( service == ProtectService::Medical )
        return ProtectServiceEnum::MEDICAL;
    else if( service == ProtectService::Fire )
        return ProtectServiceEnum::FIRE;

    return ProtectServiceEnum::NONE;
}

bool badValue( ProtectServiceEnum value ) {
    return value == ProtectServiceEnum::NONE;
}

}  // namespace CoreDHouseModes
}  // namespace HubPlatform
