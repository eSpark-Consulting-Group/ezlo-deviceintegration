#include "swinger_shutdown_settings.h"
#include "database_client/dbid.h"
#include "database_client/dbqueryparser.h"
#include "database_client/idbclient.h"
#include "resources.h"
#include <algorithm>

namespace HubPlatform {
namespace CoreD {
namespace SwingerShutdown {

using namespace Resources;

SwingerShutdownSettings::SwingerShutdownSettings( const Json& record ) {
    fromJson( record );
}

int SwingerShutdownSettings::getDefaultHitsLimit() {
    return m_hitsLimitDefault;
}

int64_t SwingerShutdownSettings::getDefaultInactiveWindow() {
    return m_inactiveWindowDefault;
}

std::list< std::string > SwingerShutdownSettings::getDisabledDevices() {
    return m_disabledDevices;
}

std::set< std::string > SwingerShutdownSettings::getDevicesIds() {
    std::set< std::string > ids;
    for( auto const& element : m_devices ) {
        ids.insert( element.first );
    }

    return ids;
}

Json SwingerShutdownSettings::toJson() const {
    using namespace Resources::ConfigFields;
    using namespace Resources::Settings;
    JsonObject result = JSON_EMPTY_OBJ;

    JsonObject defaults;
    defaults[ Settings::HitsLimit ] = m_hitsLimitDefault;
    defaults[ Settings::InactivityWindow ] = m_inactiveWindowDefault;
    result[ Defaults ] = defaults;
    result[ Version ] = m_version;

    JsonArrayBuilder devicesArr;
    for( auto device : m_devices ) {
        auto hitsLimit = json_get_int( device.second, Settings::HitsLimit, 0 );
        auto inactivityWindow = json_get_int( device.second, Settings::InactivityWindow, 0 );

        auto limits = JSON( "deviceId" << device.first << Settings::HitsLimit << hitsLimit << Settings::InactivityWindow
                                       << inactivityWindow );
        devicesArr << limits;
    }

    result[ Devices ] = devicesArr;

    JsonArrayBuilder disabledDevicesArray;
    for( auto device : m_disabledDevices ) {
        disabledDevicesArray << device;
    }

    result[ DisabledDevices ] = disabledDevicesArray;

    return result;
}

void SwingerShutdownSettings::addDisabledDevice( const std::string& deviceId ) {
    m_disabledDevices.push_back( deviceId );
}

void SwingerShutdownSettings::removeDisabledDevice( const std::string& deviceId ) {
    m_disabledDevices.remove_if(
        [deviceId]( const std::string& toRemoveDeviceId ) { return ( deviceId == toRemoveDeviceId ); } );
}

bool SwingerShutdownSettings::isDisabledDevice( const std::string& deviceId ) {
    return ( std::find( m_disabledDevices.begin(), m_disabledDevices.end(), deviceId ) != m_disabledDevices.end() );
}

JsonObject SwingerShutdownSettings::getDeviceLimits( const std::string& deviceId ) {
    if( m_devices.find( deviceId ) != m_devices.end() ) {
        return m_devices[ deviceId ];
    }
    else {
        return JSON_EMPTY_OBJ;
    }
}

void SwingerShutdownSettings::setDeviceLimits( const std::string& deviceId, int hitsLimit, int64_t inactiveWindow ) {
    m_devices[ deviceId ] =
        JSON( ConfigFields::HitsLimit << hitsLimit << ConfigFields::InactivityWindow << inactiveWindow );
}

void SwingerShutdownSettings::resetDeviceLimits( const std::string& deviceId ) {
    m_devices.erase( deviceId );
}

bool SwingerShutdownSettings::fromJson( const JsonObject& data ) {
    using namespace Resources::ConfigFields;
    using namespace Resources::Settings;
    try {
        auto param = data.find( Defaults );
        if( param != data.end() && param->is_object() ) {
            JsonObject defaults;
            json_get_obj( data, Defaults, defaults );

            if( !defaults.empty() ) {
                param = defaults.find( Settings::HitsLimit );
                if( param != defaults.end() && param->is_number() ) {
                    m_hitsLimitDefault = json_get_int( defaults, Settings::HitsLimit );
                }

                param = defaults.find( Settings::InactivityWindow );
                if( param != defaults.end() && param->is_number() ) {
                    m_inactiveWindowDefault = json_get_int( defaults, Settings::InactivityWindow );
                }
            }
        }

        JsonObject version;
        param = data.find( Version );
        if( param != data.end() && param->is_string() ) {
            m_version = json_get_str( data, Version );
        }

        param = data.find( Devices );
        if( param != data.end() && param->is_array() ) {
            if( param != data.end() && param->is_array() ) {
                JsonArray devicesArr;
                json_get_arr( data, Devices, devicesArr );
                for( JsonObject device : devicesArr ) {
                    auto deviceId = json_get_str( device, "deviceId" );
                    device.erase( "deviceId" );
                    m_devices[ deviceId ] = device;
                }
            }
        }

        param = data.find( DisabledDevices );
        if( param != data.end() && param->is_array() ) {
            if( param != data.end() && param->is_array() ) {
                JsonArray disabledDevicesArr;
                json_get_arr( data, DisabledDevices, disabledDevicesArr );
                for( JsonObject& device : disabledDevicesArr ) {
                    m_disabledDevices.push_back( device );
                }
            }
        }

        return true;
    }
    catch( const std::exception& ex ) {
        LOGGER_ERROR( cc::error( "Failed to extract device statistic: " ) + cc::note( ex.what() ) );
    }

    return false;
}

void SwingerShutdownSettings::createDbRecord( JsonObject query, const std::string dbName, IDbClient& dbClient ) {
    using namespace Resources::ConfigFields;
    if( query.empty() ) {
        dbClient.insert( dbName, JSON( Defaults << JSON( HitsLimit << 0 << InactivityWindow << 0 ) ), true );
    }
    dbClient.insert( dbName, query, true );
}

void SwingerShutdownSettings::updateDbRecord( JsonObject updateObj, const std::string& dbName, IDbClient& dbClient ) {
    dbClient.update( dbName, JSON_EMPTY_OBJ, updateObj, false, true );
}

Json SwingerShutdownSettings::loadFromDb( const std::string& dbName, IDbClient& dbClient ) {
    auto records = dbClient.findOne( dbName, JSON_EMPTY_OBJ, nullptr );
    return records;
}

}  // namespace SwingerShutdown
}  // namespace CoreD
}  // namespace HubPlatform
