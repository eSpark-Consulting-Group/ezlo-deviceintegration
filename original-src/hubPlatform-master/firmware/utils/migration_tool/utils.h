
#pragma once

#include <vector>
#include <string>

class IDbClient;

namespace HubPlatform {
namespace MigrationTool {
namespace Utils {

using Renames = std::vector< std::pair< std::string, std::string > >;

bool renameCollections( IDbClient& dbClient, const Renames& renames );
bool renameCollection( IDbClient& dbClient, const std::string& oldName, const std::string& newName );

}  // namespace Utils
}  // namespace MigrationTool
}  // namespace HubPlatform
