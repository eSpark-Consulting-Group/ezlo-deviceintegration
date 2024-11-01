#pragma once

#include "migration_tool/migration/migration.h"

namespace HubPlatform {
namespace MigrationTool {

class DummyMigration : public Migration {
public:
    bool migrate() override {
        // NOTE: nothing to do.
        return true;
    }
};

}  // namespace MigrationTool
}  // namespace HubPlatform
