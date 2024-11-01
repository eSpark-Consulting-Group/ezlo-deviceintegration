#pragma once

#include "migration_tool/system_repair/system_fix.h"
#include "uci_utils/uci_system_file.h"

#include <string>

class IDbClient;

namespace HubPlatform {
namespace MigrationTool {

/*

Problem:
    Use hub serial as hostname. As serial is specified in provision data - update hostname on firmware startup

*/

class HostnameFix final : public SystemFix {
public:
    const char* getName() const override;
    bool shouldApply() const override;
    bool apply() override;

private:
    std::string getExpectedHostname() const;

private:
    mutable std::string m_expectedHostname;
    mutable UciUtils::UciSystemFile m_systemSettings;
};

}  // namespace MigrationTool
}  // namespace HubPlatform
