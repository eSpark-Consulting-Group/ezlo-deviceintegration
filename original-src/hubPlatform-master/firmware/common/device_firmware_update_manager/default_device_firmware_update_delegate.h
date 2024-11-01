#pragma once

#include "device_firmware_update_manager/device_firmware_update_manager.h"

namespace HubPlatform {

class DefaultDeviceFirmwareUpdateDelegate : public DeviceFirmwareUpdateManager::Delegate {
public:
    DeviceFirmwareUpdateManager::DownloaderConstUPtr createFirmwareDownloader(
        const DeviceFirmwareUpdateManager::ExtendedLink& link,
        const DeviceFirmwareUpdateManager::Path& destinationPath,
        DeviceFirmwareUpdateManager::Downloader::ResultCallback resultCallback ) final;

    DeviceFirmwareUpdateManager::Path getDestinationForLink( const DeviceFirmwareUpdateManager::Link& link ) final;
};

}  // namespace HubPlatform
