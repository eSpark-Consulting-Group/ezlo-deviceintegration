#pragma once

#include "system_utils/environment.h"

#include <string>
#include <stdexcept>

namespace HubPlatform {
namespace ExtensionsManager {

class PluginPackageValidatorError : public std::runtime_error {
public:
    PluginPackageValidatorError( std::string message );
};

class PluginPackageValidator final {
public:
    PluginPackageValidator( const std::string& packagePath, const std::string& pluginName );
    void validate();

private:
    std::string m_packagePath;
    std::string m_pluginName;
    const std::string m_packageDearchivePrefix;
};

}  // namespace ExtensionsManager
}  // namespace HubPlatform
