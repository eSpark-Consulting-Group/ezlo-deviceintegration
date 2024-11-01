#pragma once

#include <string>

namespace HubPlatform {
namespace UpdateManager {

enum class UpdateOperationType { Unknown, Install, Upgrade, Sysupgrade, Uninstall };
enum class UpdateComponentType { Unknown, Firmware, Addon, Plugin, CustomPlugin, CustomPackage };

UpdateOperationType updateOperationTypeFromString( const std::string& string );
const std::string& updateOperationTypeToString( UpdateOperationType type );

UpdateComponentType updateComponentTypeFromString( const std::string& string );
const std::string& updateComponentTypeToString( UpdateComponentType type );

}  // namespace UpdateManager
}  // namespace HubPlatform
