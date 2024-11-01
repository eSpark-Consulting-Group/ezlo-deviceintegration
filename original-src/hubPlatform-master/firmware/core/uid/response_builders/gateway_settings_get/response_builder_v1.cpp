#include "uid/response_builders/gateway_settings_get/response_builder_v1.h"

#include "cored/model/setting/text_element.h"
#include "cored/model/setting/setting_enum.h"
#include "cored/model/values/value.h"
#include "cored/model/visitors/ui_value_writer_visitor.h"

namespace HubPlatform {
namespace Uid {
namespace ResponseBuilder {
namespace GatewaySettingsGet {
namespace V1 {

ResponseBuilder::ResponseBuilder( const rpc::Message& message,
                                  const std::string& api,
                                  CoreD::Model::Configuration& configuration )
: ResponseBuilderBase( message, api )
, m_configuration( configuration ) {}

void ResponseBuilder::addGatewaySetting( const std::string& id,
                                         const std::string& gatewayId,
                                         const CoreD::Model::TextElement& label,
                                         const CoreD::Model::TextElement& description,
                                         const CoreD::Model::ValueType& valueType,
                                         const CoreD::Model::Value& value,
                                         const CoreD::Model::Value* valueDefault,
                                         const CoreD::Model::Value* valueMin,
                                         const CoreD::Model::Value* valueMax,
                                         const CoreD::Model::SettingEnum& settingEnum,
                                         const Json& settingInfo,
                                         CoreD::Model::SynchronizationStatus::Enum status,
                                         const bool* hasSetter ) {
    JsonObject joSetting = Json::object();

    joSetting[ "_id" ] = id;
    joSetting[ "gatewayId" ] = gatewayId;
    setTextElement( joSetting, "label", label );
    setTextElement( joSetting, "description", description );

    CoreD::Model::UiValueWriterVisitor writerVisitor{ joSetting, m_configuration };
    setValue( "value", &value, writerVisitor );
    setValue( "valueDefault", valueDefault, writerVisitor );
    setValue( "valueMin", valueMin, writerVisitor );
    setValue( "valueMax", valueMax, writerVisitor );

    if( !valueType.isDictionary() )
        joSetting[ "status" ] = CoreD::Model::SynchronizationStatus::toString( status );

    if( settingEnum.isValid() ) {
        joSetting[ "enum" ] = settingEnum.toJson();
    }

    if( !settingInfo.empty() ) {
        joSetting[ "info" ] = settingInfo;
    }

    if( hasSetter ) {
        joSetting[ "hasSetter" ] = *hasSetter;
    }

    m_settings.push_back( std::move( joSetting ) );
}

JsonObject ResponseBuilder::buildResponse() const {
    auto joResponse = ResponseBuilderBase::buildResponse();
    joResponse[ "result" ][ "settings" ] = m_settings;
    return joResponse;
}

void ResponseBuilder::setTextElement( JsonObject& setting,
                                      const char* key,
                                      const CoreD::Model::TextElement& textElement ) {
    if( textElement.isValid() ) {
        setting[ key ] = textElement.toJson();
    }
}

void ResponseBuilder::setValue( const char* key,
                                const CoreD::Model::Value* value,
                                CoreD::Model::UiValueWriterVisitor& writer ) {
    if( value ) {
        writer.writeValue( *value, key );
    }
}

}  // namespace V1
}  // namespace GatewaySettingsGet
}  // namespace ResponseBuilder
}  // namespace Uid
}  // namespace HubPlatform
