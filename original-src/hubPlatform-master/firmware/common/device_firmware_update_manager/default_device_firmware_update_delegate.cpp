#include "device_firmware_update_manager/default_device_firmware_update_delegate.h"
#include "device_firmware_update_manager/private/default_downloader.h"

#include "common_utils/cpp14.h"

#include <unistd.h>

namespace HubPlatform {

DeviceFirmwareUpdateManager::DownloaderConstUPtr DefaultDeviceFirmwareUpdateDelegate::createFirmwareDownloader(
    const DeviceFirmwareUpdateManager::ExtendedLink& link,
    const DeviceFirmwareUpdateManager::Path& destinationPath,
    DeviceFirmwareUpdateManager::Downloader::ResultCallback resultCallback ) {
    try {
        return std::make_unique< DefaultDownloader >( link, destinationPath, std::move( resultCallback ) );
    }
    catch( const std::exception& ) {
        return {};
    }
}

DeviceFirmwareUpdateManager::Path DefaultDeviceFirmwareUpdateDelegate::getDestinationForLink(
    const DeviceFirmwareUpdateManager::Link& link ) {
    char fileName[] = "/tmp/fw.XXXXXX";
    close( mkstemp( fileName ) );
    return fileName;
}

}  // namespace HubPlatform
