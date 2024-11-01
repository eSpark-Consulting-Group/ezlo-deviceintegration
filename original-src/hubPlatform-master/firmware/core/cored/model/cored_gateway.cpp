#include "cored/model/cored_gateway.h"

#include "cored/model/resources/cored_resources.h"

#include "json_utils/json_utils.h"

namespace HubPlatform {
namespace CoreD {
namespace Model {

using namespace Resources::GatewayFields;

Gateway::Gateway( std::string name,
                  GatewayUnreachableReasons unreachableReasons,
                  GatewayUnreachableActions unreachableActions,
                  std::string pluginId,
                  Operations operations,
                  std::string label,
                  std::string reason,
                  std::string setItemValueCommand,
                  std::string setItemValueResponsePolicy,
                  std::string addItemDictionaryValueCommand,
                  std::string setItemDictionaryValueCommand,
                  std::string removeItemDictionaryValueCommand,
                  std::string clearItemDictionaryCommand,
                  std::string setGatewaySettingValueCommand,
                  std::string setSettingValueCommand,
                  std::string setSettingDictionaryValueCommand,
                  std::string resetSettingCommand,
                  std::string forceRemoveDeviceCommand,
                  std::string updateDeviceFirmwareCommand,
                  std::string checkDeviceCommand,
                  std::string createAbstractSubscriptionCommand,
                  std::string updateAbstractSubscriptionCommand,
                  std::string deleteAbstractSubscriptionCommand,
                  std::string dispatchAbstractSubscriptionCommand )
: m_name( std::move( name ) )
, m_pluginId( std::move( pluginId ) )
, m_unreachableReasons( std::move( unreachableReasons ) )
, m_unreachableActions( std::move( unreachableActions ) )
, m_operations( std::move( operations ) )
, m_label( std::move( label ) )
, m_reason( std::move( reason ) )
, m_setItemValueCommand( std::move( setItemValueCommand ) )
, m_setItemValueResponsePolicy( std::move( setItemValueResponsePolicy ) )
, m_addItemDictionaryValueCommand( std::move( addItemDictionaryValueCommand ) )
, m_setItemDictionaryValueCommand( std::move( setItemDictionaryValueCommand ) )
, m_removeItemDictionaryValueCommand( std::move( removeItemDictionaryValueCommand ) )
, m_clearItemDictionaryCommand( std::move( clearItemDictionaryCommand ) )
, m_setGatewaySettingValueCommand( std::move( setGatewaySettingValueCommand ) )
, m_setSettingValueCommand( std::move( setSettingValueCommand ) )
, m_setSettingDictionaryValueCommand( std::move( setSettingDictionaryValueCommand ) )
, m_resetSettingCommand( std::move( resetSettingCommand ) )
, m_forceRemoveDeviceCommand( std::move( forceRemoveDeviceCommand ) )
, m_updateDeviceFirmwareCommand( std::move( updateDeviceFirmwareCommand ) )
, m_checkDeviceCommand( std::move( checkDeviceCommand ) )
, m_createAbstractSubscriptionCommand( std::move( createAbstractSubscriptionCommand ) )
, m_updateAbstractSubscriptionCommand( std::move( updateAbstractSubscriptionCommand ) )
, m_deleteAbstractSubscriptionCommand( std::move( deleteAbstractSubscriptionCommand ) )
, m_dispatchAbstractSubscriptionCommand( std::move( dispatchAbstractSubscriptionCommand ) )
, m_status( GatewayStatus::NotStarted )
, m_manualDeviceAddingCapability( ManualDeviceAddingCapability::Wizard ) {
    if( m_label.empty() )
        m_label = m_name; /* why, why not? */
}

Gateway& Gateway::setLabel( const std::string& label ) {
    m_label = label;
    return *this;
}

Gateway& Gateway::setReason( const std::string& reason ) {
    m_reason = reason;
    return *this;
}

Gateway& Gateway::setStatus( const GatewayStatus& status ) {
    m_status = status;
    return *this;
}

JsonObject Gateway::getButtonMethod( const std::string& label,
                                     const std::string& description,
                                     const std::string& method ) const {
    return JSON( "type"
                 << "button"
                 << "label" << label << "description" << description << "method" << method );
}

Json Gateway::toJson() const {
    auto getUnreachableReasons = []( const GatewayUnreachableReasons& reasons ) {
        JsonArray result = JSON_EMPTY_ARR;

        for( const auto& reason : reasons )
            result.push_back( JSON( "reason" << reason.first << "advice" << reason.second ) );

        return result;
    };

    auto getUnreachableActions = []( const GatewayUnreachableActions& actions ) {
        JsonArray result = JSON_EMPTY_ARR;

        for( const auto& action : actions )
            result.push_back( JSON( "label" << action.first << "method" << action.second ) );

        return result;
    };

    auto gateway = JSON_EMPTY_OBJ;

    gateway[ NameFieldName ] = m_name;
    gateway[ UnreachableReasonsFieldName ] = getUnreachableReasons( m_unreachableReasons );
    gateway[ UnreachableActionsFieldName ] = getUnreachableActions( m_unreachableActions );
    gateway[ PluginIdFieldName ] = m_pluginId;
    gateway[ LabelFieldName ] = m_label;
    gateway[ ReasonFieldName ] = m_reason;
    gateway[ SetItemValueCommandFieldName ] = m_setItemValueCommand;
    gateway[ SetItemValueResponsePolicyFieldName ] = m_setItemValueResponsePolicy;
    gateway[ AddItemDictionaryValueCommandFieldName ] = m_addItemDictionaryValueCommand;
    gateway[ SetItemDictionaryValueCommandFieldName ] = m_setItemDictionaryValueCommand;
    gateway[ RemoveItemDictionaryValueCommandFieldName ] = m_removeItemDictionaryValueCommand;
    gateway[ ClearItemDictionaryCommandFieldName ] = m_clearItemDictionaryCommand;
    gateway[ SetGatewaySettingValueCommandFieldName ] = m_setGatewaySettingValueCommand;
    gateway[ SetSettingValueCommandFieldName ] = m_setSettingValueCommand;
    gateway[ SetSettingDictionaryValueCommandFieldName ] = m_setSettingDictionaryValueCommand;
    gateway[ ResetSettingCommandFieldName ] = m_resetSettingCommand;
    gateway[ ForceRemoveDeviceCommandFieldName ] = m_forceRemoveDeviceCommand;
    gateway[ UpdateDeviceFirmwareCommandFieldName ] = m_updateDeviceFirmwareCommand;
    gateway[ CheckDeviceCommandFieldName ] = m_checkDeviceCommand;
    gateway[ CreateAbstractSubscriptionCommandFieldName ] = m_createAbstractSubscriptionCommand;
    gateway[ UpdateAbstractSubscriptionCommandFieldName ] = m_updateAbstractSubscriptionCommand;
    gateway[ DeleteAbstractSubscriptionCommandFieldName ] = m_deleteAbstractSubscriptionCommand;
    gateway[ DispatchAbstractSubscriptionCommandFieldName ] = m_dispatchAbstractSubscriptionCommand;
    gateway[ StatusFieldName ] = toString( m_status );
    gateway[ ManualDeviceAddingFieldName ] = toString( m_manualDeviceAddingCapability );
    gateway[ ReadyFieldName ] = m_ready;
    gateway[ SettingsFieldName ] = m_settingsAmount;

    if( !m_operations.empty() ) {
        auto& jsonOperations = gateway[ "operations" ];
        for( const auto& operation : m_operations ) {
            jsonOperations[ operation.m_name ] = operation.m_template;
        }
    }
    return gateway;
}

void Gateway::checkGatewayAsJson( const Json& gateway ) {
    auto jsonCheckStringStringArray = []( const Json& jsArray ) {
        if( jsArray.empty() ) {
            throw std::runtime_error{ "array is empty" };
        }

        for( const auto& elm : jsArray ) {
            if( !elm.is_object() ) {
                throw std::runtime_error{ "array's element isn't an object" };
            }

            for( auto it = elm.begin(); it != elm.end(); ++it ) {
                const auto& jsValue = it.value();
                std::string value = jsValue.is_string() ? jsValue.get< std::string >() : std::string{};

                try {
                    jsonCheckString( it.key() );
                    jsonCheckString( value );
                }
                catch( ... ) {
                    throw std::runtime_error{ "key and/or value aren't strings" };
                }
            }
        }
    };

    /* check mandatory fields */
    jsonCheckString( gateway, NameFieldName );
    jsonCheckString( gateway, LabelFieldName );
    jsonCheckString( gateway, PluginIdFieldName );
    jsonCheckString( gateway, StatusFieldName );
    jsonCheckString( gateway, ManualDeviceAddingFieldName );
    jsonCheckBool( gateway, ReadyFieldName );
    jsonCheckInt( gateway, SettingsFieldName );

    /* check optional fields */
    int optionalFieldsCounter = 0;

    if( gateway.count( "operations" ) ) {
        ++optionalFieldsCounter;
    }

    if( gateway.count( InfoFieldName ) ) {
        if( !gateway[ InfoFieldName ].is_object() ) {
            throw std::runtime_error{ "info field isn't an object" };
        }
        ++optionalFieldsCounter;
    }

    auto jsonCheckOptionalStringField = [&]( const char* fieldName ) {
        if( gateway.count( fieldName ) ) {
            jsonCheckString( gateway, fieldName, JsonStringChecks::RESTRICTED_LENGTH );
            ++optionalFieldsCounter;
        }
    };

    jsonCheckOptionalStringField( IdFieldName );
    jsonCheckOptionalStringField( SetItemValueCommandFieldName );
    jsonCheckOptionalStringField( SetItemValueResponsePolicyFieldName );
    jsonCheckOptionalStringField( AddItemDictionaryValueCommandFieldName );
    jsonCheckOptionalStringField( SetItemDictionaryValueCommandFieldName );
    jsonCheckOptionalStringField( RemoveItemDictionaryValueCommandFieldName );
    jsonCheckOptionalStringField( ClearItemDictionaryCommandFieldName );
    jsonCheckOptionalStringField( SetGatewaySettingValueCommandFieldName );
    jsonCheckOptionalStringField( SetSettingValueCommandFieldName );
    jsonCheckOptionalStringField( SetSettingDictionaryValueCommandFieldName );
    jsonCheckOptionalStringField( ResetSettingCommandFieldName );
    jsonCheckOptionalStringField( ForceRemoveDeviceCommandFieldName );
    jsonCheckOptionalStringField( UpdateDeviceFirmwareCommandFieldName );
    jsonCheckOptionalStringField( CreateAbstractSubscriptionCommandFieldName );
    jsonCheckOptionalStringField( UpdateAbstractSubscriptionCommandFieldName );
    jsonCheckOptionalStringField( DeleteAbstractSubscriptionCommandFieldName );
    jsonCheckOptionalStringField( DispatchAbstractSubscriptionCommandFieldName );

    // these fields were made optional
    jsonCheckOptionalStringField( ReasonFieldName );
    if( gateway.count( UnreachableReasonsFieldName ) && !gateway.at( UnreachableReasonsFieldName ).empty() ) {
        jsonCheckArray( gateway, UnreachableReasonsFieldName, jsonCheckStringStringArray );
    }
    if( gateway.count( UnreachableActionsFieldName ) && !gateway.at( UnreachableActionsFieldName ).empty() ) {
        jsonCheckArray( gateway, UnreachableActionsFieldName, jsonCheckStringStringArray );
    }

    /* check if some 'trash' got into the json */
    if( gateway.size() > m_minFieldsAmount + optionalFieldsCounter ) {
        throw std::runtime_error{ "needless fields" };
    }
}

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
