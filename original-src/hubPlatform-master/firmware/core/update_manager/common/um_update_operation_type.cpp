#include "um_update_operation_type.h"

#include <unordered_map>

namespace HubPlatform {
namespace UpdateManager {

namespace {

struct UpdateOperationTypeHash {
    size_t operator()( const UpdateOperationType& type ) const {
        return static_cast< size_t >( type );
    }
};

struct UpdateComponentTypeHash {
    size_t operator()( const UpdateComponentType& type ) const {
        return static_cast< size_t >( type );
    }
};

const std::unordered_map< UpdateOperationType, std::string, UpdateOperationTypeHash > updateOperationTypeToStringMap = {
    { UpdateOperationType::Unknown, "" },
    { UpdateOperationType::Install, "install" },
    { UpdateOperationType::Upgrade, "upgrade" },
    { UpdateOperationType::Sysupgrade, "sysupgrade" },
    { UpdateOperationType::Uninstall, "uninstall" },
};

const std::unordered_map< std::string, UpdateOperationType > stringToUpdateOperationTypeMap = [&]() {
    std::unordered_map< std::string, UpdateOperationType > map;
    for( const auto& pair : updateOperationTypeToStringMap ) {
        map.emplace( std::make_pair( pair.second, pair.first ) );
    }
    return map;
}();

const std::unordered_map< UpdateComponentType, std::string, UpdateComponentTypeHash > updateComponentTypeToStringMap = {
    { UpdateComponentType::Unknown, "" },
    { UpdateComponentType::Firmware, "firmware" },
    { UpdateComponentType::Addon, "addon" },
    { UpdateComponentType::Plugin, "plugin" },
    { UpdateComponentType::CustomPlugin, "custom_plugin" },
    { UpdateComponentType::CustomPackage, "custom_package" }
};

const std::unordered_map< std::string, UpdateComponentType > stringToUpdateComponentTypeMap = [&]() {
    std::unordered_map< std::string, UpdateComponentType > map;
    for( const auto& pair : updateComponentTypeToStringMap ) {
        map.emplace( std::make_pair( pair.second, pair.first ) );
    }
    return map;
}();

}  // namespace

UpdateOperationType updateOperationTypeFromString( const std::string& string ) {
    auto it = stringToUpdateOperationTypeMap.find( string );
    if( it != stringToUpdateOperationTypeMap.end() ) {
        return it->second;
    }
    return UpdateOperationType::Unknown;
}

const std::string& updateOperationTypeToString( UpdateOperationType updateOperationType ) {
    auto it = updateOperationTypeToStringMap.find( updateOperationType );
    if( it != updateOperationTypeToStringMap.end() ) {
        return it->second;
    }
    return updateOperationTypeToStringMap.find( UpdateOperationType::Unknown )->second;
}

UpdateComponentType updateComponentTypeFromString( const std::string& string ) {
    auto it = stringToUpdateComponentTypeMap.find( string );
    if( it != stringToUpdateComponentTypeMap.end() ) {
        return it->second;
    }
    return UpdateComponentType::Unknown;
}

const std::string& updateComponentTypeToString( UpdateComponentType type ) {
    auto it = updateComponentTypeToStringMap.find( type );
    if( it != updateComponentTypeToStringMap.end() ) {
        return it->second;
    }
    return updateComponentTypeToStringMap.find( UpdateComponentType::Unknown )->second;
}

}  // namespace UpdateManager
}  // namespace HubPlatform
