#include "uid/common_methods.h"
#include "cored/remote_interface/cored_remote_interface.h"
#include "hardware_manager/remote_interface/hm_remote_interface.h"
#include "networkd/remote_interface/network_remote_interface.h"
#include "common_utils/firmware_info_helper.h"
#include "common_utils/hub_info_helper.h"
#include "base_utils/utils.h"
#include "base_utils/timeutils.h"
#include "base_utils/ezlodb.h"
#include "database_client/db_flags_names.h"
#include "version/ha_version.h"

#include "json_utils/json_utils.h"

#include <sys/utsname.h>
#include <fstream>

namespace HubPlatform {
namespace Uid {
namespace Common {

JsonObject getHubInfo( Services::Service& service ) {
    JsonObject result;
    {  // location
        CoreDRemoteInterface::CoreDRemoteInterface interface{ service };
        std::string timeZone = interface.getHubTimeZone();
        const auto geopositionPair = interface.getGeoPosition();
        const auto geoposition = geopositionPair.first;

        auto& location = result[ "location" ];
        json_add_float( location, "latitude", geoposition.latitude );
        json_add_float( location, "longitude", geoposition.longitude );
        location[ "timezone" ] = timeZone;
        location[ "state" ] = interface.getHubLocationState( geopositionPair );
    }

    {  // battery
        auto info = HardwareManager::RemoteInterface( service ).getBatteryInfo();
        if( info ) {
            result[ "battery" ] = info->toJson();
        }
    }

    result[ "serial" ] = HubInfoHelper::getSerial();
    result[ "model" ] = HubInfoHelper::getHubType();
    result[ "uuid" ] = HubInfoHelper::getControllerUUID();
    result[ "firmware" ] = FirmwareInfoHelper::getFirmwareVersionString();
    result[ "system" ] = FirmwareInfoHelper::getSystemVersionString();
    result[ "offlineAnonymousAccess" ] = Db::getFlag( OfflineAnonymousAccessFlagName );
    result[ "offlineInsecureAccess" ] = Db::getFlag( OfflineInsecureAccessFlagName );

    {
        auto revision = HubInfoHelper::getHardwareRevision();
        if( !revision.empty() )
            result[ "hardwareRevision" ] = std::move( revision );
    }

    {
        struct utsname unameData {};
        uname( &unameData );

        result[ "kernel" ] = unameData.release;
        result[ "architecture" ] = unameData.machine;
    }

    {
        std::uint64_t rest = 0;
        {
            std::ifstream stream( "/proc/uptime" );
            double seconds = 0.;
            stream >> seconds;
            rest = static_cast< std::uint64_t >( round( seconds ) );
        }

        auto getTime = [&rest]( std::uint64_t factor ) {
            auto result = rest % factor;
            rest /= factor;
            return result;
        };

        const auto seconds = getTime( 60u );
        const auto minutes = getTime( 60u );
        const auto hours = getTime( 24u );

        result[ "uptime" ] = utils::stringify( rest, "d ", hours, "h ", minutes, "m ", seconds, "s" );
    }

    {
        auto now = time( nullptr );
        struct tm timeInfo {};
        localtime_r( &now, &timeInfo );

        result[ "localtime" ] = TimeUtils::timeStructToString( timeInfo );
    }

    {
        Network::RemoteInterface interface{ service };
        auto response = interface.getSshTunnelSupportInfo();
        result[ "support" ] = std::move( response );
    }

    {
        using namespace HubPlatform::Version;

        auto& build = result[ "build" ];
        build[ "time" ] = HaBuildDate;
        build[ "builder" ] = HaBuilder;
        build[ "branch" ] = HaBranch;
        build[ "commit" ] = HaCommit;
    }
    return result;
}

}  // namespace Common
}  // namespace Uid
}  // namespace HubPlatform
