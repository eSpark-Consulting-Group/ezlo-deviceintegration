#pragma once

#include <vector>
#include <string>

namespace HubPlatform {
namespace ExtensionsManager {

class AddonUtils {
public:
    using AddonIds = std::vector< std::string >;
    virtual AddonIds getInstalledAddons() const;

private:
    std::string extractAddonId( const std::string& addonFileName ) const;
};

}  // namespace ExtensionsManager
}  // namespace HubPlatform
