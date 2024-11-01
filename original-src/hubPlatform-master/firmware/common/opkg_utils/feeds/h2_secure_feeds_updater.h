#pragma once

#include "opkg_utils/feeds/h2_feeds_updater.h"

#include "string"

namespace HubPlatform {
namespace Opkg {

class H2SecureFeedsUpdater final : public H2FeedsUpdater {
public:
    using H2FeedsUpdater::H2FeedsUpdater;

private:
    std::string getHubType() const override;
};

}  // namespace Opkg
}  // namespace HubPlatform
