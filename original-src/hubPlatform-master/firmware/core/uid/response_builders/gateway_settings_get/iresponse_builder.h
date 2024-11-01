#pragma once

#include "cored/model/setting/synchronization_status.h"

#include "json_utils/json_wrapper.h"

namespace HubPlatform {
namespace CoreD {
namespace Model {

class Value;
class ValueType;
class TextElement;
class SettingEnum;

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform

namespace HubPlatform {
namespace Uid {
namespace ResponseBuilder {
namespace GatewaySettingsGet {

class IResponseBuilder {
public:
    virtual ~IResponseBuilder() = default;

    virtual void addGatewaySetting( const std::string& id,
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
                                    const bool* const hasSetter ) = 0;

    virtual JsonObject buildResponse() const = 0;
};

}  // namespace GatewaySettingsGet
}  // namespace ResponseBuilder
}  // namespace Uid
}  // namespace HubPlatform
