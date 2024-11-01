#pragma once

#include "zigbee/zigbee_addon_callback.h"
#include "zigbee/backup/zigbee_network_info.h"

namespace HubPlatform {
namespace Zigbee {
namespace Backup {

struct ZigbeeNetworkInfo;

class BackupManager {
public:
    BackupManager( ZigbeeAddonCallbackInterface& callbackInterface );

    void collectNetworkInfo();
    void restoreNetwork( const ZigbeeNetworkInfo& info );

    bool isBackupRestoreInProgress();

    void onNetworkDown();

private:
    bool collectCurrentNodeInformation( ZigbeeNetworkInfo& info );
    void collectCurrentSecurityState( ZigbeeNetworkInfo& info );

private:
    ZigbeeAddonCallbackInterface& m_callbackInterface;
    std::unique_ptr< ZigbeeNetworkInfo > m_infoToRestore;
};

}  // namespace Backup
}  // namespace Zigbee
}  // namespace HubPlatform
