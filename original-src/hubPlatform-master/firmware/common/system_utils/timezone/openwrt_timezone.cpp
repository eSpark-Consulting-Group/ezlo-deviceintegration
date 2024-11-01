#include "system_utils/timezone/openwrt_timezone.h"

#include "system_utils/system.h"

namespace HubPlatform {
namespace system_utils {

void OpenWrtTimeZone::setTimeZone( const std::string& zoneName ) {
    bool error = false;
    if( zoneName == "UTC" ) {
        std::system( "uci delete system.@system[0].zonename" );
        error |= std::system( "uci set system.@system[0].timezone='UTC'" );
    }
    else {
        std::system( "uci delete system.@system[0].timezone" );
        error |= std::system( ( "uci set system.@system[0].zonename='" + zoneName + "'" ).c_str() );
    }

    if( error ) {
        std::system( "uci revert system.@system[0].timezone" );
        std::system( "uci revert system.@system[0].zonename" );
        throw TimeZoneError( "Failed to set timezone" );
    }

    std::system( "uci commit system" );
    std::system( "/etc/init.d/system reload" );
}

std::string OpenWrtTimeZone::getTimeZone() {
    const char* const timezoneCmd = "uci get system.@system[0].timezone";
    const char* const zonenameCmd = "uci get system.@system[0].zonename";

    auto removeNewline = []( const std::string& s ) { return s.substr( 0, s.find( '\n' ) ); };

    auto timezone = HubPlatform::System::getCommandOutput( timezoneCmd, true /*catchStderr*/ );
    if( timezone.second == 0 ) {
        if( !timezone.first.empty() )
            return removeNewline( timezone.first );
        throw TimeZoneError( "Failed to get timezone: empty value" );
    }

    auto zonename = HubPlatform::System::getCommandOutput( zonenameCmd, true /*catchStderr*/ );
    if( zonename.second == 0 ) {
        if( !zonename.first.empty() )
            return removeNewline( zonename.first );
        throw TimeZoneError( "Failed to get zonename: empty value" );
    }

    throw TimeZoneError( "Failed to get timezone" );
}

bool OpenWrtTimeZone::isTimeZoneDefault() {
    return getTimeZone() == "UTC";
}

}  // namespace system_utils
}  // namespace HubPlatform
