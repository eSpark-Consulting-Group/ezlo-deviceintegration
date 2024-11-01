#pragma once

#include <string>
#include <map>

namespace HubPlatform {
namespace ScenesManager {
namespace IdUtils {

using PlaceholdersMapping = std::map< std::string, std::string >;

bool isPlaceholder( const std::string& str );
std::string replacePlaceholder( const std::string& placeholder, PlaceholdersMapping& mapping );

}  // namespace IdUtils
}  // namespace ScenesManager
}  // namespace HubPlatform
