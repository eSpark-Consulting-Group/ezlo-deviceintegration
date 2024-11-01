#include "devices_list_builder.h"
#include "uid/response_builders/response_builders_factory.h"
#include "response_builder_v1.h"
#include "jsonrpc/jsonrpc.h"
#include "uid/response_builders/filter_data_list.h"
#include "cored/model/resources/cored_resources.h"

namespace HubPlatform {
namespace Uid {
namespace ResponseBuilder {

using DeviceAdditionalInfoArray = std::vector< HubPlatform::DeviceAdditionalInfo >;
using DeviceFirmwareInfoArray = std::vector< HubPlatform::FirmwareInfo >;
using ArmedState = Uid::ResponseBuilder::GetDevices::ArmedState;
using HouseModesOptions = Json;

std::tuple< DeviceAdditionalInfoArray, DeviceFirmwareInfoArray, ArmedState, HouseModesOptions >
prepareDeviceResponseFields( const JsonObject& device ) {
    DeviceAdditionalInfoArray additionalInfo;
    DeviceFirmwareInfoArray firmwareInfo;
    using namespace CoreD::Model::Resources;

    const JsonObject& joInfo = device[ DeviceFields::InfoFieldName ];
    if( joInfo.is_object() && !joInfo.empty() ) {
        additionalInfo.reserve( joInfo.size() );

        for( auto it = joInfo.begin(), end = joInfo.end(); it != end; ++it ) {
            const auto& joValue = it.value();

            std::string value = joValue.is_string() ? joValue.get< std::string >() : joValue.dump();
            additionalInfo.emplace_back( it.key(), std::move( value ) );
        }
    }

    const JsonObject firmwareJson = device[ DeviceFields::FirmwareFieldName ];
    if( firmwareJson.is_array() && !firmwareJson.empty() ) {
        firmwareInfo.reserve( firmwareJson.size() );

        for( const auto& firmwareInfoJson : firmwareJson ) {
            FirmwareInfo info{ json_get_str( firmwareInfoJson, DeviceFields::FirmwareIdFieldName ),
                               json_get_str( firmwareInfoJson, DeviceFields::FirmwareVersionFieldName ) };
            firmwareInfo.push_back( std::move( info ) );
        }
    }

    ArmedState armed = ArmedState::NoState;
    bool isArmed;
    if( json_get( device, DeviceFields::ArmedFieldName, isArmed ) ) {
        armed = isArmed ? ArmedState::Armed : ArmedState::Disarmed;
    }

    HouseModesOptions houseModesOptions;  // optional
    json_get_obj( device, DeviceFields::HouseModesOptionsFieldName, houseModesOptions );

    return std::make_tuple( std::move( additionalInfo ), std::move( firmwareInfo ), armed, houseModesOptions );
}

void DevicesListBuilder::buildDataListDevicesResponse( rpc::Message& rpcMessage,
                                                       const std::vector< JsonObject >& devices,
                                                       FilterDataListI* filter ) {
    using namespace CoreD::Model::Resources;
    auto responseBuilder = ResponseBuildersFactory::createDataListDevicesResponseBuilder( rpcMessage, filter );
    if( !responseBuilder )
        return;

    /* apply transformations specific to the Hub API version */
    for( const auto& d : devices ) {
        DeviceAdditionalInfoArray additionalInfo;
        DeviceFirmwareInfoArray firmwareInfo;
        ArmedState armed;
        HouseModesOptions houseModesOptions;

        std::tie( additionalInfo, firmwareInfo, armed, houseModesOptions ) = prepareDeviceResponseFields( d );

        responseBuilder->addDevice( json_get_str( d, "_id" ),
                                    json_get_str( d, "deviceTypeId" ),
                                    json_get_str( d, "gatewayId" ),
                                    json_get_str( d, "name" ),
                                    json_get_str( d, "type" ),
                                    json_get_str( d, "roomId" ),
                                    json_get_str( d, "category" ),
                                    json_get_str( d, "subcategory" ),
                                    json_get_str( d, "parentDeviceId" ),
                                    json_get_str( d, "security" ),
                                    json_get_str( d, "status" ),
                                    additionalInfo,
                                    firmwareInfo,
                                    json_get_bool( d, "batteryPowered" ),
                                    json_get_bool( d, "reachable" ),
                                    json_get_bool( d, "ready" ),
                                    armed,
                                    json_get_bool( d, "persistent" ),
                                    json_get_bool( d, "parentRoom", true ),
                                    json_get_str( d, DeviceFields::ProtectConfig ) );
    }

    JsonObject& joAnswer = rpcMessage.getAnswer();
    auto response = responseBuilder->buildResponse();
    joAnswer[ "result" ][ "devices" ] = std::move( response[ "result" ][ "devices" ] );
    joAnswer[ "api" ] = std::move( response[ "api" ] );
}

}  // namespace ResponseBuilder
}  // namespace Uid
}  // namespace HubPlatform
