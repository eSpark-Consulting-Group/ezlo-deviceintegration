#include "protect_utils.h"
#include "jsonrpc/jsonrpc.h"
#include "common_utils/hub_info_helper.h"
#include "cored/model/values/value_factory.h"
#include "config_reader/config_keys.h"
#include "database_client/idbclient.h"
#include "database_client/dbqueryparser.h"
#include "cored/model/resources/cored_resources.h"
#include "protect_validator.h"
#include "resources.h"
#include "contact_id.h"
#include "security_config.h"

namespace HubPlatform {
namespace CoreD {
namespace Protect {
namespace ProtectUtils {

void ensurePredefinedContactIdsInDb( IDbClient& dbClient, const ConfigReader& configReader ) {
    ConfigReader::Path path = ConfigReader::Path( configReader, ConfigKeys::PreDefineContactIds );
    std::size_t size = configReader.getArraySize( path );
    for( std::size_t i = 0; i < size; ++i ) {
        try {
            ConfigReader::Path idPath = path / i;
            const JsonObject contactIdObject = configReader.getJsonObject( idPath );
            ProtectValidator validator( dbClient );
            auto answer = validator.validateAddContactIdAliasRequest( contactIdObject );
            if( !answer.empty() ) {
                LOGGER_ERROR( cc::error( "Contact id object validation failed: " ) + cc::j( answer ) );
                return;
            }
            auto code = json_get_str( contactIdObject, Resources::CMSSignalAlias::Code );
            using namespace Model::Resources;
            if( !dbClient.has( DBNames::ContactIdsDB, JSON( db::ID_FIELD_NAME << code ) ) ) {
                ContactIdInfo contactIdInfo;
                if( contactIdInfo.fromJson( contactIdObject ) ) {
                    auto data = contactIdInfo.toJson();
                    dbClient.insert( DBNames::ContactIdsDB, data );
                }
            }
        }
        catch( const std::exception& ex ) {
            Log::error( "Failed to add predefined contact ids, exception at(" + std::to_string( i ) +
                        "), error :" + std::string( ex.what() ) );
        }
    }
}

void ensurePredefinedSecurityConfigsInDb( IDbClient& dbClient, const ConfigReader& configReader ) {
    ConfigReader::Path path = ConfigReader::Path( configReader, ConfigKeys::PreDefineSecurityConfigs );
    std::size_t size = configReader.getArraySize( path );
    for( std::size_t i = 0; i < size; ++i ) {
        try {
            ConfigReader::Path idPath = path / i;
            const JsonObject securityConfigObject = configReader.getJsonObject( idPath );
            ProtectValidator validator( dbClient );
            auto answer = validator.validateAddSecurityConfigRequest( securityConfigObject );
            if( !answer.empty() ) {
                LOGGER_ERROR( cc::error( "Security Config object validation failed: " ) + cc::j( answer ) );
                return;
            }
            auto id = json_get_str( securityConfigObject, db::ID_FIELD_NAME );
            using namespace Model::Resources;
            if( !dbClient.has( DBNames::SecurityConfigsDB, JSON( db::ID_FIELD_NAME << id ) ) ) {
                SecurityConfigInfo securityConfigInfo;
                if( securityConfigInfo.fromJson( securityConfigObject ) ) {
                    auto data = securityConfigInfo.toJson();
                    dbClient.insert( DBNames::SecurityConfigsDB, data );
                }
            }
        }
        catch( const std::exception& ex ) {
            Log::error( "Failed to add predefined security configs, exception at(" + std::to_string( i ) +
                        "), error :" + std::string( ex.what() ) );
        }
    }
}

JsonObject getSecurityConfigObject( IDbClient& dbClient, const std::string& id ) {
    using namespace Model::Resources;
    auto securityConfig = dbClient.findOne( DBNames::SecurityConfigsDB, JSON( db::ID_FIELD_NAME << id ), nullptr );
    return securityConfig;
}
JsonObject getContactIdObject( IDbClient& dbClient, const std::string& id ) {
    using namespace Model::Resources;
    auto contactId = dbClient.findOne( DBNames::ContactIdsDB, JSON( db::ID_FIELD_NAME << id ), nullptr );
    return contactId;
}

}  // namespace ProtectUtils
}  // namespace Protect
}  // namespace CoreD
}  // namespace HubPlatform