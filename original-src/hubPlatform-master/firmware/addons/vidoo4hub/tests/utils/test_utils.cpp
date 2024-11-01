#include "vidoo4hub/tests/utils/test_utils.h"
#include "vidoo4hub/resources/vidoo4hub_resources.h"
#include "cored/model/resources/cored_resources.h"
#include "json_utils/json_utils.h"

#include <atomic>
#include <unistd.h>

namespace HubPlatform {
namespace UT {

Vidoo4Hub::VidooCamera getDummyCamera( const std::string& cameraId ) {
    Vidoo4Hub::VidooCamera camera;
    camera.cameraId = cameraId;
    camera.cameraSubcategory = Vidoo4Hub::Resources::CameraSubcategory::DoorbellCam;
    camera.cameraName = "cameraName_" + cameraId;
    camera.cameraDeviceId = "cameraDeviceId_" + cameraId;
    camera.sensorDeviceId = "sensorDeviceId_" + cameraId;
    camera.securityThreatItemId = "securityThreatItemId_" + cameraId;
    camera.motionItemId = "motionItemId_" + cameraId;
    camera.hotzonesItemId = "hotzonesItemId" + cameraId;
    camera.snapshotItemId = "snapshotItemId_" + cameraId;
    camera.makeRecordingItemId = "startRecordingItemId_" + cameraId;
    camera.stopRecordingItemId = "stopRecordingItemId_" + cameraId;
    camera.rtspURL = "rtspURL_" + cameraId;
    camera.streamId = "streamId_" + cameraId;
    return camera;
}

std::string getUniqueID() {
    static std::atomic_int pid( getpid() );
    static std::atomic_uint id{ 0 };
    return "id_" + std::to_string( pid ) + "_" + std::to_string( ++id );
}

DetectableObject::DetectableObject( const std::string& objectId ) {
    id = "objectId_" + objectId;
    name = "objectName_" + objectId;
}

DetectableObject::DetectableObject( std::string objectId, std::string objectName )
: id( std::move( objectId ) )
, name( std::move( objectName ) ) {}

DetectablePerson::DetectablePerson( const std::string& personId ) {
    id = "personId_" + personId;
    name = "personName_" + personId;
}

DetectablePerson::DetectablePerson( std::string personId, std::string personName )
: id( std::move( personId ) )
, name( std::move( personName ) ) {}

DetectableQRCode::DetectableQRCode( const std::string& qrCodeId ) {
    id = "qrCodeId_" + qrCodeId;
    name = "qrCodeName_" + qrCodeId;
    value = "qrCodeValue" + qrCodeId;
}

DetectableQRCode::DetectableQRCode( std::string qrCodeId, std::string qrCodeName, std::string qrCodeValue )
: id( std::move( qrCodeId ) )
, name( std::move( qrCodeName ) )
, value( std::move( qrCodeValue ) ) {}

Json MessageBuilder::object( std::string objectId, std::string objectName ) {
    using namespace Vidoo4Hub::Resources;
    return JSON( MessageFields::objectId << std::move( objectId ) << MessageFields::objectName
                                         << std::move( objectName ) );
}

Json MessageBuilder::person( std::string personId, std::string personName ) {
    using namespace Vidoo4Hub::Resources;
    return JSON( MessageFields::personId << std::move( personId ) << MessageFields::personName
                                         << std::move( personName ) );
}

Json MessageBuilder::qrCode( std::string qrCodeId, std::string qrCodeName, std::string qrCodeValue ) {
    using namespace Vidoo4Hub::Resources;
    return JSON( MessageFields::qrCodeId << std::move( qrCodeId ) << MessageFields::qrCodeName
                                         << std::move( qrCodeName ) << MessageFields::qrCodeValue
                                         << std::move( qrCodeValue ) );
}

Json MessageBuilder::object( const DetectableObject& object ) {
    return MessageBuilder::object( object.id, object.name );
}

Json MessageBuilder::person( const DetectablePerson& person ) {
    return MessageBuilder::person( person.id, person.name );
}

Json MessageBuilder::qrCode( const DetectableQRCode& qrCode ) {
    return MessageBuilder::qrCode( qrCode.id, qrCode.name, qrCode.value );
}

Json MessageBuilder::hotzone( const std::string& cameraId,
                              const std::string& hotzoneId,
                              const std::string hotzoneName ) {
    using namespace Vidoo4Hub::Resources;
    return JSON( MessageFields::cameraId << cameraId << MessageFields::hotzoneId << hotzoneId
                                         << MessageFields::hotzoneName << hotzoneName );
}

std::vector< Json > MessageBuilder::objectList( const std::vector< DetectableObject >& objects ) {
    std::vector< Json > result;
    result.reserve( objects.size() );

    for( auto& object : objects )
        result.emplace_back( MessageBuilder::object( object ) );
    return result;
}

std::vector< Json > MessageBuilder::personList( const std::vector< DetectablePerson >& persons ) {
    std::vector< Json > result;
    result.reserve( persons.size() );

    for( auto& person : persons )
        result.emplace_back( MessageBuilder::person( person ) );
    return result;
}

std::vector< Json > MessageBuilder::qrCodeList( const std::vector< DetectableQRCode >& qrCodes ) {
    std::vector< Json > result;
    result.reserve( qrCodes.size() );

    for( auto& qrCode : qrCodes )
        result.emplace_back( MessageBuilder::qrCode( qrCode ) );
    return result;
}

Json DetectableBuilder::dictionary( std::string objectId, std::string objectName ) {
    namespace ObjectFields = HubPlatform::CoreD::Model::Resources::DetectableObjectBaseFields;
    return JSON( objectId << JSON( ObjectFields::Name << objectName ) );
}

Json DetectableBuilder::dictionary( std::string objectId, std::string objectName, std::string objectValue ) {
    namespace ObjectFields = HubPlatform::CoreD::Model::Resources::DetectableObjectBaseFields;
    return JSON( objectId << JSON( ObjectFields::Name << objectName << ObjectFields::Value << objectValue ) );
}

Json DetectableBuilder::dictionary( const std::string& objectId,
                                    const std::string& objectName,
                                    const std::string& date,
                                    const std::string& itemId,
                                    const std::string& hotzoneId ) {
    namespace ObjectFields = HubPlatform::CoreD::Model::Resources::DetectableObjectBaseFields;
    return JSON( objectId << JSON( ObjectFields::Name
                                   << objectName << ObjectFields::LastHotzone
                                   << JSON( ObjectFields::Date << date << ObjectFields::ItemId << itemId
                                                               << ObjectFields::Id << hotzoneId ) ) );
}

Json DetectableBuilder::dictionary( const std::string& objectId,
                                    const std::string& objectName,
                                    const std::string& objectValue,
                                    const std::string& date,
                                    const std::string& itemId,
                                    const std::string& hotzoneId ) {
    namespace ObjectFields = HubPlatform::CoreD::Model::Resources::DetectableObjectBaseFields;
    return JSON( objectId << JSON( ObjectFields::Name
                                   << objectName << ObjectFields::Value << objectValue << ObjectFields::LastHotzone
                                   << JSON( ObjectFields::Date << date << ObjectFields::ItemId << itemId
                                                               << ObjectFields::Id << hotzoneId ) ) );
}

Json DetectableBuilder::dictionary( const DetectableObject& object ) {
    return dictionary( object.id, object.name );
}

Json DetectableBuilder::dictionary( const DetectablePerson& person ) {
    return dictionary( person.id, person.name );
}

Json DetectableBuilder::dictionary( const DetectableQRCode& qrCode ) {
    return dictionary( qrCode.id, qrCode.name, qrCode.value );
}

template < class T >
Json detectableDictionary( const std::vector< T >& objects ) {
    namespace ObjectFields = HubPlatform::CoreD::Model::Resources::DetectableObjectBaseFields;
    Json result;
    for( auto& object : objects )
        result[ object.id ] = JSON( ObjectFields::Name << object.name );
    return result;
}

Json DetectableBuilder::dictionary( const std::vector< DetectableObject >& objects ) {
    namespace ObjectFields = HubPlatform::CoreD::Model::Resources::DetectableObjectBaseFields;
    Json result;
    for( auto& object : objects )
        result[ object.id ] = JSON( ObjectFields::Name << object.name );
    return result;
}

Json DetectableBuilder::dictionary( const std::vector< DetectablePerson >& persons ) {
    namespace ObjectFields = HubPlatform::CoreD::Model::Resources::DetectableObjectBaseFields;
    Json result;
    for( auto& person : persons )
        result[ person.id ] = JSON( ObjectFields::Name << person.name );
    return result;
}

Json DetectableBuilder::dictionary( const std::vector< DetectableQRCode >& qrCodes ) {
    namespace ObjectFields = HubPlatform::CoreD::Model::Resources::DetectableObjectBaseFields;
    Json result;
    for( auto& qrCode : qrCodes )
        result[ qrCode.id ] = JSON( ObjectFields::Name << qrCode.name << ObjectFields::Value << qrCode.value );
    return result;
}

Json HotzoneBuilder::hotzone( const std::string& hotzoneName, const DetectedObjects& detectedObjects ) {
    namespace HotzoneFields = HubPlatform::CoreD::Model::Resources::CameraHotzoneValueFields;

    auto hotzoneObjects = JSON_EMPTY_OBJ;
    for( auto& objects : detectedObjects ) {
        auto& type = objects.first;
        auto& ids = objects.second;
        auto& hotzoneObjectType = Vidoo4Hub::DetectableTypes::getHotzoneTypeName( type );
        hotzoneObjects[ hotzoneObjectType ] = ids;
    }

    return JSON( HotzoneFields::Name << hotzoneName << HotzoneFields::Detected << std::move( hotzoneObjects ) );
}

}  // namespace UT
}  // namespace HubPlatform
