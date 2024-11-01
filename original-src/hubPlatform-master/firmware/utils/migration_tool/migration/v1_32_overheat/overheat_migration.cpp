#include "overheat_migration.h"

#include "overheat_migration.h"

namespace HubPlatform {
namespace MigrationTool {
namespace v1_32_overheat {

OverheatMigration::OverheatMigration( IDbClient& dbClient )
: m_dbClient( dbClient ) {}

bool OverheatMigration::migrate() {
    const char* dbHouseModes = "db.firmware.cored.houseModes";

    auto modes = m_dbClient.findAll( dbHouseModes, JSON_EMPTY_OBJ, nullptr );

    auto updateHouseMode = [this, dbHouseModes]( const std::string& id, const JsonArray& armedCategories ) {
        m_dbClient.update(
            dbHouseModes, JSON( "_id" << id ), JSON( "$set" << JSON( "armedCategories" << armedCategories ) ) );
    };

    for( auto& mode : modes ) {
        if( mode.count( "armedCategories" ) == 0 )
            continue;

        int updated = 0;
        Json& armedCategories = mode[ "armedCategories" ];
        for( auto& categoryRecord : armedCategories ) {
            auto categoryValue = json_get_str( categoryRecord, "category" );
            if( categoryValue != "security_sensor" || categoryRecord.count( "subcategories" ) == 0 )
                continue;

            JsonArray& subcategories = categoryRecord[ "subcategories" ];
            if( !subcategories.is_array() )
                continue;

            auto foundIt = std::find_if( begin( subcategories ), end( subcategories ), []( const Json& subcategory ) {
                return json_get_str( subcategory ) == "heat";
            } );
            if( foundIt == end( subcategories ) ) {
                subcategories.push_back( "heat" );
                updated++;
            }
        }

        if( updated > 0 ) {
            auto modeId = json_get_str( mode, "_id" );
            updateHouseMode( modeId, armedCategories );
            m_recordsModified += updated;
        }
    }
    return true;
}

int OverheatMigration::recordsModified() const {
    return m_recordsModified;
}

}  // namespace v1_32_overheat
}  // namespace MigrationTool
}  // namespace HubPlatform