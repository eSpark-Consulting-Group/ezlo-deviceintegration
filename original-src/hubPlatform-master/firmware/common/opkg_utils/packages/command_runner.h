#pragma once

#include <string>
#include <vector>

namespace HubPlatform {
namespace Opkg {

class CommandRunner {
public:
    ~CommandRunner() = default;

    using StdOutWithReturnCode = std::pair< std::string, int >;

    virtual StdOutWithReturnCode listInstalled() const = 0;
    virtual StdOutWithReturnCode listUpgradable() const = 0;
    virtual StdOutWithReturnCode listAvailable() const = 0;
    virtual StdOutWithReturnCode listDependant( const std::string& packageName ) const = 0;

    virtual StdOutWithReturnCode downloadImage( const std::string& imageUrl, const std::string& cacheDirectory ) = 0;
    virtual StdOutWithReturnCode downloadPackage( const std::string& packageName,
                                                  const std::string& cacheDirectory,
                                                  bool withDependencies ) = 0;

    virtual StdOutWithReturnCode updateAvailable() = 0;
    virtual StdOutWithReturnCode updateDownloadOnlyAvailable() = 0;
    virtual StdOutWithReturnCode updateNoActionAvailable() = 0;

    virtual StdOutWithReturnCode sysupgrade( const std::string& imageUrl, const std::string& cacheDirectory = "" ) = 0;
    virtual StdOutWithReturnCode upgradePackage( const std::string& packageName,
                                                 const std::string& cacheDirectory = "" ) = 0;
    virtual StdOutWithReturnCode installPackage( const std::string& packageName,
                                                 const std::string& cacheDirectory = "" ) = 0;
    virtual StdOutWithReturnCode removePackage( const std::string& packageName ) = 0;
};

}  // namespace Opkg
}  // namespace HubPlatform
