#include "cored/device_manager.h"
#include "cored/core_service.h"
#include "cored/exceptions.h"
#include "cored/db_modifier/gateway_setting_db_modifier.h"
#include "cored/db_modifier/device_setting_db_modifier.h"
#include "cored/db_modifier/gateway_db_modifier.h"
#include "cored/db_modifier/item_db_modifier.h"
#include "cored/db_modifier/device_db_modifier.h"
#include "cored/db_modifier/field_modifier.h"
#include "cored/model/resources/cored_resources.h"
#include "cored/model/cored_item.h"
#include "database_client/db_tables_names.h"
#include "service_interfaces/service.h"
#include "base_utils/ezlodb.h"
#include "json_utils/json_utils.h"
#include "jsonrpc/jsonrpc.h"
#include "log_utils/ezlo_log.h"

#include "tiny/tiny_utils.h"  // tiny::tools::trim( std::string, ... )

#include <cassert>
#include "database_client/dbqueryparser.h"
#include "database_client/idbclient.h"

namespace HubPlatform {
namespace CoreD {

DeviceManager::DeviceManager( CoreService& coreService, const ProvisionDataUtils::ProvisionData& provisionData )
: m_coreService( coreService )
, m_provisionData( provisionData ) {}

DeviceManager::GatewayID DeviceManager::ensureGateway( const JsonObject& gateway ) {
    using namespace Model::Resources;
    std::string gatewayName = json_get_str( gateway, "name" );
    const auto existingGateway =
        m_coreService.getDBClient().findOne( DBNames::GatewaysDB, JSON( "name" << gatewayName ), nullptr );

    std::string gatewayId;
    if( !existingGateway.empty() ) {
        auto newGateway = gateway;
        // Do not overwrite manualDeviceAdding field
        if( newGateway.find( GatewayFields::ManualDeviceAddingFieldName ) != newGateway.end() ) {
            newGateway.erase( GatewayFields::ManualDeviceAddingFieldName );
        }

        // Do not overwrite settings field
        if( newGateway.find( GatewayFields::SettingsFieldName ) != newGateway.end() ) {
            newGateway.erase( GatewayFields::SettingsFieldName );
        }

        gatewayId = json_get_str( existingGateway, db::ID_FIELD_NAME );
        m_coreService.getGatewaysDbModifier().update( gatewayId, newGateway );
    }
    else {
        gatewayId = generateUniqueID();
        auto gate = gateway;
        gate[ db::ID_FIELD_NAME ] = gatewayId;
        m_coreService.getGatewaysDbModifier().insert( gate );
    }

    /* note we use an "id" instead of db::ID_FIELD_NAME to untie an id provided to client and an id used by database */
    return JSON( "id" << gatewayId );
}

DeviceManager::GatewaySettingId DeviceManager::addGatewaySetting( const JsonObject& setting ) {
    m_coreService.getGatewaySettingsDbModifier().insert( setting );

    LOGGER_TRACE( cc::trace( "gateway setting has been added, id:" ) +
                  cc::note( json_get_str( setting, Model::Resources::GatewaySettingFields::Id ) ) );

    /* note we use an "id" instead of db::ID_FIELD_NAME to untie an id provided to client and an id used by database */
    return JSON( "id" << setting[ Model::Resources::GatewaySettingFields::Id ] );
}

DeviceManager::DeviceID DeviceManager::addDevice( const JsonObject& device ) {
    auto dev = device;

    const auto& db = m_coreService.getDBClient();

    if( db.count( HaDevicesDBName, JSON( db::ID_FIELD_NAME << dev[ db::ID_FIELD_NAME ] ) ) ) {
        return JSON(
            "id" << dev[ db::ID_FIELD_NAME ] ); /* we're trying to add a new device, not to update an existing one */
    }

    auto parentDeviceId = json_get_str( device, Model::Resources::DeviceFields::ParentDeviceIdFieldName );
    if( !parentDeviceId.empty() ) {
        const JsonObject parentDevice = db.findOne(
            HaDevicesDBName, JSON( Model::Resources::DeviceFields::IdFieldName << parentDeviceId ), nullptr );
        if( parentDevice.empty() ) {
            throw DeviceManagerException( "Parent device with such ID doesn't exist" );
        }
        auto parentDeviceId2 = json_get_str( parentDevice, Model::Resources::DeviceFields::ParentDeviceIdFieldName );

        if( !parentDeviceId2.empty() ) {
            throw NestedParentDevicesNotAllowedException(
                "Child devices are not allowed to create further child devices" );
        }
        auto parentDeviceRoomId = json_get_str( parentDevice, Model::Resources::DeviceFields::RoomIdFieldName );
        if( !parentDeviceRoomId.empty() ) {
            dev[ Model::Resources::DeviceFields::RoomIdFieldName ] = parentDeviceRoomId;
        }
    }

    dev[ db::ID_FIELD_NAME ] = generateUniqueID();
    if( !json_contains( dev, Model::Resources::DeviceFields::ProtectConfig ) ) {
        dev[ Model::Resources::DeviceFields::ProtectConfig ] = Model::Resources::ProtectConfigName::Default;
    }
    m_coreService.getDevicesDbModifier().insert( dev );

    /* note we use an "id" instead of db::ID_FIELD_NAME to untie an id provided to client and an id used by database */
    return JSON( "id" << dev[ db::ID_FIELD_NAME ] );
}

DeviceManager::DeviceID DeviceManager::renameDevice( const JsonObject& deviceName ) {
    JsonObject response = JSON_EMPTY_OBJ;
    const auto& db = m_coreService.getDBClient();

    const DeviceID deviceId = json_get_str( deviceName, "_id" );
    std::string name;
    if( !getNonEmptyField( deviceName, "name", response, name, true ) ) {
        response[ "id" ] = deviceId;
        LOGGER_DEBUG_RAW( "DeviceManager::renameDevice device name is empty" );
        return response;
    }
    // Shrink name length to max limit.
    auto nameLength = name.length();
    if( nameLength > NameMaxLength ) {
        name.resize( NameMaxLength );
    }

    const JsonObject device =
        db.findOne( HaDevicesDBName, JSON( Model::Resources::DeviceFields::IdFieldName << deviceId ), nullptr );
    if( device.empty() ) {
        LOGGER_DEBUG_RAW( "DeviceManager::renameDevice db is empty" );
        return JSON( "id" << deviceId );
    }

    m_coreService.getDevicesDbModifier().update( deviceId,
                                                 JSON( Model::Resources::DeviceFields::NameFieldName << name ) );

    return JSON( "id" << deviceId );
}

DeviceManager::DeviceID DeviceManager::placeDevice( const std::string& deviceId,
                                                    const std::string& newRoomId,
                                                    const std::set< std::string >& separateChildDevices ) {
    using namespace Model::Resources;
    const auto& db = m_coreService.getDBClient();
    const JsonObject device = db.findOne( HaDevicesDBName, JSON( DeviceFields::IdFieldName << deviceId ), nullptr );
    auto parentDevice = json_get_str( device, DeviceFields::ParentDeviceIdFieldName );
    auto parentRoom = json_get_bool( device, DeviceFields::ParentRoomFieldName, true );
    bool isParent = parentDevice.empty();
    bool isChild = !isParent;

    // child devices except list

    std::string roomId = newRoomId;
    if( isParent == true ) {
        if( parentRoom == false || json_get_str( device, "roomId" ) != roomId ) {
            m_coreService.getDevicesDbModifier().update( deviceId, JSON( DeviceFields::RoomIdFieldName << roomId ) );

            auto childDevices =
                db.findAll( HaDevicesDBName, JSON( DeviceFields::ParentDeviceIdFieldName << deviceId ), nullptr );
            for( auto& child : childDevices ) {
                std::string Id = json_get_str( child, DeviceFields::IdFieldName );
                auto parentRoom = json_get_bool( child, DeviceFields::ParentRoomFieldName, true );
                std::string childDeviceId;
                if( json_get( child, DeviceFields::IdFieldName, childDeviceId ) ) {
                    if( parentRoom == true || ( parentRoom == false && separateChildDevices.empty() ) ||
                        ( parentRoom == false && !separateChildDevices.empty() &&
                          !separateChildDevices.count( childDeviceId ) ) ) {
                        m_coreService.getDevicesDbModifier().update( childDeviceId,
                                                                     JSON( DeviceFields::RoomIdFieldName << roomId ) );
                    }
                }
            }
        }
    }
    else if( isChild == true && parentRoom == false ) {
        if( json_get_str( device, "roomId" ) != roomId ) {
            m_coreService.getDevicesDbModifier().update( deviceId, JSON( DeviceFields::RoomIdFieldName << roomId ) );
        }
    }

    return JSON( "id" << deviceId );
}

void DeviceManager::unplaceDevice( const std::string& deviceId ) {
    using namespace Model::Resources;
    const auto& db = m_coreService.getDBClient();
    const JsonObject devices = db.findAll( HaDevicesDBName, JSON_EMPTY_OBJ, nullptr );
    const JsonObject device = db.findOne( HaDevicesDBName, JSON( DeviceFields::IdFieldName << deviceId ), nullptr );
    auto parentRoomId = json_get_str( device, DeviceFields::RoomIdFieldName );

    if( json_get_str( device, "roomId" ) != "" ) {
        m_coreService.getDevicesDbModifier().update( deviceId, JSON( DeviceFields::RoomIdFieldName << "" ) );
    }
    if( !parentRoomId.empty() ) {
        auto devices =
            db.findAll( HaDevicesDBName, JSON( DeviceFields::ParentDeviceIdFieldName << deviceId ), nullptr );
        for( auto& subDevice : devices ) {
            std::string subDeviceId;
            bool parentRoom = json_get_bool( subDevice, DeviceFields::ParentRoomFieldName, true );
            auto childRoomId = json_get_str( subDevice, DeviceFields::RoomIdFieldName );
            if( json_get( subDevice, DeviceFields::IdFieldName, subDeviceId ) ) {
                if( parentRoom == true || ( parentRoom == false && childRoomId == parentRoomId ) )
                    m_coreService.getDevicesDbModifier().update(
                        subDeviceId, JSON( DeviceFields::RoomIdFieldName << std::string() ) );
            }
        }
    }
}

DeviceManager::ItemID DeviceManager::addItem( const JsonObject& joItem ) {
    auto item = joItem;

    if( m_coreService.getDBClient().count( HaItemsDBName, JSON( db::ID_FIELD_NAME << item[ db::ID_FIELD_NAME ] ) ) ) {
        return JSON(
            "id" << item[ db::ID_FIELD_NAME ] ); /* we're trying to add a new item, not to update an existing one */
    }

    checkValueIndentItem( joItem );

    item[ db::ID_FIELD_NAME ] = generateUniqueID();

    Model::Item::encryptItemAsJson( m_provisionData, item );
    m_coreService.getItemsDbModifier().insert( item );

    /* note we use an "id" instead of db::ID_FIELD_NAME to untie an id provided to client and an id used by database */
    return JSON( "id" << item[ db::ID_FIELD_NAME ] );
}

DeviceManager::DeviceSettingID DeviceManager::addDeviceSetting( const JsonObject& joSetting ) {
    auto setting = joSetting;

    if( m_coreService.getDBClient().count( HaDeviceSettingsDBName,
                                           JSON( db::ID_FIELD_NAME << setting[ db::ID_FIELD_NAME ] ) ) ) {
        return JSON(
            "id" << setting[ db::ID_FIELD_NAME ] ); /* we're trying to add a new item, not to update an existing one */
    }

    setting[ db::ID_FIELD_NAME ] = generateUniqueID();

    m_coreService.getDeviceSettingsDbModifier().insert( setting );

    /* note we use an "id" instead of db::ID_FIELD_NAME to untie an id provided to client and an id used by database */
    return JSON( "id" << setting[ db::ID_FIELD_NAME ] );
}

DeviceManager::DeviceID DeviceManager::setDeviceArmed( const JsonObject& deviceArmedData ) {
    auto& deviceDb = m_coreService.getDevicesDbModifier();
    deviceDb.update( deviceArmedData[ db::ID_FIELD_NAME ],
                     JSON( "armed" << deviceArmedData[ "armed" ].get< bool >() ) );
    return JSON( "id" << deviceArmedData[ db::ID_FIELD_NAME ] );
}

DeviceManager::DeviceID DeviceManager::setDeviceProtectConfig( const JsonObject& deviceProtectConfigData ) {
    auto& deviceDb = m_coreService.getDevicesDbModifier();
    deviceDb.update( deviceProtectConfigData[ db::ID_FIELD_NAME ],
                     JSON( Model::Resources::DeviceFields::ProtectConfig
                           << deviceProtectConfigData[ Model::Resources::DeviceFields::ProtectConfig ] ) );
    return JSON( "id" << deviceProtectConfigData[ db::ID_FIELD_NAME ] );
}

DeviceManager::GatewayID DeviceManager::modifyGateway( const std::string& gatewayId, const JsonObject& gateway ) {
    namespace Fields = Model::Resources::GatewayFields;

    const auto& db = m_coreService.getDBClient();
    const JsonObject existingGateway =
        db.findOne( Model::Resources::DBNames::GatewaysDB, JSON( Fields::IdFieldName << gatewayId ), nullptr );
    if( existingGateway.empty() ) {
        LOGGER_DEBUG( "DeviceManager::modifyGateway gateway " + gatewayId + " not fount ind db " +
                      Model::Resources::DBNames::GatewaysDB );
        return JSON( "id" << gatewayId );
    }

    FieldModifier( m_coreService.getGatewaysDbModifier(), gateway, existingGateway, gatewayId )
        .addField< std::string >( Fields::ManualDeviceAddingFieldName )
        .addField< std::string >( Fields::StatusFieldName )
        .addField< bool >( Fields::ReadyFieldName )
        .addField< JsonObject >( Fields::InfoFieldName )
        .modifyFields();

    return JSON( "id" << gatewayId );
}

DeviceManager::DeviceID DeviceManager::modifyDevice( const std::string& deviceId, const JsonObject& device ) {
    namespace Fields = Model::Resources::DeviceFields;

    const auto& db = m_coreService.getDBClient();
    const JsonObject existingDevice =
        db.findOne( HaDevicesDBName, JSON( Model::Resources::DeviceFields::IdFieldName << deviceId ), nullptr );
    if( existingDevice.empty() ) {
        LOGGER_DEBUG_RAW( "DeviceManager::modifyDevice db is empty" );
        return JSON( "id" << deviceId );
    }
    auto parentDeviceId = json_get_str( device, Fields::ParentDeviceIdFieldName );
    if( !parentDeviceId.empty() ) {
        const JsonObject parentDevice = db.findOne(
            HaDevicesDBName, JSON( Model::Resources::DeviceFields::IdFieldName << parentDeviceId ), nullptr );
        if( parentDevice.empty() ) {
            LOGGER_DEBUG_RAW( "DeviceManager::modifyDevice: parent device doesn't exist" );
            throw DeviceManagerException( "Parent device with such ID doesn't exist" );
        }
        auto parentDeviceId2 = json_get_str( parentDevice, Fields::ParentDeviceIdFieldName );
        if( !parentDeviceId2.empty() ) {
            throw NestedParentDevicesNotAllowedException(
                "Child devices are not allowed to create further child devices" );
        }
    }

    FieldModifier( m_coreService.getDevicesDbModifier(), device, existingDevice, deviceId )
        .addField< std::string >( Fields::SecurityFieldName )
        .addField< std::string >( Fields::CategoryFieldName )
        .addField< std::string >( Fields::SubcategoryFieldName )
        .addField< std::string >( Fields::TypeFieldName )
        .addField< std::string >( Fields::NameFieldName )
        .addField< std::string >( Fields::ParentDeviceIdFieldName )
        .addField< JsonObject >( Fields::InfoFieldName )
        .addField< JsonObject >( Fields::FirmwareFieldName )
        .addField< JsonObject >( Fields::HouseModesOptionsFieldName )
        .addField< std::string >( Fields::DeviceTypeIdFieldName )
        .addField< bool >( Fields::BatteryPoweredFieldName )
        .addField< bool >( Fields::ParentRoomFieldName )
        .modifyFields();

    return JSON( "id" << deviceId );
}

DeviceManager::ItemID DeviceManager::modifyItem( const std::string& itemId, const JsonObject& item ) {
    namespace Fields = Model::Resources::ItemFields;

    auto& db = m_coreService.getDBClient();
    db.remove( HaTempItemsDBName, JSON( db::ID_FIELD_NAME << itemId ) );
    auto newItem = item;
    JsonObject existingItem = fetchItem( itemId );
    if( existingItem.empty() ) {
        LOGGER_DEBUG_RAW( "DeviceManager::modifyItem db is empty" );
        return JSON( "id" << itemId );
    }

    checkValueIndentItem( item );

    Model::Item::encryptItemAsJson( m_provisionData, newItem );

    FieldModifier( m_coreService.getItemsDbModifier(), newItem, existingItem, itemId )
        .addField< std::string >( Fields::DeviceIdFieldName )
        .addField< JsonArray >( Fields::EnumFieldName )
        .addField< bool >( Fields::HasGetterFieldName )
        .addField< bool >( Fields::HasSetterFieldName )
        .addField< std::string >( Fields::NameFieldName )
        .addField< bool >( Fields::ShowFieldName )
        .addField< std::string >( Fields::TypeFieldName )
        .addField< JsonObject >( Fields::ValueFieldName )
        .addField< JsonObject >( Fields::ValueMinFieldName )
        .addField< JsonObject >( Fields::ValueMaxFieldName )
        .addField< uint32_t >( Fields::ElementsMaxNumberFieldName )
        .addField< std::string >( Fields::UserCodeRestrictionFieldName )
        .addField< JsonArray >( Fields::UserCodeModesFieldName )
        .addField< std::string >( Fields::StringRestrictionFieldName )
        .addField< JsonObject >( Fields::ValueIndentFieldName )
        .addField< JsonObject >( Fields::InfoFieldName )
        .modifyFields();

    return JSON( "id" << itemId );
}

DeviceManager::DeviceSettingID DeviceManager::modifySetting( const std::string& settingId, const JsonObject& setting ) {
    namespace Fields = Model::Resources::DeviceSettingFields;

    const auto& db = m_coreService.getDBClient();
    const JsonObject existingSetting =
        db.findOne( HaDeviceSettingsDBName, JSON( Model::Resources::DeviceSettingFields::Id << settingId ), nullptr );

    if( existingSetting.empty() ) {
        LOGGER_DEBUG_RAW( "DeviceManager::modifySetting db is empty" );
        return JSON( "id" << settingId );
    }

    FieldModifier( m_coreService.getDeviceSettingsDbModifier(), setting, existingSetting, settingId )
        .addField< std::string >( Fields::DeviceId )
        .addField< std::string >( Fields::ValueType )
        .addField< JsonObject >( Fields::Label )
        .addField< JsonObject >( Fields::Description )
        .addField< JsonObject >( Fields::ValueDefault )
        .addField< JsonObject >( Fields::ValueMin )
        .addField< JsonObject >( Fields::ValueMax )
        .addField< JsonObject >( Fields::Enum )
        .addField< bool >( Fields::HasSetter )
        .addField< uint32_t >( Fields::ElementsMaxNumber )
        .addField< JsonObject >( Fields::Info )
        .addField< uint32_t >( Fields::ZwaveParameterNumber )
        .addField< uint32_t >( Fields::ZwaveParameterSize )
        .modifyFields();

    return JSON( "id" << settingId );
}

std::string DeviceManager::generateUniqueID() {
    /* TODO: link a size of a uid string with an max allowed size of a field within a json */
    return Db::genOid();
}

void DeviceManager::checkValueIndentItem( const JsonObject& item ) const {
    using namespace Model::Resources::ItemFields;
    auto it = item.find( ValueIndentFieldName );
    if( it != item.end() ) {
        auto valueIndentItemId = json_get_str( *it, ValueIndentItemFieldName );
        if( !m_coreService.getDBClient().count( HaItemsDBName, JSON( db::ID_FIELD_NAME << valueIndentItemId ) ) ) {
            throw DeviceManagerException( "Item '" + valueIndentItemId + "' does not exist" );
        }
    }
}

JsonObject DeviceManager::fetchDevice( const std::string& deviceId ) {
    return m_coreService.getDBClient().findOne(
        HaDevicesDBName, JSON( Model::Resources::DeviceFields::IdFieldName << deviceId ), nullptr );
}

JsonObject DeviceManager::fetchItem( const std::string& itemId ) {
    return m_coreService.getDBClient().findOne(
        HaItemsDBName, JSON( Model::Resources::ItemFields::IdFieldName << itemId ), nullptr );
}

}  // namespace CoreD
}  // namespace HubPlatform
