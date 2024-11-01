#pragma once

#include "opkg_utils/packages/package_summary.h"
#include "opkg_utils/packages/upgradable_package_summary.h"

#include <vector>

namespace HubPlatform {
namespace Opkg {

class IPackagesManager {
public:
    virtual ~IPackagesManager() = default;

    using PackageSummaries = std::vector< PackageSummary >;
    using UpgradablePackageSummaries = std::vector< UpgradablePackageSummary >;

    virtual PackageSummaries getInstalledPackages() const = 0;
    virtual UpgradablePackageSummaries getUpgradablePackages() const = 0;
    virtual PackageSummaries getAvailablePackages() const = 0;
    virtual PackageSummaries getDependantPackages( const std::string& packageName ) const = 0;

    using StdOutWithReturnCode = std::pair< std::string, int >;

    virtual StdOutWithReturnCode downloadImage( const std::string& imageUrl, const std::string& cacheDirectory ) = 0;
    virtual StdOutWithReturnCode downloadPackage( const std::string& packageName,
                                                  const std::string& cacheDirectory,
                                                  bool withDependencies ) = 0;

    virtual StdOutWithReturnCode updateAvailable() = 0;
    virtual StdOutWithReturnCode checkAvailable() = 0;

    virtual StdOutWithReturnCode sysupgrade( const std::string& imageUrl, const std::string& cacheDirectory = "" ) = 0;
    virtual StdOutWithReturnCode upgradePackage( const std::string& packageName,
                                                 const std::string& cacheDirectory = "" ) = 0;
    virtual StdOutWithReturnCode installPackage( const std::string& packageName,
                                                 const std::string& cacheDirectory = "" ) = 0;
    virtual StdOutWithReturnCode removePackage( const std::string& packageName ) = 0;
};

}  // namespace Opkg
}  // namespace HubPlatform
