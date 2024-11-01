#pragma once
#include "migration_tool/migration/migration.h"

class IDbClient;

namespace HubPlatform {
namespace MigrationTool {
namespace v1_39_security_threat {

/*

CHANGELOG:
    - If a device is armed and has no security_threat item, set device.armed=false.

*/

namespace DbNames {
extern const char* DevicesDb;
extern const char* ItemsDb;
}  // namespace DbNames

class Migration : public MigrationTool::Migration {
public:
    explicit Migration( IDbClient& dbClient );
    bool migrate() override;
    size_t modifiedDevices() const;

private:
    IDbClient& m_dbClient;
    size_t m_modifiedDevices{ 0 };
};

}  // namespace v1_39_security_threat
}  // namespace MigrationTool
}  // namespace HubPlatform
