#pragma once

#include <stdexcept>

namespace HubPlatform {
namespace CoreD {

class ItemNotFoundError : public std::runtime_error {
public:
    explicit ItemNotFoundError( const std::string& itemId )
    : runtime_error{ "Item not found by id: " + itemId } {}
};

class DeviceSettingNotFoundError : public std::runtime_error {
public:
    explicit DeviceSettingNotFoundError( const std::string& settingId )
    : runtime_error{ "Device setting not found by id: " + settingId } {}
};

class DeviceSettingReadOnlyError : public std::runtime_error {
public:
    explicit DeviceSettingReadOnlyError( const std::string& settingId )
    : runtime_error{ "Device setting is ready only, id: " + settingId } {}
};

class DeviceNotFoundError : public std::runtime_error {
public:
    explicit DeviceNotFoundError( const std::string& deviceId )
    : runtime_error{ "Device not found by id: " + deviceId } {}
};

class PolicyNotFoundError : public std::runtime_error {
public:
    explicit PolicyNotFoundError( const std::string& policyId )
    : runtime_error{ "Policy not found by id: " + policyId } {}
};

class PersistentDeviceForceRemoveDeniedError : public std::runtime_error {
public:
    explicit PersistentDeviceForceRemoveDeniedError( const std::string& deviceId )
    : runtime_error{ "Persistent device cannot be removed: " + deviceId } {}
};

class GatewayNotFoundError : public std::runtime_error {
public:
    explicit GatewayNotFoundError( const std::string& gatewayId )
    : runtime_error{ "Gateway not found by id: " + gatewayId } {}
};

class GatewaySettingNotFoundError : public std::runtime_error {
public:
    explicit GatewaySettingNotFoundError( const std::string& settingId )
    : runtime_error{ "Gateway setting not found by id: " + settingId } {}
};

class GatewaySettingReadOnlyError : public std::runtime_error {
public:
    explicit GatewaySettingReadOnlyError( const std::string& settingId )
    : runtime_error{ "Gateway setting is ready only, id: " + settingId } {}
};

class ItemHasNoMinMaxFieldsError : public std::runtime_error {
public:
    explicit ItemHasNoMinMaxFieldsError( const std::string& itemId )
    : runtime_error{ "Item associated with this id (" + itemId + ") has no minimal and maximal value fields" } {}
};

class ValueIsNotInValidRangeError : public std::runtime_error {
public:
    explicit ValueIsNotInValidRangeError( const std::string& itemId, const std::string& valueName, int min, int max )
    : runtime_error{ "Value '" + valueName + "' is not in a valid range. Valid range is: [" + std::to_string( min ) +
                     ", " + std::to_string( max ) + "]. Item id: " + itemId } {}
};

class InvalidValueTypeError : public std::runtime_error {
public:
    explicit InvalidValueTypeError( const std::string& itemId,
                                    const std::string& valueName,
                                    const std::string& unsupportedTypeName )
    : runtime_error{ "Value '" + valueName + "' has invalid type" + ". Type `" + unsupportedTypeName +
                     "' is not supported. Item id: " + itemId } {}
};

class InvalidValueSourceTypeError : public std::runtime_error {
public:
    InvalidValueSourceTypeError()
    : runtime_error{ "Value source type is empty" } {}
    explicit InvalidValueSourceTypeError( const std::string& valueSource )
    : runtime_error{ "Value source has invalid type: " + valueSource } {}
};

class DeviceBusy : public std::runtime_error {
public:
    explicit DeviceBusy( const std::string& deviceId )
    : runtime_error{ "Device " + deviceId + " is busy" } {}
};

class EmptyParameter : public std::runtime_error {
public:
    explicit EmptyParameter( const std::string& argName )
    : runtime_error{ "Empty argument: " + argName } {}
};

}  // namespace CoreD
}  // namespace HubPlatform
