#pragma once

#include "opkg_utils/packages/command_runner.h"
#include "opkg_utils/packages/ipackages_manager.h"

namespace HubPlatform {
namespace Opkg {

class PackagesManager final : public IPackagesManager {
public:
    explicit PackagesManager( CommandRunner& commandRunner );

    PackageSummaries getInstalledPackages() const override;
    UpgradablePackageSummaries getUpgradablePackages() const override;
    PackageSummaries getAvailablePackages() const override;
    PackageSummaries getDependantPackages( const std::string& packageName ) const override;

    StdOutWithReturnCode downloadImage( const std::string& imageUrl, const std::string& cacheDirectory ) override;
    StdOutWithReturnCode downloadPackage( const std::string& packageName,
                                          const std::string& cacheDirectory,
                                          bool withDependencies ) override;

    StdOutWithReturnCode updateAvailable() override;
    StdOutWithReturnCode checkAvailable() override;

    StdOutWithReturnCode sysupgrade( const std::string& imageUrl, const std::string& cacheDirectory = "" ) override;
    StdOutWithReturnCode upgradePackage( const std::string& packageName,
                                         const std::string& cacheDirectory = "" ) override;
    StdOutWithReturnCode installPackage( const std::string& packageName,
                                         const std::string& cacheDirectory = "" ) override;
    StdOutWithReturnCode removePackage( const std::string& packageName ) override;

private:
    CommandRunner& m_commandRunner;
};

}  // namespace Opkg
}  // namespace HubPlatform
