#pragma once

#include <string>

#include "device_model/h/devicetype.h"

namespace fo {

class Device {
public:
    Device();
    Device( const std::string& name,
            const std::string& type,
            const std::string& subType,
            const DeviceType& deviceType,
            const std::string& vendor,
            const std::string& model,
            const std::string& description,
            const bool batteryPowered,
            const bool hasWakeup,
            const bool reachable = true,
            const bool ready = false,
            const bool complex = false,
            const bool hasSecurity = false,
            const bool secureChannelEstablished = false,
            const bool persistent = false );
    const std::string& getDbId() const;
    const std::string& getName() const;
    const std::string& getType() const;
    const std::string& getSubType() const;
    DeviceType getDeviceType() const;
    bool getBatteryPowered() const;
    bool getHasWakeup() const;
    bool getReachable() const;
    bool getReady() const;
    bool getComplex() const;
    bool getHasSecurity() const;
    bool getSecureChannelEstablished() const;
    bool getPersistent() const;

    const std::string& getVendor() const;
    const std::string& getModel() const;
    const std::string& getDescription() const;

protected:
    void setDeviceId( const std::string& deviceId );

private:
    std::string m_deviceId;
    std::string m_name;
    std::string m_type;
    std::string m_deviceSubType;
    DeviceType m_deviceType;
    std::string m_vendor;
    std::string m_model;
    std::string m_description;
    bool m_batteryPowered = false;
    bool m_hasWakeup = true;
    bool m_reachable = true;
    bool m_ready = false;
    bool m_complex = false;
    bool m_hasSecurity = false;
    bool m_secureChannelEstablished = false;
    bool m_persistent = false;

    friend class DeviceJson;
};

}  // namespace fo
