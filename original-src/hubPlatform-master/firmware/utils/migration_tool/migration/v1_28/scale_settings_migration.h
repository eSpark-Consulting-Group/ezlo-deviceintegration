#pragma once

#include "migration_tool/migration/migration.h"
#include "json_utils/json_utils.h"

class IDbClient;

namespace HubPlatform {
namespace MigrationTool {
namespace v1_28 {

/*

CHANGELOG:
    - Set scale properties for temperature items in scenes.

*/

class Migration : public MigrationTool::Migration {
public:
    explicit Migration( IDbClient& dbClient );
    bool migrate() override;

public:
    Json start( Json& scene );

    int getItemBlocksUpdated() const;
    unsigned getScenesUpdatedCount() const;

private:
    Json processScene( Json& scene, const std::string& keyPath = std::string{} );
    Json processItem( const Json& scene, const std::string& command, Json itemFields );
    Json findCoreItem( const std::string& itemId );

private:
    void processCommandFields( Json& scene, const std::string& command );
    Json& iterateSceneKeys( Json& scene, const std::string& keyPath );
    static std::string fetchBlockMethodName( const Json& scene );
    Json& iterateCompoundBlockFields( Json& scene, Json& fields, const std::string& keyPath );
    void iterateBlockFieldsArray( Json& fields, const std::string& command, const std::string& keyPath );
    void iterateBlocksInFields( Json& scene, const std::string& keyPath, size_t idx, Json& fieldsValueArr );

    void saveScene( const Json& updatedScene );

    static bool isConditionBlock( const std::string& command );
    static bool isLogicOperatorBlock( const std::string& command );
    static bool isBlock( const std::string& key );

    void resetItemBlocksUpdated();
    void incItemBlocksUpdated();

private:
    IDbClient& m_dbClient;
    unsigned m_updatedScenes{ 0 };
    std::string m_defaultScale{ "celsius" };
    int m_itemBlocksUpdated{ 0 };
};

}  // namespace v1_28
}  // namespace MigrationTool
}  // namespace HubPlatform
