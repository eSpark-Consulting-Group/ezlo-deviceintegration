#include "hub_settings/hub_settings_reader.h"
#include "hub_settings/hub_setting.h"
#include "hub_settings/resources.h"
#include "database_client/dbqueryparser.h"

#include "common_utils/cpp14.h"

namespace HubPlatform {
namespace Hub {

SettingsReader::SettingsReader( const IDbClient& dbClient )
: m_dbClient( dbClient ) {}

std::vector< Setting > SettingsReader::getSettings() const {
    std::vector< Setting > result;
    result.reserve( 8 );

    getSettings( [&result]( Setting&& setting ) { result.emplace_back( std::move( setting ) ); } );
    result.shrink_to_fit();
    return result;
}

int SettingsReader::getSettings( const SettingsReader::OnSettingFound& callback ) const {
    int found = 0;
    using namespace Resources;
    auto jsons = m_dbClient.findAll( DBNames::SettingsDB, JSON_EMPTY_OBJ, nullptr );
    for( auto& json : jsons ) {
        callback( Setting{ std::move( json ) } );
        found++;
    }
    return found;
}

std::unique_ptr< Setting > SettingsReader::findSetting( const std::string& setting ) const {
    using namespace Resources;
    auto json = m_dbClient.findOne( DBNames::SettingsDB, JSON( db::ID_FIELD_NAME << setting ), nullptr );
    if( !json.empty() ) {
        return std::make_unique< Setting >( json );
    }
    return nullptr;
}

}  // namespace Hub
}  // namespace HubPlatform