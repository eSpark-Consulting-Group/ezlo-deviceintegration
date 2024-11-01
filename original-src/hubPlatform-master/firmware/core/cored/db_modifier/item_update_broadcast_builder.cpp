#include "cored/db_modifier/item_update_broadcast_builder.h"
#include "cored/db_modifier/item_update_broadcast_protect_notification.h"
#include "cored/features_manager.h"
#include "cored/core_service.h"

#include "cored/house_modes/resources/house_mode_resources.h"
#include "cored/house_modes/house_mode_utils.h"
#include "cored/house_modes/resources/house_mode_resources.h"

#include "database_client/dbqueryparser.h"
#include "database_client/idbclient.h"

#include "cored/model/resources/cored_resources.h"
#include "cored/model/configuration/configuration_factory.h"
#include "cored/model/visitors/ui_value_writer_visitor.h"
#include "cored/model/visitors/ui_value_formatted_getter_visitior.h"
#include "cored/model/values/value_factory.h"
#include "cored/model/values/value.h"
#include "cored/model/values/value_type.h"
#include "cored/model/visitors/item_value_float_visitor.h"
#include "cored/db_modifier/power_broadcast_modifier.h"
#include "cored/db_modifier/security_item_db_modifier.h"
#include "cored/db_modifier/item_update_broadcast_builder.h"
#include "db_modifier_utils.h"

#include <cassert>

