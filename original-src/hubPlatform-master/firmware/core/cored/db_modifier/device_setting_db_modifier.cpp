#include "cored/db_modifier/device_setting_db_modifier.h"
#include "cored/model/resources/cored_resources.h"
#include "cored/model/values/value_factory.h"
#include "cored/model/values/value_type.h"
#include "cored/model/values/value.h"
#include "cored/model/visitors/ui_value_writer_visitor.h"
#include "cored/model/configuration/configuration_factory.h"
#include "cored/model/configuration/configuration.h"

#include "service_interfaces/service.h"

namespace HubPlatform {
namespace CoreD {

namespace Fields = Model::Resources::DeviceSettingFields;

DeviceSettingDbModifier::DeviceSettingDbModifier( Services::Service& service, Broadcaster broadcaster )
: DbModifier(
      service.getDBClient(),
      std::move( broadcaster ),
      Model::Resources::DBNames::DevicesSettingsDB,
      { { HUB_DEVICE_SETTING_ADDED, {} }, { HUB_DEVICE_SETTING_UPDATED, {} }, { HUB_DEVICE_SETTING_REMOVED, {} } },
      {} )
, m_service( service ) {}

DeviceSettingDbModifier::~DeviceSettingDbModifier() = default;

Json& DeviceSettingDbModifier::prepareInsertBroadcastMessage( Json& broadcastData ) {
    return formatBroadcastValues( broadcastData );
}

Json& DeviceSettingDbModifier::prepareUpdateBroadcastMessage( Json& broadcastData ) {
    using namespace HubPlatform::CoreD::Model::Resources;

    const auto settingId = json_get_str( broadcastData, DeviceSettingFields::Id );
    const auto setting =
        m_db.findOne( DBNames::DevicesSettingsDB, JSON( DeviceSettingFields::Id << settingId ), nullptr );

    broadcastData[ DeviceSettingFields::DeviceId ] = json_get_str( setting, DeviceSettingFields::DeviceId );

    auto valueTypeStr = json_get_str( setting, DeviceSettingFields::ValueType );
    auto valueType = Model::ValueType::fromString( valueTypeStr );
    if( !valueType.isContainer() ) {
        broadcastData[ DeviceSettingFields::ValueType ] = valueTypeStr;
        return formatBroadcastValues( broadcastData );
    }

    return broadcastData;
}

Json& DeviceSettingDbModifier::prepareRemoveBroadcastMessage( Json& broadcastData, const Json& /*removedRecord*/ ) {
    return broadcastData;
}

void DeviceSettingDbModifier::sendUpdateBroadcast( MessageSubClasses subClass,
                                                   Json& broadcast,
                                                   const Json& oldFieldsValues ) {
    using namespace Model::Resources;
    prepareUpdateBroadcastMessage( broadcast );
    if( !m_dictionary.empty() ) {
        broadcast.erase( DeviceSettingFields::Value );
        broadcast[ DictionaryEditRequestFields::OperationFieldName ] = m_dictionary.operation;
        broadcast[ DictionaryEditRequestFields::ElementFieldName ][ m_dictionary.id ]
                 [ DictionaryEditRequestFields::ValueFieldName ] = m_dictionary.value;
        DbModifier::sendUpdateBroadcast( HUB_DEVICE_SETTING_DICTIONARY_UPDATED, broadcast, oldFieldsValues );
    }
    else {
        DbModifier::sendUpdateBroadcast( subClass, broadcast, oldFieldsValues );
    }
}

void DeviceSettingDbModifier::updateDictionary( const IDbModifier::Id& recordId,
                                                const Json& updatePortion,
                                                const Dictionary& dictionary ) {
    m_dictionary = dictionary;
    update( recordId, updatePortion );
    m_dictionary.clear();
}

void DeviceSettingDbModifier::formatValue( Json& broadcastData,
                                           const char* key,
                                           const Model::ValueType& valueType,
                                           Model::UiValueWriterVisitor& writer ) {
    auto it = broadcastData.find( key );
    if( it != broadcastData.end() ) {
        auto value = Model::ValueFactory::createItemValue( valueType, *it );
        writer.writeValue( *value, key );
    }
}

Json& DeviceSettingDbModifier::formatBroadcastValues( Json& broadcastData ) {
    auto config = Model::ConfigurationFactory::createConfiguration( m_service.getDBClient() );
    Model::UiValueWriterVisitor valueWriter( broadcastData, *config );

    const auto valueType = Model::ValueType::fromString( broadcastData[ Fields::ValueType ] );

    formatValue( broadcastData, Fields::Value, valueType, valueWriter );
    formatValue( broadcastData, Fields::ValueDefault, valueType, valueWriter );
    formatValue( broadcastData, Fields::ValueMin, valueType, valueWriter );
    formatValue( broadcastData, Fields::ValueMax, valueType, valueWriter );

    return broadcastData;
}

}  // namespace CoreD
}  // namespace HubPlatform
