#pragma once

#include "json_utils/json_wrapper.h"
#include "cored/model/setting/synchronization_status.h"
#include "base_utils/optional.h"

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
namespace DeviceSettingsGet {

class IResponseBuilder {
public:
    virtual ~IResponseBuilder() = default;

    virtual void addDeviceSetting( const std::string& id,
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
                                   common::optional< bool > targetChannelSupported ) = 0;

    virtual JsonObject buildResponse() const = 0;
};

}  // namespace DeviceSettingsGet
}  // namespace ResponseBuilder
}  // namespace Uid
}  // namespace HubPlatform
