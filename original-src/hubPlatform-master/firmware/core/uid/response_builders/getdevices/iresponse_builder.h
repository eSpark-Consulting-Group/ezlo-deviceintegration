#pragma once

#include "json_utils/json_wrapper.h"
#include "device_model/h/device_additional_info.h"
#include "device_model/h/firmware_info.h"

#include <vector>

namespace HubPlatform {
namespace Uid {
namespace ResponseBuilder {
namespace GetDevices {

enum class ArmedState {
    NoState,
    Disarmed,
    Armed,
};

class IResponseBuilder {
public:
    virtual ~IResponseBuilder() = default;

    virtual void addDevice( const std::string& id,
                            const std::string& deviceTypeId,
                            const std::string& gatewayId,
                            const std::string& name,
                            const std::string& type,
                            const std::string& roomId,
                            const std::string& category,
                            const std::string& subcategory,
                            const std::string& parentDeviceId,
                            const std::string& security,
                            const std::string& status,
                            const std::vector< DeviceAdditionalInfo >& additionalInfo,
                            const std::vector< FirmwareInfo >& firmware,
                            bool batteryPowered,
                            bool reachable,
                            bool ready,
                            ArmedState armed,
                            bool persistent,
                            bool parentRoom,
                            const std::string& protectConfig ) = 0;

    virtual JsonObject buildResponse() const = 0;
};

}  // namespace GetDevices

namespace DataListDevices {

class IResponseBuilder : public GetDevices::IResponseBuilder {
public:
};

}  // namespace DataListDevices
}  // namespace ResponseBuilder
}  // namespace Uid
}  // namespace HubPlatform
