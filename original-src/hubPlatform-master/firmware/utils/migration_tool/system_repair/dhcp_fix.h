#pragma once

#include "migration_tool/system_repair/system_fix.h"

namespace HubPlatform {
namespace MigrationTool {

/*

Problem:
    DHCPv6 working on ethernet connection. Disable it.

*/

class DhcpFix final : public SystemFix {
public:
    const char* getName() const override;
    bool shouldApply() const override;
    bool apply() override;
};

}  // namespace MigrationTool
}  // namespace HubPlatform
