#pragma once

#include "service_interfaces/script_event.h"
#include "cloud_devices/resources/cloud_devices_resources.h"

#include <unordered_map>
#include <string>

namespace HubPlatform {
namespace CloudDevices {

inline void convertDeviceFromDbToModelFormat( const Json& dbDevice, Json& modelDevice ) {
    static std::unordered_map< std::string, std::string > mapping = {
        { Resources::DB::Fields::Id, Resources::Model::Device::Id },
    };

    for( auto it = dbDevice.begin(); it != dbDevice.end(); ++it ) {
        auto keyIt = mapping.find( it.key() );
        if( keyIt != mapping.end() ) {
            modelDevice[ keyIt->second ] = it.value();
        }
        else {
            modelDevice[ it.key() ] = it.value();
        }
    }
}

inline Json convertDeviceFromDbToModelFormat( const Json& dbDevice ) {
    Json modelDevice;
    convertDeviceFromDbToModelFormat( dbDevice, modelDevice );
    return modelDevice;
}

}  // namespace CloudDevices
}  // namespace HubPlatform
