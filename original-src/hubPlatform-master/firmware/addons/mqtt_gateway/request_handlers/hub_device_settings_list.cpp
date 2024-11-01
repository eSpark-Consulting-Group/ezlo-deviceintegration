#include "mqtt_gateway/request_handlers/hub_device_settings_list.h"

#include "mqtt_gateway/utils/utils.h"
#include "mqtt_gateway/mqtt_gateway_exception.h"

#include "service_interfaces/service.h"
#include "json_utils/json_utils.h"
#include "common_utils/exception_utils.h"

#include "cored/remote_interface/cored_interface_factory.h"
#include "cored/model/resources/cored_resources.h"
#include "cored/model/cored_device_setting.h"
#include "cored/model/visitors/ui_value_writer_visitor.h"
#include "cored/model/setting/text_element.h"

#include <algorithm>

namespace HubPlatform {
namespace MQTTGateway {
namespace RequestHandlers {

HubDeviceSettingsList::HubDeviceSettingsList( Services::Service& service )
: m_service( service ){};

Json HubDeviceSettingsList::process( Json& params ) {
    // NOTE: content of this function is taken from ha-uid
    // UI handlers must be moved from ha-uid to separate lib and reused here

    try {
        auto settingIds = json_get_vec< std::string >( params, "ids" );
        auto deviceIds = json_get_vec< std::string >( params, "deviceIds" );

        std::remove_if( settingIds.begin(), settingIds.end(), []( std::string& s ) { return s.empty(); } );
        std::remove_if( deviceIds.begin(), deviceIds.end(), []( std::string& s ) { return s.empty(); } );

        auto dbQuery = JSON_EMPTY_OBJ;
        if( !settingIds.empty() ) {
            dbQuery[ CoreD::Model::Resources::DeviceSettingFields::Id ] = JSON( "$in" << settingIds );
        }

        if( !deviceIds.empty() ) {
            dbQuery[ CoreD::Model::Resources::DeviceSettingFields::DeviceId ] = JSON( "$in" << deviceIds );
        }

        auto core = HubPlatform::CoreDRemoteInterface::makeInstance( m_service );
        auto settings = core->getDeviceSettingsBy( dbQuery );

        auto setTextElement = []( JsonObject& setting, const char* key, const CoreD::Model::TextElement& textElement ) {
            if( textElement.isValid() ) {
                setting[ key ] = textElement.toJson();
            }
        };

        auto setValue =
            []( const char* key, const CoreD::Model::Value* value, CoreD::Model::UiValueWriterVisitor& writer ) {
                if( value ) {
                    writer.writeValue( *value, key );
                }
            };

        auto& coreConfiguration = core->getConfiguration();

        auto result = Json::array();
        for( auto& element : settings ) {
            auto& settingId = element.first;
            auto& setting = element.second;

            JsonObject joSetting = Json::object();

            joSetting[ "_id" ] = settingId;
            joSetting[ "deviceId" ] = setting.getDeviceId();
            setTextElement( joSetting, "label", setting.getLabel() );
            setTextElement( joSetting, "description", setting.getDescription() );

            CoreD::Model::UiValueWriterVisitor writerVisitor{ joSetting, coreConfiguration };
            setValue( "value", &setting.getValue(), writerVisitor );
            setValue( "valueDefault", setting.getDefaultValue(), writerVisitor );
            setValue( "valueMin", setting.getLowerLimitValue(), writerVisitor );
            setValue( "valueMax", setting.getUpperLimitValue(), writerVisitor );
            setValue( "valueStep", setting.getValueStep(), writerVisitor );

            if( !setting.getValueType().isDictionary() )
                joSetting[ "status" ] =
                    CoreD::Model::SynchronizationStatus::toString( setting.getSynchronizationStatus() );

            if( setting.getEnum().isValid() ) {
                joSetting[ "enum" ] = setting.getEnum().toJson();
            }

            if( !setting.getInfo().empty() ) {
                joSetting[ "info" ] = setting.getInfo();
            }

            if( setting.getElementsMaxNumber() != 0 ) {
                joSetting[ "elementsMaxNumber" ] = setting.getElementsMaxNumber();
            }

            if( setting.getHasSetterValue() ) {
                joSetting[ "hasSetter" ] = *setting.getHasSetterValue();
            }

            if( setting.getTargetChannelSupportedValue() ) {
                joSetting[ "targetChannelSupported" ] = *setting.getTargetChannelSupportedValue();
            }

            result.push_back( std::move( joSetting ) );
        }

        return JSON( "settings" << result );
    }
    catch( const std::exception& e ) {
        auto error = "Failed to get device seetings: " + CommonUtils::whatError( e );
        LOGGER_ERROR_RAW( cc::warn( error ) );
        return MQTTGateway::Utils::buildErrorResponseBody( Errors::INTERNAL_OPERATION_ERROR, error );
    }
}

}  // namespace RequestHandlers
}  // namespace MQTTGateway
}  // namespace HubPlatform
