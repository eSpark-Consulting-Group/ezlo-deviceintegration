#pragma once

#include "opkg_utils/packages/package_summary.h"

namespace HubPlatform {
namespace Opkg {

class UpgradablePackageSummary final {
public:
    PackageSummary summary;
    std::string availableVersion;
};

}  // namespace Opkg
}  // namespace HubPlatform