#pragma once

#include "update_manager/common/um_update_operation_type.h"
#include "update_manager/common/um_update_plan.h"
#include "update_manager/common/um_hub_reset_type.h"

#include "common_utils/addons/addon_summary.h"
#include "common_utils/hub_state.h"
#include "base_utils/firmware_version.h"
#include "provision_data_utils/provision_data.h"

namespace HubPlatform {
namespace Services {
class Service;
}
namespace UpdateManager {

class RemoteInterface final {
public:
    explicit RemoteInterface( Services::Service& hostService );

    CommonUtils::HubState requestSoftwareInfo();

    void requestUpdatePlan( UpdateOperation operation, const std::string& operationId );
    void executeUpdatePlan( UpdatePlan plan, const std::string& operationId );

    void executeLuupSwitch();
    void executeBootloaderUpdate();
    void executeRecoveryPartitionUpdate();

    Json getCurrentUpdateState() const;

public:
    void executeHubReset( HubResetType resetType = HubResetType::Factory );

public:
    bool isPluginUnavailable( const std::string& pluginName ) const;

private:
    Services::Service& m_hostService;
};

}  // namespace UpdateManager
}  // namespace HubPlatform
