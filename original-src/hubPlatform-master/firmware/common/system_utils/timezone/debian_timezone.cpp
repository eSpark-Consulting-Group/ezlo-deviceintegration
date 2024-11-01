#include "system_utils/timezone/debian_timezone.h"

#include "system_utils/filesystem.h"
#include "system_utils/system.h"

#include <fstream>
#include <iostream>

namespace HubPlatform {
namespace system_utils {

const char* const TimeZonesFolderPath = "/usr/share/zoneinfo/";
const char* const DebianLocalTimeLink = "/etc/localtime";
const char* const DebianTimeZoneConfig = "/etc/timezone";

void DebianTimeZone::setTimeZone( const std::string& zoneName ) {
    const std::string localtimeLink( DebianLocalTimeLink );
    const std::string timezoneConfig( DebianTimeZoneConfig );

    if( !filesystem::isLinkExist( localtimeLink ) ) {
        if( filesystem::isExist( localtimeLink ) )
            throw TimeZoneError( "Failed to set timezone: localtime is not a symlink" );
    }

    if( !filesystem::isFileExist( timezoneConfig ) ) {
        if( filesystem::isExist( timezoneConfig ) )
            throw TimeZoneError( "Failed to set timezone: timezone config is not a file" );
    }

    auto timezonePath = std::string( TimeZonesFolderPath ) + zoneName;
    if( !filesystem::isExist( timezonePath ) )
        throw TimeZoneError( "Failed to set timezone: file does not exist: " + timezonePath );

    std::string localtimeCmd = "ln -snf " + timezonePath + " " + localtimeLink;
    auto localtimeRes = HubPlatform::System::getCommandOutput( localtimeCmd.c_str(), true /*catchStderr*/ );
    if( localtimeRes.second != 0 )
        throw TimeZoneError( "Failed to create localtime link :" + localtimeRes.first );

    std::ofstream stream( timezoneConfig );
    if( !stream.is_open() ) {
        throw TimeZoneError( "Failed to write to timezone config" );
    }
    stream << zoneName << std::endl;
}

std::string DebianTimeZone::getTimeZone() {
    const std::string timezoneFolder( TimeZonesFolderPath );
    const std::string localtimeLink( DebianLocalTimeLink );
    const std::string timezoneConfig( DebianTimeZoneConfig );

    std::string zoneName;

    std::ifstream stream( timezoneConfig );
    if( stream.is_open() ) {
        stream >> zoneName;
        if( !zoneName.empty() )
            return zoneName;
    }

    if( !filesystem::isLinkExist( localtimeLink ) )
        throw TimeZoneError( "Failed to get timezone: localtime does not exist: " + localtimeLink );
    std::string timeZonePath = filesystem::getSymlinkValue( localtimeLink );

    if( timeZonePath.find( timezoneFolder ) != 0 )
        throw TimeZoneError( "Failed to get timezone: unknown timezone" );
    zoneName = timeZonePath.substr( timezoneFolder.size() );

    return zoneName;
}

bool DebianTimeZone::isTimeZoneDefault() {
    return getTimeZone() == "UTC";
}

}  // namespace system_utils
}  // namespace HubPlatform
