#pragma once

#include "migration_tool/system_repair/system_fix.h"

namespace HubPlatform {
namespace MigrationTool {

/*

Problem:
    Plugins can not receive incoming UDP packets from EZVIZ cameras. Allow incoming packets

*/

class FirewallFix final : public SystemFix {
public:
    const char* getName() const override;
    bool shouldApply() const override;
    bool apply() override;
};

}  // namespace MigrationTool
}  // namespace HubPlatform
