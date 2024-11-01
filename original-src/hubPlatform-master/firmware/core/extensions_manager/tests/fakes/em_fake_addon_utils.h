#pragma once

#include "extensions_manager/common/addon_utils.h"

namespace HubPlatform {
namespace ExtensionsManager {
namespace UT {

class FakeAddonUtils final : public AddonUtils {
public:
    using AddonIds = std::vector< std::string >;
    AddonIds getInstalledAddons() const override {
        return AddonIds();
    }
};

}  // namespace UT
}  // namespace ExtensionsManager
}  // namespace HubPlatform
