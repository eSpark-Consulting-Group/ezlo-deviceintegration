#include "vidoo4hub/vidoo_device/vidoo_device.h"

#include "vidoo4hub/core_manager/core_manager_interface.h"
#include "vidoo4hub/vidoo4hub_exception.h"
#include "vidoo4hub/resources/vidoo4hub_resources.h"

#include "json_utils/json_utils.h"
#include "database_client/dbqueryparser.h"
#include "cored/model/cored_device.h"
#include "cored/model/cored_item.h"
#include "cored/model/values/value_factory.h"

namespace HubPlatform {
namespace Vidoo4Hub {

namespace {

std::string createVidooDevice( CoreDRemoteInterface::CoreDInterface& core, const std::string& gatewayId ) {
    HubPlatform::CoreD::Model::Device device(
        gatewayId, "Vidoo Device", "vidoo_device", "vidoo_device", "camera_suite", false );
    device.setSubcategory( "vidoo" );

    auto result = core.addDevice( device );
    if( !result.second )
        throw Vidoo4HubError( "Failed to add vidoo device" );

    const auto deviceId = std::move( result.first );
    return deviceId;
}

std::string createDetectableItem( CoreDRemoteInterface::CoreDInterface& core,
                                  const std::string& deviceId,
                                  DetectableTypes::Type type ) {
    using namespace HubPlatform::CoreD::Model;
    using namespace Vidoo4Hub::Resources::DB;

    const std::map< DetectableTypes::Type, ValueType::Enum > coreDTypeMap{
        { DetectableTypes::Type::Object, ValueType::Enum::DetectableObject },
        { DetectableTypes::Type::Person, ValueType::Enum::DetectablePerson },
        { DetectableTypes::Type::QRCode, ValueType::Enum::DetectableQRCode }
    };

    const std::map< DetectableTypes::Type, const char* > itemNames{
        { DetectableTypes::Type::Object, FieldNames::objectListItemName },
        { DetectableTypes::Type::Person, FieldNames::peopleListItemName },
        { DetectableTypes::Type::QRCode, FieldNames::qrCodeListItemName }
    };

    auto valueType = ValueType::makeDictionaryType( coreDTypeMap.at( type ) );
    auto value = ValueFactory::createDefaultItemValue( valueType );
    const std::string itemName = itemNames.at( type );

    HubPlatform::CoreD::Model::Item item( deviceId, itemName, valueType, true, false, std::move( value ) );
    item.setShow( true );

    auto result = core.addItem( item );
    auto itemId = std::move( result.first );
    if( !result.second )
        throw Vidoo4HubError( "Failed to add " + itemName + " item" );
    return itemId;
}

const char* getItemIdDbField( DetectableTypes::Type type ) {
    using namespace Vidoo4Hub::Resources::DB;

    static const std::map< DetectableTypes::Type, const char* > names = {
        { DetectableTypes::Type::Object, FieldNames::objectListItemId },
        { DetectableTypes::Type::Person, FieldNames::peopleListItemId },
        { DetectableTypes::Type::QRCode, FieldNames::qrCodeListItemId }
    };
    return names.at( type );
}

}  // namespace

VidooDevice::VidooDevice( Services::Service& service, ICoreManager& coreManager, std::string coreDGatewayId ) {
    using namespace Vidoo4Hub::Resources::DB;
    auto& dbClient = service.getDBClient();
    auto& core = coreManager.core();

    readFromDb( dbClient );
    try {
        if( m_deviceId.empty() )
            m_deviceId = createVidooDevice( core, coreDGatewayId );

        for( auto& type : DetectableTypes::getKnownTypes() ) {
            if( !m_detectableItemsMap.count( type ) )
                m_detectableItemsMap.emplace( type, createDetectableItem( core, m_deviceId, type ) );
        }

        writeToDb( dbClient );
    }
    catch( const Vidoo4HubError& e ) {
        dbClient.clear( TableNames::VidooDeviceDBName );

        for( auto& item : m_detectableItemsMap ) {
            const auto& itemId = item.second;
            core.removeItem( itemId );
        }
        m_detectableItemsMap.clear();

        if( !m_deviceId.empty() ) {
            core.removeDevice( m_deviceId );
            m_deviceId.clear();
        }
        throw;
    }
}

std::string VidooDevice::getDeviceId() {
    return m_deviceId;
}

std::string VidooDevice::getDetectableItemId( DetectableTypes::Type type ) {
    return m_detectableItemsMap.at( type );
}

bool VidooDevice::readFromDb( IDbClient& dbClient ) {
    using namespace Vidoo4Hub::Resources::DB;
    auto query = JSON( db::ID_FIELD_NAME << FieldNames::vidooDeviceId );
    auto result = dbClient.findOne( TableNames::VidooDeviceDBName, query, nullptr );
    if( result.empty() )
        return false;

    m_deviceId = json_get_str( result, FieldNames::vidooDeviceId );

    for( auto& type : DetectableTypes::getKnownTypes() ) {
        std::string itemId = json_get_str( result, getItemIdDbField( type ) );
        if( !itemId.empty() )
            m_detectableItemsMap.emplace( type, std::move( itemId ) );
    }

    return true;
}

void VidooDevice::writeToDb( IDbClient& dbClient ) {
    using namespace Vidoo4Hub::Resources::DB;
    auto j = JSON( db::ID_FIELD_NAME << FieldNames::vidooDeviceId << FieldNames::vidooDeviceId << m_deviceId );

    for( auto& item : m_detectableItemsMap ) {
        auto& type = item.first;
        const auto& itemDbField = getItemIdDbField( type );
        j[ itemDbField ] = item.second;
    }
    dbClient.update( TableNames::VidooDeviceDBName, JSON_EMPTY_OBJ, j, true, false );
}

}  // namespace Vidoo4Hub
}  // namespace HubPlatform