namespace HubPlatform {
namespace CoreD {

ItemUpdateBroadcastBuilder::ItemUpdateBroadcastBuilder( Services::Service& service,
                                                        const ProvisionDataUtils::ProvisionData& provisionData,
                                                        const IFeaturesManager& featureManager,
                                                        SecurityItemDbModifier& securityItemDbModifier )
: m_service( service )
, m_dbClient( service.getDBClient() )
, m_provisionData( provisionData )
, m_featureManager( featureManager )
, m_securityItemDbModifier( securityItemDbModifier ) {
    m_modifyBroadcast.emplace_back( std::make_unique< PowerBroadcastModifier >( service.getDBClient() ) );
    m_modifyBroadcast.shrink_to_fit();
}

Json& ItemUpdateBroadcastBuilder::prepareUpdateBroadcastMessage( Json& broadcastData,
                                                                 const Json& oldFieldsValues,
                                                                 ShouldUserBeNotified shouldUserBeNotified ) {
    using namespace HubPlatform::CoreD::Model::Resources;
    const auto ItemId = json_get_str( broadcastData, ItemFields::IdFieldName );
    const auto item = m_dbClient.findOne( DBNames::ItemsDB, JSON( ItemFields::IdFieldName << ItemId ), nullptr );
    const auto deviceId = json_get_str( item, ItemFields::DeviceIdFieldName );
    const auto& device =
        m_dbClient.findOne( DBNames::DevicesDB, JSON( DeviceFields::IdFieldName << deviceId ), nullptr );
    const auto name = json_get_str( item, ItemFields::NameFieldName );
    const auto itemValueTypeStr = json_get_str( item, Model::Resources::ItemFields::TypeFieldName );
    const auto valueType = Model::ValueType::fromString( itemValueTypeStr );
    const auto currentHouseMode = CoreDHouseModes::getCurrentHouseMode( m_dbClient );
    const auto bypassStatus = m_securityItemDbModifier.getBypassModeProtectNotification( *currentHouseMode, deviceId );
    const auto protectMode = currentHouseMode->m_protect;
    const auto notificationUserIds = currentHouseMode->m_notificationUserIds;

    setGenericInfo( broadcastData, item, deviceId, notificationUserIds );
    addBroadcastTimestamp( broadcastData );
    setCategoryInfo( oldFieldsValues, shouldUserBeNotified, ItemId, deviceId, name, broadcastData, device );
    setRoomInfo( broadcastData, device );
    setUserCodeInfo( broadcastData, item, valueType );
    setValueInfo( broadcastData, item, itemValueTypeStr, valueType );
    setProtectNotificationInfo( broadcastData, device, protectMode, bypassStatus );
    broadcastData = DBModifierUtils::addProtectionConfig( broadcastData, device );

    return broadcastData;
}

Json& ItemUpdateBroadcastBuilder::renameScalableField( const std::string& replaceField,
                                                       const std::string& responseField,
                                                       const std::string& itemValueTypeStr,
                                                       Json& broadcastData ) {
    using namespace Model;
    using namespace Model::Resources;

    // replace valueMin/valueMax objects with minValue/maxValue: number
    auto fieldIt = broadcastData.find( replaceField );
    if( fieldIt != broadcastData.end() ) {
        auto& serialized = fieldIt;
        auto valueTypeStr =
            itemValueTypeStr.empty() ? json_get_str( broadcastData, ItemFields::TypeFieldName ) : itemValueTypeStr;
        const auto valueType = Model::ValueType::fromString( valueTypeStr );

        bool isScalable = valueType != Model::ValueType::Enum::NoValue && valueType.getScales().size() > 0;
        if( isScalable ) {
            auto config = Model::ConfigurationFactory::createConfiguration( m_service.getDBClient() );
            auto scale = config->getScaleForValueType( valueType );
            ItemValueFloatVisitor floatVisitor( scale.c_str() );

            const auto valueObject = ValueFactory::createItemValue( valueType, broadcastData[ replaceField ] );
            valueObject->accept( floatVisitor );

            auto fetchedValue = floatVisitor.getValue();

            json_remove( broadcastData, replaceField );
            json_add_float( broadcastData, responseField, fetchedValue );
        }
    }
    return broadcastData;
}

void ItemUpdateBroadcastBuilder::addBroadcastTimestamp( Json& broadcastData ) {
    using namespace std::chrono;
    int64_t now = duration_cast< milliseconds >( system_clock::now().time_since_epoch() ).count();
    broadcastData[ Model::Resources::System::FirmwareTimestampMs ] = now;
}

void ItemUpdateBroadcastBuilder::setValueInfo( Json& broadcastData,
                                               const Json& item,
                                               const std::string& itemValueTypeStr,
                                               const Model::ValueType& valueType ) {
    renameScalableField( Model::Resources::ItemFields::ValueMinFieldName,
                         Model::Resources::ItemFields::Response::MinValueFieldName,
                         itemValueTypeStr,
                         broadcastData );
    renameScalableField( Model::Resources::ItemFields::ValueMaxFieldName,
                         Model::Resources::ItemFields::Response::MaxValueFieldName,
                         itemValueTypeStr,
                         broadcastData );

    auto itemValueIt = broadcastData.find( Model::Resources::ItemFields::ValueFieldName );
    if( itemValueIt != broadcastData.end() && valueType != Model::ValueType::Enum::NoValue ) {
        auto config = Model::ConfigurationFactory::createConfiguration( m_service.getDBClient() );
        Model::UiValueWriterVisitor valueWriter( broadcastData, *config );

        auto valueObject = Model::ValueFactory::createItemValue( valueType, *itemValueIt );
        valueWriter.writeValue( *valueObject, Model::Resources::ItemFields::ValueFieldName );

        Model::UiValueFormattedGetterVisitor valueFormatter( *config );
        broadcastData[ Model::Resources::ItemFields::ValueFormattedFieldName ] =
            Json( valueFormatter.getValue( *valueObject ) );

        if( broadcastData.find( Model::Resources::ItemFields::TypeFieldName ) == broadcastData.end() ) {
            broadcastData[ Model::Resources::ItemFields::TypeFieldName ] =
                json_get_str( item, Model::Resources::ItemFields::TypeFieldName );
            Model::Item::decryptItemAsJson( m_provisionData, broadcastData );
            json_remove( broadcastData, Model::Resources::ItemFields::TypeFieldName );
        }
        else {
            Model::Item::decryptItemAsJson( m_provisionData, broadcastData );
        }
    }
}

void ItemUpdateBroadcastBuilder::setGenericInfo(
    Json& broadcastData,
    const Json& item,
    const std::string& deviceId,
    const CoreDHouseModes::HouseModeEntry::Notifications& notificationUserIds ) const {
    using namespace HubPlatform::CoreD::Model::Resources;
    broadcastData[ System::SyncNotification ] = json_contains( broadcastData, ItemFields::NameFieldName );
    broadcastData[ DictionaryEditRequestFields::DeviceIdFieldName ] = deviceId;

    CoreDHouseModes::HouseModeEntry::notificationsToJson( broadcastData[ "notifications" ], notificationUserIds );

    if( item.count( Model::Resources::ItemFields::StringRestrictionFieldName ) ) {
        broadcastData[ "stringRestriction" ] =
            json_get_str( item, Model::Resources::ItemFields::StringRestrictionFieldName );
    }

    if( item.count( Model::Resources::ItemFields::OneWeekDayCost ) ) {
        broadcastData[ "oneWeekDayCost" ] = item[ Model::Resources::ItemFields::OneWeekDayCost ];
    }

    if( item.count( Model::Resources::ItemFields::OneShiftedWeekDayCost ) ) {
        broadcastData[ "oneShiftedWeekDayCost" ] = item[ Model::Resources::ItemFields::OneShiftedWeekDayCost ];
    }

    if( item.count( Model::Resources::ItemFields::InfoFieldName ) ) {
        broadcastData[ "info" ] = item[ Model::Resources::ItemFields::InfoFieldName ];
    }
}

void ItemUpdateBroadcastBuilder::setCategoryInfo( const Json& oldFieldsValues,
                                                  const ShouldUserBeNotified& shouldUserBeNotified,
                                                  const std::string& ItemId,
                                                  const std::string& deviceId,
                                                  const std::string& name,
                                                  Json& broadcastData,
                                                  const Json& device ) const {
    using namespace HubPlatform::CoreD::Model::Resources;
    std::string deviceName;
    std::string parentDeviceId;

    std::string deviceCategory;
    std::string deviceSubcategory;
    if( device.empty() ) {
        Log::warn( std::string( "Item '" ) + ItemId + "' doesn't have related device(" + deviceId + ")" );
    }
    else {
        deviceName = json_get_str( device, Model::Resources::DeviceFields::NameFieldName );
        if( device.count( Model::Resources::DeviceFields::ArmedFieldName ) >= 1 ) {
            broadcastData[ "deviceArmed" ] = json_get_bool( device, Model::Resources::DeviceFields::ArmedFieldName );
        }

        parentDeviceId = json_get_str( device, Model::Resources::DeviceFields::ParentDeviceIdFieldName );

        deviceCategory = json_get_str( device, Model::Resources::DeviceFields::CategoryFieldName );
        deviceSubcategory = json_get_str( device, Model::Resources::DeviceFields::SubcategoryFieldName );
        bool serviceNotificationFlag;
        if( json_get(
                device, Model::Resources::DeviceFields::ServiceNotificationFieldName, serviceNotificationFlag ) ) {
            broadcastData[ Model::Resources::DeviceFields::ServiceNotificationFieldName ] = serviceNotificationFlag;
        }
    }

    broadcastData[ "deviceCategory" ] = deviceCategory;
    broadcastData[ "deviceSubcategory" ] = deviceSubcategory;
    broadcastData[ "deviceName" ] = deviceName;
    broadcastData[ Model::Resources::DictionaryEditRequestFields::NameFieldName ] = name;

    if( !parentDeviceId.empty() ) {
        broadcastData[ "parentDeviceId" ] = parentDeviceId;
    }

    auto prepareUpdateBroadcast =
        [&name, &deviceCategory, &deviceSubcategory, &broadcastData]( const BroadcastModifierUPtr& notifyUser ) {
            return notifyUser->prepareUpdateBroadcast( name, deviceCategory, deviceSubcategory, broadcastData );
        };
    bool isPrepared = std::any_of( begin( m_modifyBroadcast ), end( m_modifyBroadcast ), prepareUpdateBroadcast );
    if( !isPrepared ) {
        broadcastData[ "userNotification" ] =
            shouldUserBeNotified( name, deviceCategory, deviceSubcategory, broadcastData, oldFieldsValues );
    }
}

void ItemUpdateBroadcastBuilder::setRoomInfo( Json& broadcastData, const Json& device ) const {
    std::string roomName;
    const auto roomId = json_get_str( device, Model::Resources::DeviceFields::RoomIdFieldName );
    const auto roomObj = m_dbClient.findOne(
        Model::Resources::DBNames::RoomsDB, JSON( Model::Resources::RoomFields::IdFieldName << roomId ), nullptr );
    if( !roomObj.empty() ) {
        roomName = json_get_str( roomObj, Model::Resources::RoomFields::NameFieldName );
    }
    broadcastData[ "roomName" ] = roomName;
}

std::string getElementsMaxNumberKeyName( const Model::ValueType& valueType,
                                         Model::Item::ContainerLevel containerLevel ) {
    if( containerLevel == 0 )
        return "elementsMaxNumber";

    if( containerLevel == 1 && valueType.isArray() )
        return "elementsMaxNumberPerArray";

    assert( !"Please, add custom elementsMaxNumber for current container" );
    return "elementsMaxNumber" + std::to_string( containerLevel + 1 );
}

void ItemUpdateBroadcastBuilder::setUserCodeInfo( Json& broadcastData,
                                                  const Json& item,
                                                  const Model::ValueType& valueType ) const {
    Model::Item::ContainerLevel containerLevel = 0;
    for( auto type = &valueType; type && type->isContainer(); type = type->getSubType().get(), ++containerLevel ) {
        auto foundElementsMaxNumber = item.find( Model::Item::getElementsMaxNumberFieldName( containerLevel ) );
        if( foundElementsMaxNumber != item.end() ) {
            broadcastData[ getElementsMaxNumberKeyName( *type, containerLevel ) ] = foundElementsMaxNumber.value();
        }
    }

    if( item.count( Model::Resources::ItemFields::UserCodeRestrictionFieldName ) ) {
        broadcastData[ "userCodeRestriction" ] =
            json_get_str( item, Model::Resources::ItemFields::UserCodeRestrictionFieldName );
    }

    if( item.count( Model::Resources::ItemFields::UserCodeModesFieldName ) ) {
        broadcastData[ "userCodeModes" ] = item[ Model::Resources::ItemFields::UserCodeModesFieldName ];
    }

    if( item.count( Model::Resources::ItemFields::UserCodeRestrictionFieldName ) ) {
        broadcastData[ "userCodeRestriction" ] =
            json_get_str( item, Model::Resources::ItemFields::UserCodeRestrictionFieldName );
    }

    if( item.count( Model::Resources::ItemFields::UserCodeModesFieldName ) ) {
        broadcastData[ "userCodeModes" ] = item[ Model::Resources::ItemFields::UserCodeModesFieldName ];
    }
}

void ItemUpdateBroadcastBuilder::setProtectNotificationInfo(
    Json& broadcastData,
    const Json& device,
    bool protectMode,
    const SecurityItemDbModifier::BypassStatus& bypassStatus ) {
    if( m_securityItemDbModifier.inTestMode() ) {
        ProtectNotificationTestModeCase protectNotificationCase(
            broadcastData, device, protectMode, bypassStatus, m_featureManager, m_securityItemDbModifier, m_dbClient );
        protectNotificationCase.process();
    }
    else if( bypassStatus == SecurityItemDbModifier::BypassStatus::NONE ||
             bypassStatus == SecurityItemDbModifier::BypassStatus::SHUTDOWN_PASS ) {
        ProtectNotificationDefaultCase protectNotificationCase(
            broadcastData, device, protectMode, bypassStatus, m_featureManager, m_securityItemDbModifier, m_dbClient );
        protectNotificationCase.process();
    }
    else if( bypassStatus == SecurityItemDbModifier::BypassStatus::BYPASS ) {
        ProtectNotificationBypassCase protectNotificationCase(
            broadcastData, device, protectMode, bypassStatus, m_featureManager, m_securityItemDbModifier, m_dbClient );
        protectNotificationCase.process();
    }
    else if( bypassStatus == SecurityItemDbModifier::BypassStatus::SHUTDOWN_FILTERED ) {
        ProtectNotificationShutdownFilteredCase protectNotificationCase(
            broadcastData, device, protectMode, bypassStatus, m_featureManager, m_securityItemDbModifier, m_dbClient );
        protectNotificationCase.process();
    }
    else if( bypassStatus == SecurityItemDbModifier::BypassStatus::SHUTDOWN_FIRST_TIME ) {
        ProtectNotificationShutdownFirstTimeCase protectNotificationCase(
            broadcastData, device, protectMode, bypassStatus, m_featureManager, m_securityItemDbModifier, m_dbClient );
        protectNotificationCase.process();
    }
}

}  // namespace CoreD
}  // namespace HubPlatform
