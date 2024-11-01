#pragma once

#include "opkg_utils/packages/openwrt_command_runner.h"

namespace HubPlatform {
namespace Opkg {

class LocalCommandRunner : public OpenwrtCommandRunner {
public:
    StdOutWithReturnCode listInstalled() const override;
};

}  // namespace Opkg
}  // namespace HubPlatform
