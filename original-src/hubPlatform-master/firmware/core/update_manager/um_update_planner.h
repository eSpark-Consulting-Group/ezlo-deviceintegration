#pragma once

#include "update_manager/common/um_update_plan.h"

#include "opkg_utils/opkg_manager.h"
#include "base_utils/firmware_version.h"

namespace HubPlatform {
namespace UpdateManager {

class UpdatePlanner final {
public:
    UpdatePlanner( const Opkg::IPackagesManager& packagesManager,
                   const Opkg::IFeedsUpdater& feedsUpdater,
                   Firmware::Version firmwareVersion );

    std::unique_ptr< UpdatePlan > createUpdatePlan( UpdateOperation operation );

private:
    bool isHubPlatformPackage( const std::string& name );
    bool isAddonPackage( const std::string& name );
    bool isPluginPackage( const std::string& name );
    bool isCustomPackage( const std::string& name );

private:
    const Opkg::IPackagesManager& m_packagesManager;
    const Opkg::IFeedsUpdater& m_feedsUpdater;

    Firmware::Version m_firmwareVersion;
};

}  // namespace UpdateManager
}  // namespace HubPlatform
