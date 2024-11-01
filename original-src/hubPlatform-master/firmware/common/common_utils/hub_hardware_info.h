#pragma once

#include <string>

namespace HubHardwareInfo {
namespace Revisions {
extern const char* const E550_REV_3_0;
extern const char* const E550_REV_4_0;
}  // namespace Revisions

class HubHardwareInfo {
public:
    enum class ModuleBatteryGauge { Hardware, Software, NotPresent };
    enum class Module2GIG { Present, NotPresent };

    HubHardwareInfo( std::string hubType, std::string hardwareRevision );

    ModuleBatteryGauge getBatteryGaugeInfo();
    Module2GIG get2GIGInfo();

private:
    const std::string m_hubType;
    const std::string m_hardwareRevision;
};

}  // namespace HubHardwareInfo
