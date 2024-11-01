#pragma once

#include "json_utils/json_fwd.h"

#include <string>

namespace HubPlatform {

class PlatformConfiguration final {
public:
    using ProgramPath = std::string;
    using DeviceName = std::string;

public:
    static const PlatformConfiguration& getPlatformConfiguration();

    DeviceName getZwaveDeviceName() const;
    DeviceName getZigBeeDeviceName() const;
    DeviceName getUserPartitionDeviceName() const;
    ProgramPath getBootloaderUpdaterPath() const;
    ProgramPath getRecoveryUpdaterPath() const;

private:
    PlatformConfiguration();

private:
    Json m_platformConfiguration;
};

}  // namespace HubPlatform
