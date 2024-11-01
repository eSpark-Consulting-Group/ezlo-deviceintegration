#include "migration_tool/migration/v1_7/migration1_7.h"

#include "database_client/idbclient.h"
#include "json_utils/json_utils.h"

namespace HubPlatform {
namespace MigrationTool {
namespace v1_7 {

Migration::Migration( IDbClient& dbClient, const ProvisionDataUtils::ProvisionData& provisionData )
: m_dbClient( dbClient )
, m_provisionData( provisionData ) {}

bool Migration::migrate() {
    if( !migrateWiFiPasswords() )
        return false;

    return migrateUserCodes();
}

bool Migration::migrateWiFiPasswords() {
    auto WifiSettingsDBName = "db.firmware.networkd.wifiSettings";
    auto currentSettings = m_dbClient.findOne( WifiSettingsDBName,
                                               JSON( "_id"
                                                     << "current_settings" ),
                                               nullptr );
    if( !currentSettings.empty() ) {
        currentSettings[ "key" ] = m_provisionData.encryptSensitiveData( currentSettings[ "key" ] );
        m_dbClient.update( WifiSettingsDBName,
                           JSON( "_id"
                                 << "current_settings" ),
                           currentSettings );
    }

    auto backupSettings = m_dbClient.findOne( WifiSettingsDBName,
                                              JSON( "_id"
                                                    << "backup_settings" ),
                                              nullptr );
    if( !backupSettings.empty() ) {
        backupSettings[ "key" ] = m_provisionData.encryptSensitiveData( backupSettings[ "key" ] );
        m_dbClient.update( WifiSettingsDBName,
                           JSON( "_id"
                                 << "backup_settings" ),
                           backupSettings );
    }
    return true;
}

bool Migration::migrateUserCodes() {
    auto ItemsDBName = "db.firmware.uid.items";
    auto TempItemsDBName = "temp.db.firmware.uid.items";

    std::vector< JsonObject > items;
    const auto itemsStatic = m_dbClient.findAll( ItemsDBName, JSON_EMPTY_OBJ, nullptr );
    for( const auto& itemStatic : itemsStatic ) {
        const auto itemDynamic = m_dbClient.findOne( TempItemsDBName, JSON( "_id" << itemStatic[ "_id" ] ), nullptr );
        items.push_back( JSON( itemStatic << itemDynamic ) );
    }

    for( auto& item : items ) {
        const auto valueIt = item.find( "value" );
        if( valueIt == item.end() )
            continue;

        auto& value = valueIt.value();

        const auto type = json_get_str( item, "type" );
        if( type == "dictionary.userCode" ) {
            for( auto it = value.begin(); it != value.end(); ++it ) {
                auto& userCodeValue = it.value();
                auto userCodeIt = userCodeValue.find( "code" );
                if( userCodeIt == userCodeValue.end() || !userCodeIt->is_string() )
                    continue;

                item[ "value" ][ it.key() ][ "code" ] = m_provisionData.encryptSensitiveData( userCodeIt.value() );
            }
        }
        else if( type == "userCode" ) {
            auto userCodeIt = value.find( "code" );
            if( userCodeIt == value.end() || !userCodeIt->is_string() )
                continue;

            item[ "value" ][ "code" ] = m_provisionData.encryptSensitiveData( userCodeIt.value() );
        }
        m_dbClient.update( ItemsDBName,
                           JSON( "_id" << item[ "_id" ] << "$or"
                                       << JSON_ARRAY( JSON( "value" << JSON( "$exists" << false ) )
                                                      << JSON( "value" << JSON( "$ne" << JSON_EMPTY_OBJ ) ) ) ),
                           JSON( "$set" << JSON( "value" << item[ "value" ] ) ) );

        m_dbClient.update( TempItemsDBName,
                           JSON( "_id" << item[ "_id" ] << "$or"
                                       << JSON_ARRAY( JSON( "value" << JSON( "$exists" << false ) )
                                                      << JSON( "value" << JSON( "$ne" << JSON_EMPTY_OBJ ) ) ) ),
                           JSON( "$set" << JSON( "value" << item[ "value" ] ) ) );
    }

    return true;
}

}  // namespace v1_7
}  // namespace MigrationTool
}  // namespace HubPlatform
