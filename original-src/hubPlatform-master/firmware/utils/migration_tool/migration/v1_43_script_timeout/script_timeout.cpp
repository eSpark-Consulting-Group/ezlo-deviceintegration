#include "script_timeout.h"
#include "database_client/idbclient.h"
#include "json_utils/json_utils.h"
#include "config_reader/config_reader.h"
#include "config_reader/config_keys.h"

namespace HubPlatform {
namespace MigrationTool {
namespace v1_43_script_timeout {

namespace DbNames {
const char CoreSettingsDBName[] = "db.firmware.cored.internal_settings";
}  // namespace DbNames

namespace Fields {
const char id[] = "_id";
const char MinTimeout[] = "lua.min_timeout";
const char DefaultTimeout[] = "lua.default_timeout";
};  // namespace Fields

ScriptTimeoutMigration::ScriptTimeoutMigration( IDbClient& dbClient )
: m_dbClient( dbClient ) {}

int ScriptTimeoutMigration::getConfigTimeoutValue( const std::string& key ) {
    ConfigReader configReader;
    const auto configKey = key.c_str();
    if( configReader.hasValue( configKey ) )
        return configReader.getInt( configKey );

    LOGGER_WARN( "Cannot read value in \"" + cc::note( configKey ) + "\"." );
    return 0;
}

bool ScriptTimeoutMigration::migrate() {
    LOGGER_DEBUG_RAW( "ScriptTimeoutMigration::migrate called" );

    const char Value[] = "value";
    const char ValueType[] = "valueType";

    int defaultTimeout = getConfigTimeoutValue( ConfigKeys::LuadScriptTimeout );
    int minTimeout = getConfigTimeoutValue( ConfigKeys::LuadScriptMinTimeout );

    m_dbClient.insert( DbNames::CoreSettingsDBName,
                       JSON( Fields::id << Fields::DefaultTimeout << Value << defaultTimeout << ValueType << "int" ) );
    m_dbClient.insert( DbNames::CoreSettingsDBName,
                       JSON( Fields::id << Fields::MinTimeout << Value << minTimeout << ValueType << "int" ) );

    return true;
}

}  // namespace v1_43_script_timeout
}  // namespace MigrationTool
}  // namespace HubPlatform
