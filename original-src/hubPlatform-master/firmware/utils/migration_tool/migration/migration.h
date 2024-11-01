#pragma once

#include "migration_tool/migration/data_version.h"

namespace HubPlatform {
namespace MigrationTool {

class Migration {
public:
    virtual ~Migration() = default;

    virtual bool migrate() = 0;
};

}  // namespace MigrationTool
}  // namespace HubPlatform
