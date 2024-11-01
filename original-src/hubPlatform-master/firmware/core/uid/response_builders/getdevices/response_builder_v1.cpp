#include "response_builder_v1.h"

#include "jsonrpc/jsonrpc.h"
#include "uid/response_builders/filter_data_list.h"
#include "uid/response_builders/set_field_helper.h"

namespace HubPlatform {
namespace Uid {
namespace ResponseBuilder {

namespace GetDevices {
namespace V1 {

ResponseBuilder::ResponseBuilder( const rpc::Message& message, const std::string& api, FilterDataListI* filter )
: ResponseBuilderBase( message, api )
, m_filter( filter ) {}

void ResponseBuilder::addDevice( const std::string& id,
                                 const std::string& deviceTypeId,
                                 const std::string& gatewayId,
                                 const std::string& name,
                                 const std::string& type,
                                 const std::string& roomId,
                                 const std::string& category,
                                 const std::string& subcategory,
                                 const std::string& parentDeviceId,
                                 const std::string& security,
                                 const std::string& status,
                                 const std::vector< DeviceAdditionalInfo >& additionalInfo,
                                 const std::vector< FirmwareInfo >& firmware,
                                 bool batteryPowered,
                                 bool reachable,
                                 bool ready,
                                 ArmedState armed,
                                 bool persistent,
                                 bool parentRoom,
                                 const std::string& protectConfig ) {
    JsonObject device = JSON_EMPTY_OBJ;
    device[ "_id" ] = id;  // set _id ALWAYS!

    SetFieldHelper helper( device, m_filter );
    helper.set( "batteryPowered", batteryPowered );
    helper.set( "deviceTypeId", deviceTypeId );
    helper.set( "gatewayId", gatewayId );
    helper.set( "name", name );
    helper.set( "reachable", reachable );
    helper.set( "type", type );
    helper.set( "roomId", roomId );
    helper.set( "category", category );
    helper.set( "subcategory", subcategory );
    helper.set( "parentDeviceId", parentDeviceId );
    helper.set( "security", security );
    helper.set( "status", status );
    helper.set( "ready", ready );
    helper.set( "persistent", persistent );
    if( !parentDeviceId.empty() ) {
        helper.set( "parentRoom", parentRoom );
    }
    helper.set( "protectConfig", protectConfig );

    JsonObject joInfo = JSON_EMPTY_OBJ;
    if( !additionalInfo.empty() ) {
        for( const auto& info : additionalInfo ) {
            joInfo[ info.m_label ] = info.m_value;
        }
    }
    helper.set( "info", std::move( joInfo ) );

    if( !firmware.empty() ) {
        auto firmwareJson = Json::array();
        for( const auto& firmwareInfo : firmware ) {
            firmwareJson.push_back( JSON( "id" << firmwareInfo.id << "version" << firmwareInfo.version ) );
        }
        helper.set( "firmware", std::move( firmwareJson ) );
    }

    if( armed != ArmedState::NoState ) {
        helper.set( "armed", ( armed == ArmedState::Armed ) );
    }

    m_devices.emplace_back( std::move( device ) );
}

JsonObject ResponseBuilder::buildResponse() const {
    JsonObject joResponse = ResponseBuilderBase::buildResponse();
    joResponse[ "result" ][ "devices" ] = m_devices;
    return joResponse;
}

}  // namespace V1
}  // namespace GetDevices

namespace DataListDevices {
namespace V1 {

ResponseBuilder::ResponseBuilder( const rpc::Message& message, const std::string& api, FilterDataListI* filter )
: GetDevices::V1::ResponseBuilder( message, api, filter ) {}

void ResponseBuilder::addDevice( const std::string& id,
                                 const std::string& deviceTypeId,
                                 const std::string& gatewayId,
                                 const std::string& name,
                                 const std::string& type,
                                 const std::string& roomId,
                                 const std::string& category,
                                 const std::string& subcategory,
                                 const std::string& parentDeviceId,
                                 const std::string& security,
                                 const std::string& status,
                                 const std::vector< DeviceAdditionalInfo >& additionalInfo,
                                 const std::vector< FirmwareInfo >& firmware,
                                 bool batteryPowered,
                                 bool reachable,
                                 bool ready,
                                 GetDevices::ArmedState armed,
                                 bool persistent,
                                 bool parentRoom,
                                 const std::string& protectConfig ) {
    GetDevices::V1::ResponseBuilder::addDevice( id,
                                                deviceTypeId,
                                                gatewayId,
                                                name,
                                                type,
                                                roomId,
                                                category,
                                                subcategory,
                                                parentDeviceId,
                                                security,
                                                status,
                                                additionalInfo,
                                                firmware,
                                                batteryPowered,
                                                reachable,
                                                ready,
                                                armed,
                                                persistent,
                                                parentRoom,
                                                protectConfig );
}

JsonObject ResponseBuilder::buildResponse() const {
    JsonObject joResponse = ResponseBuilderBase::buildResponse();
    /**result.devices: [ ITEMS ]
     * ITEMS: {
     *      itemId: { FILTERED_FIELDS }
     * }
     * */
    JsonObject result;

    for( const auto& device : m_devices ) {
        std::string deviceId = json_get_str( device, "_id" );
        if( !m_filter || m_filter->hasId( deviceId ) ) {
            result[ deviceId ] = makeFieldsDict( device, m_filter );
        }
    }

    fillResultWithMissedIds( result );

    joResponse[ "result" ][ "devices" ] = result;
    return joResponse;
}

void ResponseBuilder::fillResultWithMissedIds( JsonObject& result ) const {
    fillResultMissedIds( result, m_filter, m_devices, JsonObject( nullptr ) );
}

}  // namespace V1
}  // namespace DataListDevices

}  // namespace ResponseBuilder
}  // namespace Uid
}  // namespace HubPlatform
