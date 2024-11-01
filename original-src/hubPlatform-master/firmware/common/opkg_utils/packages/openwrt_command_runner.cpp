#include "opkg_utils/packages/openwrt_command_runner.h"

#include "opkg_utils/packages/openwrt_utils.h"

#include "system_utils/filesystem.h"
#include "system_utils/system.h"

#include <fstream>
#include <unistd.h>

namespace HubPlatform {
namespace Opkg {

namespace {

constexpr const char OpkgListInstalledCommand[] = "opkg list-installed";
constexpr const char OpkgListUpgradableCommand[] = "opkg list-upgradable";
constexpr const char OpkgListAvailableCommand[] = "opkg list";
constexpr const char OpkgListDependantCommandPrefix[] = "opkg whatdepends ";

constexpr const char OpkgUpdateAvailableCommand[] = "opkg update ";
constexpr const char OpkgUpdateNoActionParameter[] = "--noaction ";
constexpr const char OpkgUpdateDownloadOnlyParameter[] = "--download-only ";
constexpr const char OpkgUpgradePackageCommandPrefix[] = "opkg upgrade --force-overwrite --force-space ";
constexpr const char OpkgInstallPackageCommandPrefix[] = "opkg install --force-space ";
constexpr const char OpkgDownloadPackagesCommandPrefix[] = "opkg install --force-space --download-only ";
constexpr const char OpkgRemovePackageCommandPrefix[] = "opkg remove --force-depends ";
constexpr const char OpkgCacheParameter[] = "--cache ";
constexpr const char OpkgDownloadPackageCommandPrefix[] = "opkg download ";

}  // namespace

OpenwrtCommandRunner::StdOutWithReturnCode OpenwrtCommandRunner::listInstalled() const {
    return System::getCommandOutput( OpkgListInstalledCommand, true /*catchStderr*/ );
}

OpenwrtCommandRunner::StdOutWithReturnCode OpenwrtCommandRunner::listUpgradable() const {
    return System::getCommandOutput( OpkgListUpgradableCommand, true /*catchStderr*/ );
}

OpenwrtCommandRunner::StdOutWithReturnCode OpenwrtCommandRunner::listAvailable() const {
    return System::getCommandOutput( OpkgListAvailableCommand, true /*catchStderr*/ );
}

OpenwrtCommandRunner::StdOutWithReturnCode OpenwrtCommandRunner::listDependant( const std::string& packageName ) const {
    std::string command = OpkgListDependantCommandPrefix + packageName;
    return System::getCommandOutput( command.c_str(), true /*catchStderr*/ );
}

OpenwrtCommandRunner::StdOutWithReturnCode OpenwrtCommandRunner::updateAvailable() {
    return System::getCommandOutput( OpkgUpdateAvailableCommand, true /*catchStderr*/ );
}

OpenwrtCommandRunner::StdOutWithReturnCode OpenwrtCommandRunner::updateDownloadOnlyAvailable() {
    std::string opkgCommand = std::string{ OpkgUpdateAvailableCommand } + OpkgUpdateDownloadOnlyParameter;
    return System::getCommandOutput( opkgCommand.c_str(), true /*catchStderr*/ );
}

OpenwrtCommandRunner::StdOutWithReturnCode OpenwrtCommandRunner::updateNoActionAvailable() {
    std::string opkgCommand = std::string{ OpkgUpdateAvailableCommand } + OpkgUpdateNoActionParameter;
    return System::getCommandOutput( opkgCommand.c_str(), true /*catchStderr*/ );
}

OpenwrtCommandRunner::StdOutWithReturnCode OpenwrtCommandRunner::sysupgrade( const std::string& imageUrl,
                                                                             const std::string& cacheDirectory ) {
    std::string imagePath = "/tmp/sysupgrade.img";
    std::string checksumPath = "/tmp/sysupgrade.img.md5";
    if( !cacheDirectory.empty() ) {
        filesystem::createFolder( cacheDirectory );
        imagePath = cacheDirectory + "/sysupgrade.img";
        checksumPath = cacheDirectory + "/sysupgrade.img.md5";
    }
    if( cacheDirectory.empty() || !filesystem::isFileExist( imagePath ) ) {
        using namespace std::placeholders;
        OpenwrtUtils::Downloader downloader = std::bind( download, _2, _1 );
        auto result = OpenwrtUtils::downloadImage( downloader, imageUrl, cacheDirectory );
        if( result.second != 0 )
            return result;
    }
    else if( !filesystem::isFileExist( checksumPath ) ) {
        return { "Image MD5 checksum not found", 1 };
    }
    else {
        auto result = OpenwrtUtils::verifyMD5Checksum( checksumPath );
        if( result.second != 0 )
            return result;
    }

    return System::getCommandOutput( ( "sysupgrade " + imagePath ).c_str(), true /*catchStderr*/ );
}

OpenwrtCommandRunner::StdOutWithReturnCode OpenwrtCommandRunner::upgradePackage( const std::string& packageName,
                                                                                 const std::string& cacheDirectory ) {
    std::string command = OpkgUpgradePackageCommandPrefix;
    if( !cacheDirectory.empty() ) {
        command += OpkgCacheParameter + cacheDirectory + " ";
    }
    command += packageName;
    auto result = System::getCommandOutput( command.c_str(), true /*catchStderr*/ );
    sync();
    return result;
}

OpenwrtCommandRunner::StdOutWithReturnCode OpenwrtCommandRunner::installPackage( const std::string& packageName,
                                                                                 const std::string& cacheDirectory ) {
    std::string command = OpkgInstallPackageCommandPrefix;
    if( !cacheDirectory.empty() ) {
        command += OpkgCacheParameter + cacheDirectory + " ";
    }
    command += packageName;
    auto result = System::getCommandOutput( command.c_str(), true /*catchStderr*/ );
    sync();
    return result;
}

OpenwrtCommandRunner::StdOutWithReturnCode OpenwrtCommandRunner::downloadImage( const std::string& imageUrl,
                                                                                const std::string& cacheDirectory ) {
    if( cacheDirectory.empty() ) {
        return { "\n", 0 };
    }

    using namespace std::placeholders;
    OpenwrtUtils::Downloader downloader = std::bind( download, _2, _1 );
    return OpenwrtUtils::downloadImage( downloader, imageUrl, cacheDirectory );
}

OpenwrtCommandRunner::StdOutWithReturnCode OpenwrtCommandRunner::downloadPackage( const std::string& packageName,
                                                                                  const std::string& cacheDirectory,
                                                                                  bool withDependencies ) {
    if( cacheDirectory.empty() ) {
        return { "\n", 0 };
    }
    if( withDependencies ) {
        std::string command = OpkgDownloadPackagesCommandPrefix;
        command += OpkgCacheParameter + cacheDirectory + " " + packageName;
        return System::getCommandOutput( command.c_str(), true /*catchStderr*/ );
    }
    else {
        std::string command = "cd " + cacheDirectory + " && " + OpkgDownloadPackageCommandPrefix + " " + packageName;
        return System::getCommandOutput( command.c_str(), true /*catchStderr*/ );
    }
}

OpenwrtCommandRunner::StdOutWithReturnCode OpenwrtCommandRunner::removePackage( const std::string& packageName ) {
    std::string command = OpkgRemovePackageCommandPrefix + packageName;
    auto result = System::getCommandOutput( command.c_str(), true /*catchStderr*/ );
    sync();
    return result;
}

OpenwrtCommandRunner::StdOutWithReturnCode OpenwrtCommandRunner::download( const std::string& outputPath,
                                                                           const std::string& imageUrl ) {
    const std::string downloadCommand = "curl --fail " + imageUrl + " -o " + outputPath;
    return System::getCommandOutput( downloadCommand.c_str(), true /*catchStderr*/ );
}

}  // namespace Opkg
}  // namespace HubPlatform
