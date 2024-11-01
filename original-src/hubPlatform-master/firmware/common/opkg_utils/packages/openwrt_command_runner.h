#pragma once

#include "opkg_utils/packages/command_runner.h"

namespace HubPlatform {
namespace Opkg {

class OpenwrtCommandRunner : public CommandRunner {
public:
    StdOutWithReturnCode listInstalled() const override;
    StdOutWithReturnCode listUpgradable() const override;
    StdOutWithReturnCode listAvailable() const override;
    StdOutWithReturnCode listDependant( const std::string& packageName ) const override;

    StdOutWithReturnCode downloadImage( const std::string& imageUrl, const std::string& cacheDirectory ) override;
    StdOutWithReturnCode downloadPackage( const std::string& packageName,
                                          const std::string& cacheDirectory,
                                          bool withDependencies ) override;

    StdOutWithReturnCode updateAvailable() override;
    StdOutWithReturnCode updateDownloadOnlyAvailable() override;
    StdOutWithReturnCode updateNoActionAvailable() override;

    StdOutWithReturnCode sysupgrade( const std::string& imageUrl, const std::string& cacheDirectory = "" ) override;
    StdOutWithReturnCode upgradePackage( const std::string& packageName,
                                         const std::string& cacheDirectory = "" ) override;
    StdOutWithReturnCode installPackage( const std::string& packageName,
                                         const std::string& cacheDirectory = "" ) override;
    StdOutWithReturnCode removePackage( const std::string& packageName ) override;

protected:
    static StdOutWithReturnCode download( const std::string& outputPath, const std::string& imageUrl );
};

}  // namespace Opkg
}  // namespace HubPlatform
