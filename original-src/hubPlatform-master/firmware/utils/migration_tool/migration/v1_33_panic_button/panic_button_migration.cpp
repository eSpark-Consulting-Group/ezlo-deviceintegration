#include "panic_button_migration.h"
#include "resources.h"

namespace HubPlatform {
namespace MigrationTool {
namespace v1_33_panic {

PanicButtonMigration::PanicButtonMigration( IDbClient& dbClient )
: m_dbClient( dbClient ) {}

bool PanicButtonMigration::migrate() {
    const auto armedCategoryToAdd =
        JSON( Fields::category << Values::sceneController << Fields::subcategories << JSON_ARRAY_( Values::panic ) );

    auto modes = m_dbClient.findAll( Tables::houseModes, JSON_EMPTY_OBJ, nullptr );

    auto updateHouseMode = [this]( const std::string& id, const JsonArray& armedCategories ) {
        m_dbClient.update( Tables::houseModes,
                           JSON( Fields::id << id ),
                           JSON( "$set" << JSON( Fields::armedCategories << armedCategories ) ) );
    };

    for( auto& mode : modes ) {
        Json& armedCategories = mode[ Fields::armedCategories ];
        armedCategories.push_back( Json{ { Fields::category, Values::sceneController },
                                         { Fields::subcategories, JSON_ARRAY_( Values::panic ) } } );

        auto modeId = json_get_str( mode, Fields::id );
        updateHouseMode( modeId, armedCategories );
        m_recordsModified++;
    }
    return true;
}

int PanicButtonMigration::recordsModified() const {
    return m_recordsModified;
}

}  // namespace v1_33_panic
}  // namespace MigrationTool
}  // namespace HubPlatform
