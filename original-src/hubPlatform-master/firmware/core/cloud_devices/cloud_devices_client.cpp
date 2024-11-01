#include "cloud_devices/cloud_devices_client.h"
#include "cloud_devices/cloud_devices_service.h"
#include "cloud_devices/resources/cloud_devices_resources.h"
#include "cloud_devices/common/device_converters.h"

#include "uid/remote_interface/ui_remote_interface.h"

#include "json_utils/json_utils.h"

namespace HubPlatform {
namespace CloudDevices {

CloudDevicesClient::CloudDevicesClient( Addons::Addon& hostAddon )
: ScriptClient( CloudDevicesService::Name, hostAddon ) {}

Json CloudDevicesClient::getAllDevicesIds() const {
    using namespace Resources::DB;
    auto devices = JSON_EMPTY_ARR;
    auto fieldsToReturn = JSON( Fields::Id << 1 );
    auto allDevices = getDBClient().findAll( TableNames::Devices, JSON_EMPTY_OBJ, &fieldsToReturn );
    for( auto& deviceJson : allDevices ) {
        auto id = json_get_str( deviceJson, Fields::Id );
        if( !id.empty() ) {
            devices.push_back( std::move( id ) );
        }
    }
    return devices;
}

Json CloudDevicesClient::getDevice( const std::string& deviceId ) const {
    using namespace Resources::DB;
    auto device = convertDeviceFromDbToModelFormat(
        getDBClient().findOne( TableNames::Devices, JSON( Fields::Id << deviceId ), nullptr ) );
    if( !device.empty() && !Uid::RemoteInterface( getHost() ).isRegisteredOnCloud() ) {
        device[ Resources::Model::Device::Reachable ] = 0;
    }
    return device;
}

std::string CloudDevicesClient::sendCommand( const std::string& commandId,
                                             const std::string& deviceId,
                                             const std::string& capability,
                                             const std::string& command,
                                             const Json& params ) {
    auto device = convertDeviceFromDbToModelFormat( getDevice( deviceId ) );
    if( device.empty() ) {
        Log::error( cc::error( "Trying to send command " ) + cc::note( command ) + cc::error( " to unknown device: " ) +
                    cc::note( deviceId ) );
        return "Trying to send command to unknown device";
    }

    const JsonArray& capabilities = device[ Resources::Model::Device::Capabilities ];
    if( std::find( capabilities.begin(), capabilities.end(), capability ) == capabilities.end() ) {
        Log::error( cc::error( "Trying to send command to unknown capability: " ) + cc::note( capability ) );
        return "Trying to send command to unknown capability";
    }

    auto rpcParams = JSON_EMPTY_OBJ;
    rpcParams[ Resources::CloudRPCParams::CommandId ] = commandId;
    rpcParams[ Resources::CloudRPCParams::DeviceId ] = deviceId;
    rpcParams[ Resources::CloudRPCParams::Capability ] = capability;
    rpcParams[ Resources::CloudRPCParams::Command ] = command;
    rpcParams[ Resources::CloudRPCParams::Params ] = params;

    auto answer = sendMethodCall( MessageSubClasses::CLOUD_DEVICES_SEND_COMMAND, std::move( rpcParams ) );
    if( !answer[ "error" ].empty() ) {
        return json_get_str( answer[ "error" ], "message" );
    }

    return {};
}

std::string CloudDevicesClient::sendCommand( const std::string& command, const Json& params ) {
    auto rpcParams = JSON_EMPTY_OBJ;
    rpcParams[ Resources::CloudRPCParams::Command ] = command;
    rpcParams[ Resources::CloudRPCParams::Params ] = params;

    auto answer = sendMethodCall( MessageSubClasses::CLOUD_DEVICES_GATEWAY_COMMAND, std::move( rpcParams ) );
    if( !answer[ "error" ].empty() ) {
        return json_get_str( answer[ "error" ], "message" );
    }

    return {};
}

const IDbClient& CloudDevicesClient::getDBClient() const {
    return getHost().getDBClient();
}

}  // namespace CloudDevices
}  // namespace HubPlatform
