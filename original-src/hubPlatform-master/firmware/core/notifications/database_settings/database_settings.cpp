#include "notifications/database_settings/database_settings.h"
#include "notifications/resources/notifications_resources.h"
#include "notifications/exceptions.h"

#include "json_utils/json_utils.h"
#include "database_client/db_tables_names.h"
#include "database_client/idbclient.h"
#include "database_client/dbqueryparser.h"
#include "provision_data_utils/provision_data_manager.h"
#include "crypto_utils/digests.h"
#include "base_utils/string_utils.h"

#include <functional>

namespace HubPlatform {
namespace Notifications {

namespace {
using TypeChecker = bool ( Json::* )() const;

void checkType( const Json& j,
                const std::string& fieldName,
                bool required,
                TypeChecker typeChecker,
                const std::string& expectedType ) {
    auto it = j.find( fieldName );
    if( it == j.cend() ) {
        if( required )
            throw SettingsError( "Missing field '" + fieldName + "'" );
        return;
    }
    auto& value = *it;
    if( !( value.*typeChecker )() )
        throw SettingsError( "Invalid value type for field '" + fieldName + "', expected type '" + expectedType + "'" );
};

void checkEncryption( const Json& j ) {
    namespace Fields = Resources::DB::Settings::FieldsSMTP;
    bool encryption = json_get_bool( j, Fields::Encryption );
    bool verifyServerCertificate = json_get_bool( j, Fields::VerifyServerCertificate );
    bool verifyServerHostname = json_get_bool( j, Fields::VerifyServerHostname );
    std::string serverCACertificate = json_get_str( j, Fields::ServerCACertificate );
    std::string clientCertificate = json_get_str( j, Fields::ClientCertificate );
    std::string clientCertificateKey = json_get_str( j, Fields::ClientCertificateKey );

    std::string url = json_get_str( j, Fields::SMTPServerUrl );

    if( encryption ) {
        if( url.find( "smtps://" ) != 0 )
            throw SettingsError( "Encryption is enabled but server URL does not start with smtps://" );
    }
    else {
        if( url.find( "smtp://" ) != 0 )
            throw SettingsError( "Encryption is disabled but server URL does not start with smtp://" );

        if( verifyServerCertificate )
            throw SettingsError( "Server certificate verification is enabled, while encryption is disabled" );

        if( verifyServerHostname )
            throw SettingsError(
                "Server hostname verification (via certificate) is enabled, while encryption is disabled" );

        if( !serverCACertificate.empty() )
            throw SettingsError( "Server CA certificate is provided, while encryption is disabled" );
    }

    if( verifyServerCertificate && serverCACertificate.empty() )
        throw SettingsError( "Server certificate validation is requested, but CA certificate is not provided" );

    if( !clientCertificate.empty() && clientCertificateKey.empty() )
        throw SettingsError( "Client certificate is provided, while certificate key is missing" );

    if( !clientCertificateKey.empty() && clientCertificate.empty() )
        throw SettingsError( "Client certificate key is provided, while certificate is missing" );
};

void checkServerUrl( const Json& j ) {
    namespace Fields = Resources::DB::Settings::FieldsSMTP;
    std::string url = json_get_str( j, Fields::SMTPServerUrl );

    if( url.find( "smtp://" ) != 0 && url.find( "smtps://" ) != 0 )
        throw SettingsError( "Invalid server URL. URL should start with 'smtp://' or 'smtps://' prefixes" );
}

void validateSMTPSettings( const JsonObject& settings ) {
    namespace Fields = Resources::DB::Settings::FieldsSMTP;
    const bool required = true;

    checkType( settings, Fields::SMTPServerUrl, required, &Json::is_string, "string" );
    checkType( settings, Fields::Encryption, required, &Json::is_boolean, "bool" );
    checkType( settings, Fields::VerifyServerCertificate, !required, &Json::is_boolean, "bool" );
    checkType( settings, Fields::VerifyServerHostname, !required, &Json::is_boolean, "bool" );
    checkType( settings, Fields::ClientUsername, !required, &Json::is_string, "string" );
    checkType( settings, Fields::ClientPassword, !required, &Json::is_string, "string" );

    checkType( settings, Fields::CertificatesVersion, !required, &Json::is_number_integer, "int" );
    checkType( settings, Fields::ServerCACertificate, !required, &Json::is_string, "string" );
    checkType( settings, Fields::ClientCertificate, !required, &Json::is_string, "string" );
    checkType( settings, Fields::ClientCertificateKey, !required, &Json::is_string, "string" );

    checkEncryption( settings );
    checkServerUrl( settings );
}

void encrypt( Json& j, const std::string& key ) {
    auto& crypto = ProvisionDataUtils::getManager().getProvisionData();
    auto it = j.find( key );
    if( it != j.end() ) {
        *it = crypto.encryptSensitiveData( it->get< std::string >() );
    }
};

void decrypt( Json& j, const std::string& key ) {
    auto& crypto = ProvisionDataUtils::getManager().getProvisionData();
    auto it = j.find( key );
    if( it != j.end() ) {
        *it = crypto.decryptSensitiveData( it->get< std::string >() );
    }
};

void replaceSensitiveDataWithHash( Json& j, const std::string& key ) {
    auto& crypto = ProvisionDataUtils::getManager().getProvisionData();
    auto it = j.find( key );
    if( it != j.end() ) {
        j[ key + "MD5" ] = CryptoUtils::Digests::md5( it->get< std::string >() );
        j.erase( it );
    }
};

}  // namespace

DatabaseSettings::DatabaseSettings( IDbClient& dbClient )
: m_dbClient( dbClient ) {}

void DatabaseSettings::setSMTPSettings( JsonObject settings ) {
    namespace Fields = Resources::DB::Settings::FieldsSMTP;

    validateSMTPSettings( settings );
    json_add_str( settings, db::ID_FIELD_NAME, Resources::DB::Settings::SMTPSettingsId );

    encrypt( settings, Fields::ClientUsername );
    encrypt( settings, Fields::ClientPassword );

    encrypt( settings, Fields::ServerCACertificate );
    encrypt( settings, Fields::ClientCertificate );
    encrypt( settings, Fields::ClientCertificateKey );

    m_dbClient.update( HaNotificationsSettingsDbName, JSON_EMPTY_OBJ, settings, true, false );
}

JsonObject DatabaseSettings::getSMTPSettings() {
    namespace Fields = Resources::DB::Settings::FieldsSMTP;

    std::string id = Resources::DB::Settings::SMTPSettingsId;
    auto settings = m_dbClient.findOne( HaNotificationsSettingsDbName, JSON( db::ID_FIELD_NAME << id ), nullptr );
    json_remove( settings, db::ID_FIELD_NAME );

    decrypt( settings, Fields::ClientUsername );
    decrypt( settings, Fields::ClientPassword );

    decrypt( settings, Fields::ServerCACertificate );
    decrypt( settings, Fields::ClientCertificate );
    decrypt( settings, Fields::ClientCertificateKey );

    return settings;
}

JsonObject DatabaseSettings::getHashedSMTPSettings() {
    namespace Fields = Resources::DB::Settings::FieldsSMTP;

    auto settings = getSMTPSettings();

    replaceSensitiveDataWithHash( settings, Fields::ClientUsername );
    replaceSensitiveDataWithHash( settings, Fields::ClientPassword );

    replaceSensitiveDataWithHash( settings, Fields::ServerCACertificate );
    replaceSensitiveDataWithHash( settings, Fields::ClientCertificate );
    replaceSensitiveDataWithHash( settings, Fields::ClientCertificateKey );

    return settings;
}

}  // namespace Notifications
}  // namespace HubPlatform
