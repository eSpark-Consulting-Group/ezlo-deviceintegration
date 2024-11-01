#pragma once

#include "update_manager/common/um_hub_reset_type.h"

namespace HubPlatform {
namespace HardwareManager {
class RemoteInterface;
}
}  // namespace HubPlatform

namespace HubPlatform {
namespace UpdateManager {

class HubResetter final {
public:
    HubResetter( HardwareManager::RemoteInterface& hardwareManager );

    void reset( HubResetType resetType = HubResetType::Factory );
    void switchToLuup();

private:
    void handleSoftReset();
    void handleFactoryReset();

private:
    HardwareManager::RemoteInterface& m_hardwareManager;
};

}  // namespace UpdateManager
}  // namespace HubPlatform
