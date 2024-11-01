#pragma once

#include "uid/response_builders/device_settings_get/iresponse_builder.h"
#include "uid/response_builders/response_builder_base.h"

namespace HubPlatform {
namespace CoreD {
namespace Model {

class Configuration;
class UiValueWriterVisitor;

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform

namespace HubPlatform {
namespace Uid {
namespace ResponseBuilder {
namespace DeviceSettingsGet {
namespace V1 {

class ResponseBuilder : public ResponseBuilderBase, public IResponseBuilder {
public:
    ResponseBuilder( const rpc::Message& message, const std::string& api, CoreD::Model::Configuration& configuration );

    ~ResponseBuilder() override;

    void addDeviceSetting( const std::string& id,
                           const std::string& deviceId,
                           const CoreD::Model::TextElement& label,
                           const CoreD::Model::TextElement& description,
                           const CoreD::Model::ValueType& valueType,
                           const CoreD::Model::Value& value,
                           const CoreD::Model::Value* valueDefault,
                           const CoreD::Model::Value* valueMin,
                           const CoreD::Model::Value* valueMax,
                           const CoreD::Model::Value* valueStep,
                           const CoreD::Model::SettingEnum& settingEnum,
                           const Json& settingInfo,
                           std::size_t elementsMaxNumber,
                           CoreD::Model::SynchronizationStatus::Enum status,
                           common::optional< bool > hasSetter,
                           common::optional< bool > targetChannelSupported ) override;

    JsonObject buildResponse() const override;

private:
    void setTextElement( JsonObject& setting, const char* key, const CoreD::Model::TextElement& textElement );

    void setValue( const char* key, const CoreD::Model::Value* value, CoreD::Model::UiValueWriterVisitor& writer );

private:
    CoreD::Model::Configuration& m_configuration;
    std::vector< JsonObject > m_settings;
};

}  // namespace V1
}  // namespace DeviceSettingsGet
}  // namespace ResponseBuilder
}  // namespace Uid
}  // namespace HubPlatform
