#include "cored/db_modifier/gateway_setting_db_modifier.h"
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

namespace Fields = Model::Resources::GatewaySettingFields;

GatewaySettingDbModifier::GatewaySettingDbModifier( Services::Service& service, Broadcaster broadcaster )
: DbModifier(
      service.getDBClient(),
      std::move( broadcaster ),
      Model::Resources::DBNames::GatewaysSettingsDB,
      { { HUB_GATEWAY_SETTING_ADDED, {} }, { HUB_GATEWAY_SETTING_UPDATED, {} }, { HUB_GATEWAY_SETTING_REMOVED, {} } },
      {} )
, m_service( service ) {}

void GatewaySettingDbModifier::insert( const Json& record ) {
    incrementGatewaySettings( json_get_str( record, Fields::GatewayId ) );
    DbModifier::insert( record );
}

void GatewaySettingDbModifier::remove( const Id& recordId ) {
    decrementGatewaySettings( recordId );
    DbModifier::remove( recordId );
}

Json& GatewaySettingDbModifier::prepareInsertBroadcastMessage( Json& broadcastData ) {
    return formatBroadcastValues( broadcastData );
}

Json& GatewaySettingDbModifier::prepareRemoveBroadcastMessage( Json& broadcastData, const Json& /*removedRecord*/ ) {
    return broadcastData;
}

void GatewaySettingDbModifier::formatValue( Json& broadcastData,
                                            const char* key,
                                            const Model::ValueType& valueType,
                                            Model::UiValueWriterVisitor& writer ) {
    auto it = broadcastData.find( key );
    if( it != broadcastData.end() ) {
        auto value = Model::ValueFactory::createItemValue( valueType, *it );
        writer.writeValue( *value, key );
    }
}

Json& GatewaySettingDbModifier::formatBroadcastValues( Json& broadcastData ) {
    auto config = Model::ConfigurationFactory::createConfiguration( m_service.getDBClient() );
    Model::UiValueWriterVisitor valueWriter( broadcastData, *config );

    const auto valueType = Model::ValueType::fromString( json_get_str( broadcastData, Fields::ValueType ) );

    formatValue( broadcastData, Fields::Value, valueType, valueWriter );
    formatValue( broadcastData, Fields::ValueDefault, valueType, valueWriter );
    formatValue( broadcastData, Fields::ValueMin, valueType, valueWriter );
    formatValue( broadcastData, Fields::ValueMax, valueType, valueWriter );

    return broadcastData;
}

void GatewaySettingDbModifier::incrementGatewaySettings( const std::string& gatewayId ) {
    using namespace Model::Resources;

    auto gateway = m_db.findOne( DBNames::GatewaysDB, JSON( GatewayFields::IdFieldName << gatewayId ), nullptr );

    gateway[ GatewayFields::SettingsFieldName ] = json_get_int( gateway, GatewayFields::SettingsFieldName ) + 1;
    m_db.insert( DBNames::GatewaysDB, gateway );
}

void GatewaySettingDbModifier::decrementGatewaySettings( const Id& gatewaySettingId ) {
    using namespace Model::Resources;

    const auto gatewaySetting =
        m_db.findOne( DBNames::GatewaysSettingsDB, JSON( GatewaySettingFields::Id << gatewaySettingId ), nullptr );

    auto gateway = m_db.findOne(
        DBNames::GatewaysDB,
        JSON( GatewayFields::IdFieldName << json_get_str( gatewaySetting, GatewaySettingFields::GatewayId ) ),
        nullptr );

    gateway[ GatewayFields::SettingsFieldName ] = json_get_int( gateway, GatewayFields::SettingsFieldName ) - 1;
    m_db.insert( DBNames::GatewaysDB, gateway );
}

}  // namespace CoreD
}  // namespace HubPlatform
