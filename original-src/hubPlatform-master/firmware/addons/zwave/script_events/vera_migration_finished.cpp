#include "zwave/script_events/vera_migration_finished.h"

#include "zwave/resources/zwave_resources.h"
#include "json_utils/json_utils.h"

namespace HubPlatform {
namespace Zwave {

constexpr char RootType[] = "urn:schemas-micasaverde-com:device:ZWaveNetwork:1";
constexpr char RootInvisible[] = "1";
constexpr int Rootcategory = 19;

VeraMigrationFinished::VeraMigrationFinished( Json&& veraDevicesModel )
: Services::ScriptEvent( Resources::ScriptEvent::Name::VeraMigrationFinished )
, m_veraDevicesModel( std::move( veraDevicesModel ) ) {}

Json VeraMigrationFinished::toJson() const {
    using namespace Resources::ScriptEvent;

    auto result = Services::ScriptEvent::toJson();

    result[ Param::VeraDeviceModel ] = simplyfiDeviceModel();

    return result;
}

Json VeraMigrationFinished::simplyfiDeviceModel() const {
    using namespace Resources::ScriptEvent::VeraDeviceModel;

    JsonArray result;

    JsonArray devices;
    json_get_arr( m_veraDevicesModel, Devices, devices );

    if( auto rootId = findZwaveRootDevice( devices ) ) {
        for( const auto& device : devices ) {
            auto parentId = json_get_int64( device, ParentId, -1 );
            if( parentId == *rootId ) {
                auto altId = json_get_str( device, AltId );
                auto id = json_get_int64( device, Id, -1 );
                result.push_back( JSON( AltId << altId << Id << id ) );
            }
        }
    }

    return JSON( Devices << result );
}

common::optional< uint32_t > VeraMigrationFinished::findZwaveRootDevice( const JsonArray& devices ) {
    using namespace Resources::ScriptEvent::VeraDeviceModel;

    auto it = std::find_if( devices.begin(), devices.end(), []( const Json& deviceInfo ) {
        auto type = json_get_str( deviceInfo, DeviceType );
        auto invisible = json_get_str( deviceInfo, Invisible );
        auto category = json_get_int( deviceInfo, Category, -1 );
        if( type == RootType && invisible == RootInvisible && category == Rootcategory ) {
            return true;
        }
        return false;
    } );

    if( it != devices.end() ) {
        auto id = json_get_int64( *it, Id, -1 );
        if( id != -1 ) {
            return id;
        }
    }

    return {};
}

}  // namespace Zwave
}  // namespace HubPlatform
