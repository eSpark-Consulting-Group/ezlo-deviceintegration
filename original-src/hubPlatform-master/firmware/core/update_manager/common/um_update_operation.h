#pragma once

#include "opkg_utils/packages/package_summary.h"
#include "um_update_operation_type.h"

#include "json_utils/json_fwd.h"

namespace HubPlatform {
namespace UpdateManager {

namespace Resources {
namespace UpdateOperationFields {
constexpr const char Operation[] = "operation";
constexpr const char Source[] = "source";
constexpr const char Snapshot[] = "snapshot";
constexpr const char Type[] = "type";
constexpr const char Id[] = "id";
constexpr const char Version[] = "version";
constexpr const char ArchiveHashMd5[] = "archive_hash_md5";
}  // namespace UpdateOperationFields
}  // namespace Resources

struct UpdateOperation {
    UpdateOperation();
    UpdateOperation( Opkg::PackageSummary packageSummary,
                     UpdateOperationType operationType,
                     std::string source,
                     int snapshot );

    static UpdateOperation fromJson( const JsonObject& json );
    JsonObject toJson() const;

    bool isValid() const;

private:
    static bool fillOperationName( const JsonObject& json, UpdateOperation& target );
    bool isCustomPlugin() const;
    bool isCustomPluginSupportedOperation() const;
    static bool hasAddonPrefix( const std::string& packageName );
    static bool hasPluginPrefix( const std::string& packageName );
    static bool hasCustomPackagePrefix( const std::string& packageName );

public:
    Opkg::PackageSummary packageSummary;
    UpdateOperationType operationType;
    UpdateComponentType componentType;
    std::string source;
    int snapshot;

    std::string componentHash;  // md5
};

}  // namespace UpdateManager
}  // namespace HubPlatform
