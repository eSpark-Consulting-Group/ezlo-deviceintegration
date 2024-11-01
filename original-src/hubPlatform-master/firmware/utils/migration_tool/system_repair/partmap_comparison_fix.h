#pragma once

#include "migration_tool/system_repair/system_fix.h"

#include <string>

class IDbClient;

namespace HubPlatform {
namespace MigrationTool {

/*

Problem:
    User partition size on controller may differ from its size in sysupgrade image,
    current sysupgrade logic errors out if any partition size differs, however
    we do not use user partition from image, so it should not affect sysupgrade

*/

class PartmapComparisonFix final : public SystemFix {
public:
    const char* getName() const override;
    bool shouldApply() const override;
    bool apply() override;

private:
    std::string getFileHash( const std::string& fileName ) const;
    bool safeCopyFile( const std::string& fromFile, const std::string& toFile );
};

}  // namespace MigrationTool
}  // namespace HubPlatform
