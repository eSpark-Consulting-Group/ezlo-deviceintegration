#include "uid/handlers/hub_offline_commands_handler.h"
#include "uid/ui_service.h"

#include "provision_data_utils/provision_data_manager.h"
#include "database_client/dbqueryparser.h"
#include "database_client/db_flags_names.h"
#include "system_utils/environment.h"
#include "system_utils/filesystem.h"
#include "base_utils/utils.h"
#include "base_utils/ezlodb.h"
#include "ssl_utils/sha256.h"
#include "ssl_utils/x509.h"
#include "tiny/tiny_process.h"

namespace HubPlatform {
namespace Uid {

namespace {
constexpr char VersionField[] = "version";
constexpr char NewVersionField[] = "newVersion";
constexpr char NotModifiedField[] = "not_modified";

constexpr char ClientsField[] = "clients";
constexpr char PeersField[] = "peers";
constexpr char CurrentField[] = "current";

constexpr char CertificateField[] = "certificate";
constexpr char PrivateField[] = "private";
constexpr char PublicParentField[] = "public_parent";

constexpr char EnabledField[] = "enabled";
}  // namespace

HubOfflineCommandsHandler::HubOfflineCommandsHandler( Uid::Service& hostService )
: m_hostService( hostService ) {}

//  "result": {
//        "clients": {
//            "12312.... (uuid of user 1 here)": [
//                "7f83b1657ff1fc53b92dc18148a1d65dfc2d4b1fa3d677284addd200126d9069",
//                "hash-of-key2 in hex format"
//            ],
//            "12312.... (uuid of user 2 here)": [
//                "hash-of-key3 in hex format"
//            ]
//        },
//        "peers": {
//             "1245-.. (uuid of controller)" : {
//                 "id": "70010111",
//                 "tokens": ["32525225 (key string)", "325235232 (2nd key string"]
//             }
//        },
//        "current": [{
//            "certificate": "3215235235325.25...53.2...0oi0bhgejkbeamgae (as provisioning platform)",
//            "private": "rfeiojheroiphjeoihgjeorijhoejhgjoijer (as provisioning platform)",
//            "public_parent": "3252532523523.23523532525235 (as provisioning platform)"
//        }],
//        "version": 45
//    }
// -or-
//  "result": {
//        "version": 1,
//        "not_modified": 1
//    }
void HubOfflineCommandsHandler::handleCloudAccessKeysControllerSyncRequest( rpc::Message& rpcMessage ) {
    // TODO: Remove copy of params to result that is needed only for debugging with index.html
    if( rpcMessage.getMessage().count( "result" ) == 0 ) {
        rpcMessage.getMessage()[ "result" ] = rpcMessage.getParams();
    }

    const JsonObject& params = rpcMessage.getMessage()[ "result" ];
    rpcMessage.getAnswer() = nullptr;
    JsonObject errors;

    if( params.count( "error" ) > 0 ) {
        // If cloud API returned an error, re-ask the request in 20 minutes
        auto traceId = rpcMessage.getId();
        LOGGER_ERROR_RAW( "Local mode access keys sync failed with an error: " + cc::j( params[ "error" ] ) );
        const auto AccessKeysRepeatAfterError = std::chrono::minutes( 20 );
        LOGGER_INFO_RAW( "Schedule next call for local mode access keys in " +
                         cc::num10( AccessKeysRepeatAfterError.count() ) + " minutes" );

        auto requestOfflineCredentialsCall = m_hostService.createPostponedTask( [traceId]( Services::PostponedTask& ) {
            LOGGER_INFO_RAW( "Request local mode certificates after previous error, trace: " + traceId );
            HubOfflineCommandsHandler offlineCommandsHandler( Uid::Service::getInstance() );
            offlineCommandsHandler.requestCredentials();
        } );

        requestOfflineCredentialsCall->start( AccessKeysRepeatAfterError, false );

        // Register in the service, cancel registered call
        m_hostService.registerRequestOfflineCredentialsCall( std::move( requestOfflineCredentialsCall ) );
    }

    // Check request parameters
    if( !checkField( params, VersionField, errors ) ) {
        Log::error( cc::error( "Unsupported local mode credentials sync request version format" ) );
        return;
    }

    int newVersion = json_get_int( params, VersionField, 0 );
    int notModified = json_get_int( params, NotModifiedField, 0 );
    int currentVersion = getCurrentCredentialsVersion( m_hostService.getDBClient() );

    if( currentVersion == newVersion || notModified == 1 ) {
        Log::info( cc::info( "Current local mode credentials are up-to-date (v" ) +
                   cc::note( std::to_string( currentVersion ) ) + cc::info( ")" ) );

        return;
    }

    if( !checkField( params, ClientsField, errors ) || !checkField( params, PeersField, errors ) ||
        !checkField( params, CurrentField, errors ) ) {
        Log::error( cc::error( "Unsupported local mode credentials sync request format" ) );
        return;
    }

    auto& clients = params[ ClientsField ];
    auto& peers = params[ PeersField ];
    auto& current = params[ CurrentField ];

    std::string localCertificatesDirectory = HubPlatform::Environment::getCertificateFolder() + "/local";

    // Read previous certificates if any
    std::string prevCertificate;
    std::string prevPrivateKey;
    std::string prevParent;
    auto fieldsToReturn = JSON( "currentCert" << 1 );
    auto prevJson = m_hostService.getDBClient().findOne(
        CredentialsDBName, JSON( db::ID_FIELD_NAME << "current" ), &fieldsToReturn );
    if( !prevJson.empty() ) {
        int prevCertIdx = json_get_int( prevJson, "currentCert", -1 );
        if( prevCertIdx != -1 ) {
            std::string basePath = localCertificatesDirectory + "/" + std::to_string( prevCertIdx );
            if( !tiny::tools::file2string( ( basePath + ".crt" ).c_str(), prevCertificate ) ) {
                Log::error( cc::error( "Can not read " ) + cc::note( basePath + ".crt" ) );
            }
            if( !tiny::tools::file2string( ( basePath + ".key" ).c_str(), prevPrivateKey ) ) {
                Log::error( cc::error( "Can not read " ) + cc::note( basePath + ".key" ) );
            }
            if( !tiny::tools::file2string( ( basePath + ".ca" ).c_str(), prevParent ) ) {
                Log::error( cc::error( "Can not read " ) + cc::note( basePath + ".ca" ) );
            }
        }
    }

    std::set< std::string > missingControllersIds;
    {
        auto currentControllerCredentials = m_hostService.getDBClient().findAll(
            CredentialsDBName, JSON( "serial" << JSON( "$exists" << true ) ), nullptr );
        for( auto& credentials : currentControllerCredentials ) {
            auto controllerId = json_get_str( credentials, "serial" );
            missingControllersIds.insert( controllerId );
        }
    }

    auto lock = m_hostService.lockCertificateFiles();

    // Remove previous credentials and certificates
    m_hostService.getDBClient().clear( CredentialsDBName );

    if( filesystem::isExist( localCertificatesDirectory ) ) {
        filesystem::clearFolder( localCertificatesDirectory );
    }

    // Save new credentials
    for( auto it = clients.begin(); it != clients.end(); ++it ) {
        auto uuid = it.key();
        auto& hashes = it.value();
        for( const auto& element : hashes ) {
            auto hash = json_get_str( element );
            m_hostService.getDBClient().insert( CredentialsDBName, JSON( "uuid" << uuid << "hash" << hash ) );
        }
    }

    auto provision = ProvisionDataUtils::getManager().getProvisionData();

    for( auto it = peers.begin(); it != peers.end(); ++it ) {
        auto uuid = it.key();
        auto& peer = it.value();
        if( peer.is_array() ) {
            for( const auto& element : peer ) {
                auto key = json_get_str( element );
                auto encryptedKey = provision.encryptSensitiveData( key );
                m_hostService.getDBClient().insert( CredentialsDBName,
                                                    JSON( "uuid" << uuid << "serial"
                                                                 << ""
                                                                 << "key" << encryptedKey ) );
            }
        }
        else {
            auto controllerId = json_get_str( peer, "id" );
            JsonArray keys;
            json_get_arr( peer, "tokens", keys );
            for( const auto& element : keys ) {
                auto key = json_get_str( element );
                auto encryptedKey = provision.encryptSensitiveData( key );
                m_hostService.getDBClient().insert(
                    CredentialsDBName, JSON( "uuid" << uuid << "serial" << controllerId << "key" << encryptedKey ) );
            }
            m_hostService.getClusterManager().onCredentialsUpdated( controllerId );
            missingControllersIds.erase( controllerId );
        }
    }

    for( auto& controllerId : missingControllersIds ) {
        m_hostService.getClusterManager().onCredentialsUpdated( controllerId );
    }

    // Save new certificates
    filesystem::createFolder( localCertificatesDirectory );

    Json currentJson;
    currentJson[ db::ID_FIELD_NAME ] = "current";
    currentJson[ "version" ] = newVersion;

    int idx = -1;
    for( const auto& element : current ) {
        idx++;
        std::string certificate;
        std::string privateKey;
        std::string parent;

        if( !getNonEmptyField( element, CertificateField, errors, certificate ) ||
            !getNonEmptyField( element, PrivateField, errors, privateKey ) ||
            !getNonEmptyField( element, PublicParentField, errors, parent ) ) {
            Log::error( cc::error( "Unsupported local mode credentials sync request certificates format" ) );
            Log::error( cc::error( "Certificate Index: " + cc::num10( idx ) + ", error " ) + cc::j( errors ) );
            break;
        }

        auto period = SslUtils::getValidityPeriod( certificate );
        currentJson[ "certificates" ][ std::to_string( idx ) ] =
            JSON( "notBefore" << period.first << "notAfter" << period.second );

        // Check if this certificates are same as previous
        if( certificate == prevCertificate && privateKey == prevPrivateKey && parent == prevParent ) {
            Log::debug( cc::info( "Previous local mode certificate now have index " ) +
                        cc::note( std::to_string( idx ) ) );
            currentJson[ "currentCert" ] = idx;
            LOGGER_TRACE( "New current index is set: " + cc::num10( idx ) );
        }

        bool filesSerialized = true;
        std::string basePath = localCertificatesDirectory + "/" + std::to_string( idx );
        if( !tiny::tools::string2file( ( basePath + ".crt" ).c_str(), certificate ) ) {
            Log::error( cc::error( "Can not write " ) + cc::note( basePath + ".crt" ) );
            filesSerialized = false;
        }
        if( !tiny::tools::string2file( ( basePath + ".key" ).c_str(), privateKey ) ) {
            Log::error( cc::error( "Can not write " ) + cc::note( basePath + ".key" ) );
            filesSerialized = false;
        }
        if( !tiny::tools::string2file( ( basePath + ".ca" ).c_str(), parent ) ) {
            Log::error( cc::error( "Can not write " ) + cc::note( basePath + ".ca" ) );
            filesSerialized = false;
        }

        if( !filesSerialized ) {
            Log::error( "Couldn't write certificate/key data to files" );
        }
    }

    m_hostService.getDBClient().insert( CredentialsDBName, currentJson );

    Log::info( cc::info( "Updated local mode credentials and certificates to v" ) +
               cc::note( std::to_string( newVersion ) ) );
    Log::info( cc::info( "The new certificates:" ) + cc::j( currentJson ) +
               ", object keys: " + cc::j( json_object_keys( currentJson ) ) );

    auto service = &m_hostService;
    m_hostService.addTask( [service]() { service->manageServerListener(); } );
}

//    "params": {
//        "enabled": true
//    }
void HubOfflineCommandsHandler::handleOfflineAnonymousAccessEnabledSetRequest( rpc::Message& rpcMessage ) {
    const JsonObject& params = rpcMessage.getParams();
    JsonObject& joAnswer = rpcMessage.getAnswer();
    if( !checkField( params, EnabledField, joAnswer ) ) {
        return;
    }

    bool enabled = json_get_bool( params, EnabledField, false );
    if( enabled ) {
        Db::setFlag( OfflineAnonymousAccessFlagName );
    }
    else {
        Db::clrFlag( OfflineAnonymousAccessFlagName );
    }
}

//    "params": {
//        "enabled": true
//    }
void HubOfflineCommandsHandler::handleOfflineInsecureAccessEnabledSetRequest( rpc::Message& rpcMessage ) {
    const JsonObject& params = rpcMessage.getParams();
    JsonObject& joAnswer = rpcMessage.getAnswer();
    if( !checkField( params, EnabledField, joAnswer ) ) {
        return;
    }

    bool oldEnabled = Db::getFlag( OfflineInsecureAccessFlagName );
    bool enabled = json_get_bool( params, EnabledField, false );

    if( oldEnabled != enabled ) {
        if( enabled ) {
            Db::setFlag( OfflineInsecureAccessFlagName );
        }
        else {
            Db::clrFlag( OfflineInsecureAccessFlagName );
        }

        auto service = &m_hostService;
        m_hostService.addTask( [service]() { service->manageServerListener( true ); } );
    }
}

//    "params": {
//        "newVersion": 45
//    }
void HubOfflineCommandsHandler::handleCredentialsUpdatedRequest( rpc::Message& rpcMessage ) {
    const JsonObject& params = rpcMessage.getParams();
    JsonObject& joAnswer = rpcMessage.getAnswer();

    if( !checkField( params, NewVersionField, joAnswer ) ) {
        return;
    }

    int newVersion = json_get_int( params, NewVersionField, 0 );
    int currentVersion = getCurrentCredentialsVersion( m_hostService.getDBClient() );

    if( currentVersion == newVersion ) {
        Log::info( cc::info( "Current credentials are up-to-date (v" ) + cc::note( std::to_string( currentVersion ) ) +
                   cc::info( ")" ) );
        return;
    }

    Log::info( cc::info( "New local mode credentials and certificates are available (v" ) +
               cc::note( std::to_string( newVersion ) ) + cc::info( ")" ) );
    auto service = &m_hostService;
    m_hostService.addTask( [service]() { HubOfflineCommandsHandler( *service ).requestCredentials(); } );
}

void HubOfflineCommandsHandler::requestCredentials() {
    int currentVersion = getCurrentCredentialsVersion( m_hostService.getDBClient() );

    Log::info( cc::info( "Request local mode credentials and certificates. Current are v" ) +
               cc::note( std::to_string( currentVersion ) ) );

    auto message = prepareMethod( "cloud.access_keys_controller_sync" );
    message[ "api" ] = "2";  // NOTE: go to previous api version as far as cloud does not support api 3 yet
    if( currentVersion != 0 ) {
        message[ "params" ] = JSON( "version" << currentVersion );
    }

    m_hostService.getNmaClient()->postMessage( message );
}

bool HubOfflineCommandsHandler::anonymousAccessEnabled() {
    return Db::getFlag( OfflineAnonymousAccessFlagName );
}

bool HubOfflineCommandsHandler::authenticateUser( const std::string& uuid, const std::string& key ) {
    auto match = Db::findOne( CredentialsDBName, JSON( "uuid" << uuid << "hash" << SslUtils::sha256( key ) ) );
    return !match.empty();
}

bool HubOfflineCommandsHandler::authenticateController( const std::string& uuid, const std::string& key ) {
    auto provision = ProvisionDataUtils::getManager().getProvisionData();
    auto fieldsToReturn = JSON( "key" << true );
    auto keys =
        Db::findAll( CredentialsDBName, JSON( "uuid" << uuid << "key" << JSON( "$exists" << true ) ), &fieldsToReturn );

    for( auto& keyJson : keys ) {
        auto decryptedKey = provision.decryptSensitiveData( json_get_str( keyJson, "key" ) );
        if( key == decryptedKey ) {
            return true;
        }
    }

    return false;
}

int HubOfflineCommandsHandler::getCurrentCredentialsVersion( IDbClient& dbClient ) {
    int initialVersion = 0;

    auto result = dbClient.findOne( CredentialsDBName, JSON( db::ID_FIELD_NAME << "current" ), nullptr );
    if( result.empty() ) {
        return initialVersion;
    }

    return json_get_int( result, "version", initialVersion );
}

JsonArray HubOfflineCommandsHandler::getCurrentCredentials( IDbClient& dbClient ) {
    Json certificates = JSON_EMPTY_OBJ;

    auto result = dbClient.findOne( CredentialsDBName, JSON( db::ID_FIELD_NAME << "current" ), nullptr );
    if( result.empty() ) {
        LOGGER_ERROR( "Credentials current record is empty!" );
        return certificates;
    }

    if( !json_get( result, "certificates", certificates ) ) {
        LOGGER_ERROR( "No certificates field in the current local mode credentials record found!" );
    }
    return certificates;
}

}  // namespace Uid
}  // namespace HubPlatform
