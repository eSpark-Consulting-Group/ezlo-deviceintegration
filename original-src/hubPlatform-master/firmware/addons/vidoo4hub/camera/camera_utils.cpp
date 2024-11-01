#include "vidoo4hub/camera/camera_utils.h"
#include "vidoo4hub/vidoo4hub_exception.h"
#include "vidoo4hub/resources/vidoo4hub_resources.h"

#include "database_client/dbqueryparser.h"
#include "common_utils/cpp14.h"

#include "cored/model/cored_device.h"
#include "cored/model/cored_item.h"
#include "cored/model/values/value_factory.h"
#include "cored/model/resources/cored_resources.h"

#include <set>

namespace HubPlatform {
namespace Vidoo4Hub {
namespace CameraUtils {

bool isKnownCameraSubcategory( const std::string& cameraSubcategory ) {
    using namespace Vidoo4Hub::Resources::CameraSubcategory;
    const static std::set< std::string > subcategories = { IndoorCam, OutdoorCam, DoorbellCam };
    return subcategories.count( cameraSubcategory );
}

Json cameraToJson( const VidooCamera& camera ) {
    using namespace Vidoo4Hub::Resources::DB;

    Json j;
    json_add_str( j, FieldNames::cameraId, camera.cameraId );
    json_add_str( j, FieldNames::cameraSubcategory, camera.cameraSubcategory );
    json_add_str( j, FieldNames::cameraName, camera.cameraName );
    json_add_str( j, FieldNames::cameraDeviceId, camera.cameraDeviceId );
    json_add_str( j, FieldNames::sensorDeviceId, camera.sensorDeviceId );
    json_add_str( j, FieldNames::securityThreatItemId, camera.securityThreatItemId );
    json_add_str( j, FieldNames::motionItemId, camera.motionItemId );
    json_add_str( j, FieldNames::hotzonesItemId, camera.hotzonesItemId );
    json_add_str( j, FieldNames::snapshotItemId, camera.snapshotItemId );
    json_add_str( j, FieldNames::makeRecordingItemId, camera.makeRecordingItemId );
    json_add_str( j, FieldNames::stopRecordingItemId, camera.stopRecordingItemId );
    json_add_str( j, FieldNames::rtspURL, camera.rtspURL );
    json_add_str( j, FieldNames::streamId, camera.streamId );
    return j;
}

VidooCamera cameraFromJson( const Json& j ) {
    using namespace Vidoo4Hub::Resources::DB;

    VidooCamera camera;
    camera.cameraId = json_get_str( j, FieldNames::cameraId );
    camera.cameraSubcategory = json_get_str( j, FieldNames::cameraSubcategory );
    camera.cameraName = json_get_str( j, FieldNames::cameraName );
    camera.cameraDeviceId = json_get_str( j, FieldNames::cameraDeviceId );
    camera.sensorDeviceId = json_get_str( j, FieldNames::sensorDeviceId );
    camera.securityThreatItemId = json_get_str( j, FieldNames::securityThreatItemId );
    camera.motionItemId = json_get_str( j, FieldNames::motionItemId );
    camera.hotzonesItemId = json_get_str( j, FieldNames::hotzonesItemId );
    camera.snapshotItemId = json_get_str( j, FieldNames::snapshotItemId );
    camera.makeRecordingItemId = json_get_str( j, FieldNames::makeRecordingItemId );
    camera.stopRecordingItemId = json_get_str( j, FieldNames::stopRecordingItemId );
    camera.rtspURL = json_get_str( j, FieldNames::rtspURL );
    camera.streamId = json_get_str( j, FieldNames::streamId );
    return camera;
}

void storeCameraToDB( IDbClient& dbClient, const VidooCamera& camera ) {
    using namespace Vidoo4Hub::Resources::DB;

    auto j = cameraToJson( camera );
    json_remove( j, FieldNames::cameraId );
    json_add_str( j, db::ID_FIELD_NAME, camera.cameraId );
    dbClient.update( TableNames::VidooCamerasDBName, JSON_EMPTY_OBJ, j, true, false );
}

void deleteCameraFromDB( IDbClient& dbClient, const std::string& cameraId ) {
    using namespace Vidoo4Hub::Resources::DB;
    dbClient.remove( TableNames::VidooCamerasDBName, JSON( db::ID_FIELD_NAME << cameraId ) );
}

std::unique_ptr< VidooCamera > readCameraFromDB( IDbClient& dbClient, const std::string& cameraId ) {
    using namespace Vidoo4Hub::Resources::DB;
    auto query = JSON( db::ID_FIELD_NAME << cameraId );
    auto result = dbClient.findOne( TableNames::VidooCamerasDBName, query, nullptr );
    if( result.empty() )
        return {};

    json_add_str( result, FieldNames::cameraId, cameraId );
    return std::make_unique< VidooCamera >( cameraFromJson( result ) );
}

std::map< std::string, VidooCamera > readAllCamerasFromDB( IDbClient& dbClient ) {
    using namespace Vidoo4Hub::Resources::DB;
    auto result = dbClient.findAll( TableNames::VidooCamerasDBName, JSON_EMPTY_OBJ, nullptr );
    if( result.empty() )
        return {};

    std::map< std::string, VidooCamera > cameras;
    for( auto& cameraJson : result ) {
        auto id = json_get_str( cameraJson, db::ID_FIELD_NAME );
        json_add_str( cameraJson, FieldNames::cameraId, id );
        cameras.emplace( id, cameraFromJson( cameraJson ) );
    }
    return cameras;
}

std::string createCameraDevice( CoreInterface& core,
                                const std::string& gatewayId,
                                const std::string& subcategory,
                                const std::string& cameraDeviceName ) {
    HubPlatform::CoreD::Model::Device device(
        gatewayId, cameraDeviceName, "vidoo_generic_cam", "vidoo_generic_cam", "camera", false );
    device.setSubcategory( subcategory );

    auto result = core.addDevice( device );
    if( !result.second )
        throw Vidoo4HubError( "Failed to add vidoo camera device" );

    const auto deviceId = std::move( result.first );
    core.setDeviceArmed( deviceId, true );
    return deviceId;
}

std::string createMotionSensorDevice( CoreInterface& core,
                                      const std::string& gatewayId,
                                      const std::string& cameraDeviceId,
                                      const std::string& cameraDeviceName ) {
    std::string sensorName = createMotionSensorName( cameraDeviceName );

    HubPlatform::CoreD::Model::Device motionSensor(
        gatewayId, sensorName, "vidoo_motion_sensor", "motion", "security_sensor", false );
    motionSensor.setSubcategory( "motion" );
    motionSensor.setParentDeviceId( cameraDeviceId );
    auto result = core.addDevice( motionSensor );
    std::string sensorId = std::move( result.first );
    if( !result.second )
        throw Vidoo4HubError( "Failed to add vidoo motion sensor" );
    return sensorId;
}

std::string createSecurityThreatItem( CoreInterface& core, const std::string& motionSensorId ) {
    using namespace HubPlatform::CoreD::Model;

    auto value = ValueFactory::createItemValue( ValueType::Enum::Bool, false );

    HubPlatform::CoreD::Model::Item item(
        motionSensorId, "security_threat", ValueType::Enum::Bool, true, false, std::move( value ) );

    auto result = core.addItem( item );
    auto itemId = std::move( result.first );
    if( !result.second )
        throw Vidoo4HubError( "Failed to add vidoo security threat item" );
    return itemId;
}

std::string createMotionItem( CoreInterface& core, const std::string& motionSensorId ) {
    using namespace HubPlatform::CoreD::Model;

    auto value = ValueFactory::createItemValue( ValueType::Enum::Bool, false );

    HubPlatform::CoreD::Model::Item item(
        motionSensorId, "motion", ValueType::Enum::Bool, true, false, std::move( value ) );

    auto result = core.addItem( item );
    auto itemId = std::move( result.first );
    if( !result.second )
        throw Vidoo4HubError( "Failed to add vidoo motion item" );
    return itemId;
}

std::string createHotzonesItem( CoreInterface& core, const std::string& cameraDeviceId ) {
    using namespace HubPlatform::CoreD::Model;

    auto valueType = ValueType::makeDictionaryType( ValueType::Enum::CameraHotzone );
    auto value = ValueFactory::createItemValue( valueType, getDefaultHotzoneDictionaryJson() );

    HubPlatform::CoreD::Model::Item item( cameraDeviceId, "hotzones", valueType, true, false, std::move( value ) );
    item.setShow( true );

    auto result = core.addItem( item );
    auto itemId = std::move( result.first );
    if( !result.second )
        throw Vidoo4HubError( "Failed to add vidoo hotzones item" );
    return itemId;
}

std::string createSnapshotItem( CoreInterface& core, const std::string& cameraDeviceId ) {
    using namespace HubPlatform::CoreD::Model;

    auto value = ValueFactory::createItemValue( ValueType::Enum::Int, 0 );

    HubPlatform::CoreD::Model::Item item(
        cameraDeviceId, "take_snapshot", ValueType::Enum::Int, false, true, std::move( value ) );
    item.setShow( true );

    auto result = core.addItem( item );
    auto itemId = std::move( result.first );
    if( !result.second )
        throw Vidoo4HubError( "Failed to add vidoo snapshot item" );
    return itemId;
}

std::string createMakeRecordingItem( CoreInterface& core, const std::string& cameraDeviceId ) {
    using namespace Vidoo4Hub::Resources;
    using namespace HubPlatform::CoreD::Model;

    auto value = ValueFactory::createItemValue( ValueType::Enum::Int, 0 );
    auto minValue = ValueFactory::createItemValue( ValueType::Enum::Int, DefaultValues::MinRecordingDuration );
    auto maxValue = ValueFactory::createItemValue( ValueType::Enum::Int, DefaultValues::MaxRecordingDuration );

    HubPlatform::CoreD::Model::Item item(
        cameraDeviceId, "make_recording", ValueType::Enum::Int, false, true, std::move( value ) );
    item.setLowerLimitValue( std::move( minValue ) );
    item.setUpperLimitValue( std::move( maxValue ) );
    item.setShow( true );

    auto result = core.addItem( item );
    auto itemId = std::move( result.first );
    if( !result.second )
        throw Vidoo4HubError( "Failed to add vidoo make recording item" );
    return itemId;
}

std::string createStopRecordingItem( CoreInterface& core, const std::string& cameraDeviceId ) {
    using namespace HubPlatform::CoreD::Model;

    auto value = ValueFactory::createItemValue( ValueType::Enum::Int, 0 );

    HubPlatform::CoreD::Model::Item item(
        cameraDeviceId, "stop_recording", ValueType::Enum::Int, false, true, std::move( value ) );
    item.setShow( true );

    auto result = core.addItem( item );
    auto itemId = std::move( result.first );
    if( !result.second )
        throw Vidoo4HubError( "Failed to add vidoo stop recording item" );
    return itemId;
}

std::string createStreamItem( CoreInterface& core, const std::string& cameraDeviceId, const std::string& streamName ) {
    using namespace HubPlatform::CoreD::Model;

    auto jsonURL = JSON( "cloudLiveStreamUrl"
                         << ""
                         << "localRtspUrl"
                         << "" );
    auto value = ValueFactory::createItemValue( ValueType::Enum::CameraStream, jsonURL );

    HubPlatform::CoreD::Model::Item item(
        cameraDeviceId, streamName, ValueType::Enum::CameraStream, true, false, std::move( value ) );

    auto result = core.addItem( item );
    auto itemId = std::move( result.first );
    if( !result.second )
        throw Vidoo4HubError( "Failed to add vidoo stream item" );
    return itemId;
}

void removeDevice( CoreInterface& core, const std::string& deviceId ) {
    core.removeDevice( deviceId );
}

void removeItem( CoreInterface& core, const std::string& itemId ) {
    core.removeItem( itemId );
}

void setMotionStarted( CoreInterface& core, const std::string& itemId ) {
    core.updateItemValue( itemId, []( HubPlatform::CoreD::Model::ValueType valueType ) {
        return HubPlatform::CoreD::Model::ValueFactory::createItemValue( valueType, Json( true ) );
    } );
}

void setMotionEnded( CoreInterface& core, const std::string& itemId ) {
    core.updateItemValue( itemId, []( HubPlatform::CoreD::Model::ValueType valueType ) {
        return HubPlatform::CoreD::Model::ValueFactory::createItemValue( valueType, Json( false ) );
    } );
}

void setStreamStarted( CoreInterface& core, const std::string& itemId, const std::string& rtspURL ) {
    namespace StreamFields = HubPlatform::CoreD::Model::Resources::CameraStreamValueFields;

    auto value = JSON( StreamFields::localRtspUrl << rtspURL << StreamFields::cloudLiveStreamUrl << "" );
    core.updateItemValue( itemId, [&value]( HubPlatform::CoreD::Model::ValueType valueType ) {
        return HubPlatform::CoreD::Model::ValueFactory::createItemValue( valueType, value );
    } );
}

void setStreamStopped( CoreInterface& core, const std::string& itemId ) {
    core.updateItemValue( itemId, []( HubPlatform::CoreD::Model::ValueType valueType ) {
        return HubPlatform::CoreD::Model::ValueFactory::createDefaultItemValue( valueType );
    } );
}

Json getDefaultHotzoneJson() {
    using namespace HubPlatform::CoreD::Model;
    namespace HotzoneFields = HubPlatform::CoreD::Model::Resources::CameraHotzoneValueFields;
    auto itemJson = ValueFactory::createDefaultItemValue( ValueType::Enum::CameraHotzone )->toJson();
    itemJson.at( HotzoneFields::Name ) = Resources::DefaultValues::defaultHotzoneName;
    return itemJson;
}

Json getDefaultHotzoneDictionaryJson() {
    Json dictionary;
    dictionary[ Resources::DefaultValues::defaultHotzoneId ] = getDefaultHotzoneJson();
    return dictionary;
}

void cleanCameraItemValues( CoreInterface& core, const VidooCamera& camera ) {
    namespace HotzoneFields = HubPlatform::CoreD::Model::Resources::CameraHotzoneValueFields;

    auto updateItemValue = [&]( const std::string& itemId, const Json& value ) {
        core.updateItemValue( itemId, [&]( HubPlatform::CoreD::Model::ValueType valueType ) {
            return HubPlatform::CoreD::Model::ValueFactory::createItemValue( valueType, value );
        } );
    };

    updateItemValue( camera.motionItemId, Json( false ) );
    updateItemValue( camera.securityThreatItemId, Json( false ) );
    updateItemValue( camera.snapshotItemId, Json( 0 ) );
    updateItemValue( camera.makeRecordingItemId, Json( 0 ) );
    updateItemValue( camera.stopRecordingItemId, Json( 0 ) );

    auto hotzonesItem = core.getItem( camera.hotzonesItemId );
    if( hotzonesItem.empty() )
        return;

    auto valueKey = HubPlatform::CoreD::Model::Resources::ItemFields::ValueFieldName;
    auto hotzones = JSON_EMPTY_OBJ;
    json_get_obj( hotzonesItem, valueKey, hotzones );

    if( hotzones.empty() )
        return;

    auto updatedHotzones = hotzones;
    for( auto& hotzone : updatedHotzones ) {
        hotzone.at( HotzoneFields::Detected ) = JSON_EMPTY_OBJ;
    }

    if( updatedHotzones != hotzones )
        updateItemValue( camera.hotzonesItemId, updatedHotzones );
}

std::string createMotionSensorName( const std::string& cameraDeviceName ) {
    return "sensorof[" + cameraDeviceName + "]";
}

}  // namespace CameraUtils
}  // namespace Vidoo4Hub
}  // namespace HubPlatform
