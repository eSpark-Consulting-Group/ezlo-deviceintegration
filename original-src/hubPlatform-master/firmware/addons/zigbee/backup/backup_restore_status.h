#pragma once

namespace HubPlatform {
namespace Zigbee {
namespace Backup {

enum class BackupRestoreStatus {
    Finished = 0,
    Timeout = 128,
    CannotImportTcData,
    FailedToInitializedNetworkSecurityState,
    FailedToSetEuid64,
    FailedToSetNetworkFrameCounter,
    FailedToSetGlobalFrameCounter,
    FailedToFormNetwork
};

}  // namespace Backup
}  // namespace Zigbee
}  // namespace HubPlatform
