#pragma once
#include <string>

namespace HubPlatform {
namespace CoreDHouseModes {

enum class ProtectServiceEnum {
    NONE = 0,
    POLICE,
    MEDICAL,
    FIRE,
};

int toInt( ProtectServiceEnum value );
std::string toString( ProtectServiceEnum value );
ProtectServiceEnum toProtectService( const std::string& service );
bool badValue( ProtectServiceEnum );

}  // namespace CoreDHouseModes
}  // namespace HubPlatform