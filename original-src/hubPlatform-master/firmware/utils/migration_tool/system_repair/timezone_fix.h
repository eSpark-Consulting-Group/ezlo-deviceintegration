#pragma once

#include "migration_tool/system_repair/system_fix.h"
#include "uci_utils/uci_system_file.h"

#include <string>

class IDbClient;

namespace HubPlatform {
namespace MigrationTool {

/*

Problem:
    After timezone is set via API it may be changed in system from console or by any other means.
    If system timezone differs from one stored in our database, update system timezone

*/

class TimezoneFix final : public SystemFix {
public:
    TimezoneFix( IDbClient& dbClient );
    const char* getName() const override;
    bool shouldApply() const override;
    bool apply() override;

private:
    std::string getExpectedTimezone() const;

private:
    IDbClient& m_dbClient;

    mutable std::string m_expectedTimezone;
    mutable UciUtils::UciSystemFile m_systemSettings;
};

}  // namespace MigrationTool
}  // namespace HubPlatform
