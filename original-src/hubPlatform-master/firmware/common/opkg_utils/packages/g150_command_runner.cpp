#include "opkg_utils/packages/g150_command_runner.h"
#include "opkg_utils/feeds/g150_feeds_updater.h"

#include "system_utils/filesystem.h"
#include "system_utils/system.h"

namespace HubPlatform {
namespace Opkg {

OpenwrtCommandRunner::StdOutWithReturnCode G150CommandRunner::sysupgrade( const std::string& imageUrl,
                                                                          const std::string& cacheDirectory ) {
    auto baseUrl = G150FeedsUpdater::extractBaseUrlFromImageUrl( imageUrl );
    // auto recoveryImageUrl = G150FeedsUpdater::generateRecoveryImageUrl( baseUrl );
    // auto result = updateRecovery( recoveryImageUrl, cacheDirectory );
    // if( result.second != 0 )
    //     return result;
    return updateSystem( imageUrl, cacheDirectory );
}

OpenwrtCommandRunner::StdOutWithReturnCode G150CommandRunner::updateRecovery( const std::string& imageUrl,
                                                                              const std::string& cacheDirectory ) {
    std::string imagePath = "/tmp/recovery.bin";
    if( !cacheDirectory.empty() ) {
        filesystem::createFolder( cacheDirectory );
        imagePath = cacheDirectory + "/recovery.bin";
    }
    if( cacheDirectory.empty() || !filesystem::isFileExist( imagePath ) ) {
        auto result = download( imagePath, imageUrl );
        if( result.second != 0 )
            return result;
    }
    const std::string updateRecoveryCommand = "/opt/firmware/bin/update_recovery.sh " + imagePath;
    return System::getCommandOutput( updateRecoveryCommand.c_str(), true /*catchStderr*/ );
}

OpenwrtCommandRunner::StdOutWithReturnCode G150CommandRunner::updateSystem( const std::string& imageUrl,
                                                                            const std::string& cacheDirectory ) {
    std::string imagePath = "/tmp/sysupgrade.bin";
    if( !cacheDirectory.empty() ) {
        filesystem::createFolder( cacheDirectory );
        imagePath = cacheDirectory + "/sysupgrade.bin";
    }
    if( cacheDirectory.empty() || !filesystem::isFileExist( imagePath ) ) {
        auto result = download( imagePath, imageUrl );
        if( result.second != 0 )
            return result;
    }
    return System::getCommandOutput( ( "sysupgrade " + imagePath ).c_str(), true /*catchStderr*/ );
}

}  // namespace Opkg
}  // namespace HubPlatform
