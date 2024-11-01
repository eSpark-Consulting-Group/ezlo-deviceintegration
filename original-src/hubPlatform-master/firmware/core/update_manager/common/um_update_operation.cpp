#include "update_manager/common/um_update_operation.h"
#include "update_manager/resources/um_resources.h"

#include "json_utils/json_utils.h"

#include <cstring>
#include <utility>

namespace HubPlatform {
namespace UpdateManager {

UpdateOperation::UpdateOperation()
: UpdateOperation( Opkg::PackageSummary{}, UpdateOperationType::Unknown, "", 0 ) {}

UpdateOperation::UpdateOperation( Opkg::PackageSummary packageSummary,
                                  UpdateOperationType operationType,
                                  std::string source,
                                  int snapshot )
: packageSummary( std::move( packageSummary ) )
, operationType( operationType )
, componentType()
, source( std::move( source ) )
, snapshot( snapshot )
, componentHash() {}

// json = {
//        "operation": "upgrade",
//        "source": "http://dl.mios.com/linux_firmware/qa",
//        "snapshot": 42,
//        "type": "plugin",
//        "id": "zwave",
//        "version": "1.3.42"
//    }

UpdateOperation UpdateOperation::fromJson( const JsonObject& json ) {
    namespace Fields = Resources::UpdateOperationFields;
    UpdateOperation operation;

    operation.componentType = updateComponentTypeFromString( json_get_str( json, Fields::Type ) );

    if( !fillOperationName( json, operation ) ) {
        LOGGER_ERROR( cc::error( "Failed to fill operation name" ) );
        return {};
    }

    namespace Fields = Resources::UpdateOperationFields;
    auto operationType = json_get_str( json, Fields::Operation );
    operation.operationType = updateOperationTypeFromString( operationType );
    if( operation.operationType == UpdateOperationType::Unknown ) {
        LOGGER_ERROR( cc::error( "Invalid Operation type: Unknown: " ) + operationType );
        return {};
    }

    if( operation.isCustomPlugin() && operation.isCustomPluginSupportedOperation() ) {
        if( operation.componentHash.empty() ) {
            LOGGER_ERROR( cc::error( "Empty custom plugin component hash" ) );
            return {};
        }
    }

    operation.source = json_get_str( json, Fields::Source );
    operation.snapshot = json_get_int( json, Fields::Snapshot, 0 );
    auto versionString = json_get_str( json, Fields::Version );
    if( !versionString.empty() )
        operation.packageSummary.version = versionString;

    return operation;
}

// TODO: EFWR-4028: Cover with tests
bool UpdateOperation::fillOperationName( const JsonObject& json, UpdateOperation& operation ) {
    namespace Fields = Resources::UpdateOperationFields;

    if( operation.componentType == UpdateComponentType::Firmware ) {
        operation.packageSummary.name = Resources::PackageNames::Firmware;
    }
    else if( operation.componentType == UpdateComponentType::Addon ) {
        auto id = json_get_str( json, Fields::Id );
        operation.packageSummary.name = Resources::PackageNames::AddonPrefix + id;
    }
    else if( operation.componentType == UpdateComponentType::Plugin ) {
        auto id = json_get_str( json, Fields::Id );
        operation.packageSummary.name = Resources::PackageNames::PluginPrefix + id;
    }
    else if( operation.componentType == UpdateComponentType::CustomPlugin ) {
        operation.packageSummary.name = json_get_str( json, Fields::Id );
        operation.componentHash = json_get_str( json, Fields::ArchiveHashMd5 );

        if( operation.packageSummary.name.empty() ) {
            return false;
        }
    }
    else if( operation.componentType == UpdateComponentType::CustomPackage ) {
        auto id = json_get_str( json, Fields::Id );
        operation.packageSummary.name = Resources::PackageNames::CustomPackagePrefix + id;
    }
    else {
        return false;
    }
    return true;
}

JsonObject UpdateOperation::toJson() const {
    namespace Fields = Resources::UpdateOperationFields;
    JsonObject json = JSON_EMPTY_OBJ;
    json[ Fields::Operation ] = updateOperationTypeToString( operationType );
    json[ Fields::Source ] = source;
    json[ Fields::Snapshot ] = snapshot;

    json[ Fields::Type ] = updateComponentTypeToString( componentType );

    if( packageSummary.name == Resources::PackageNames::Firmware ) {
        json[ Fields::Type ] = updateComponentTypeToString( UpdateComponentType::Firmware );
    }
    else if( hasAddonPrefix( packageSummary.name ) ) {
        json[ Fields::Type ] = updateComponentTypeToString( UpdateComponentType::Addon );
        json[ Fields::Id ] = packageSummary.name.substr( strlen( Resources::PackageNames::AddonPrefix ) );
    }
    else if( hasPluginPrefix( packageSummary.name ) ) {
        json[ Fields::Type ] = updateComponentTypeToString( UpdateComponentType::Plugin );
        json[ Fields::Id ] = packageSummary.name.substr( strlen( Resources::PackageNames::PluginPrefix ) );
    }
    else if( hasCustomPackagePrefix( packageSummary.name ) ) {
        json[ Fields::Type ] = updateComponentTypeToString( UpdateComponentType::CustomPackage );
        json[ Fields::Id ] = packageSummary.name.substr( strlen( Resources::PackageNames::CustomPackagePrefix ) );
    }

    json[ Fields::Version ] = packageSummary.version;

    if( componentType == UpdateComponentType::CustomPlugin ) {
        json[ Fields::ArchiveHashMd5 ] = componentHash;
        json[ Fields::Id ] = packageSummary.name;
    }

    return json;
}

bool UpdateOperation::isValid() const {
    return operationType != UpdateOperationType::Unknown;
}

bool UpdateOperation::isCustomPlugin() const {
    return componentType == UpdateComponentType::CustomPlugin;
}

bool UpdateOperation::isCustomPluginSupportedOperation() const {
    return ( operationType == UpdateOperationType::Install || operationType == UpdateOperationType::Upgrade );
}

bool UpdateOperation::hasAddonPrefix( const std::string& packageName ) {
    return ( strncmp( packageName.c_str(),
                      Resources::PackageNames::AddonPrefix,
                      strlen( Resources::PackageNames::AddonPrefix ) ) == 0 );
}

bool UpdateOperation::hasPluginPrefix( const std::string& packageName ) {
    return ( strncmp( packageName.c_str(),
                      Resources::PackageNames::PluginPrefix,
                      strlen( Resources::PackageNames::PluginPrefix ) ) == 0 );
}

bool UpdateOperation::hasCustomPackagePrefix( const std::string& packageName ) {
    return ( strncmp( packageName.c_str(),
                      Resources::PackageNames::CustomPackagePrefix,
                      strlen( Resources::PackageNames::CustomPackagePrefix ) ) == 0 );
}

}  // namespace UpdateManager
}  // namespace HubPlatform
