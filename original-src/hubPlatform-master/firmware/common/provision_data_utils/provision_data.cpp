#include "provision_data_utils/provision_data.h"
#include "provision_data_utils/crypto_utils.h"

#include "json_utils/json_utils.h"
#include "base_utils/md5.h"
#include "common_utils/cpp14.h"

namespace HubPlatform {
namespace ProvisionDataUtils {

namespace {
namespace Fields {
constexpr const char ConfigVersion[] = "config_version";
constexpr const char ConfigVersionOld[] = "ver";
constexpr const char Id[] = "id";
constexpr const char Uuid[] = "uuid";
constexpr const char PkDevice[] = "pk_device";
constexpr const char HardwareRevision[] = "hardware_revision";
constexpr const char CloudUuid[] = "cloud_uuid";
constexpr const char OrderUuid[] = "order_uuid";
constexpr const char CloudServer[] = "cloud_server";
constexpr const char ProvisionServer[] = "provision_server";
constexpr const char ProvisionToken[] = "provision_token";
constexpr const char ProvisionTokenOld[] = "provisioning_token";
constexpr const char ProvisionOrder[] = "provision_order";
constexpr const char SslPrivateKey[] = "ssl_private_key";
constexpr const char SslPublicKey[] = "ssl_public_key";
constexpr const char SslSharedKey[] = "ssl_shared_key";
constexpr const char SslSharedKeyOld[] = "ssl_certificate";
constexpr const char SigningCaCertificate[] = "signing_ca_certificate";
constexpr const char SigningCaCertificateOld[] = "ca_certificate";
constexpr const char ZwaveRegion[] = "zwave_region";
constexpr const char DefaultSshPassword[] = "default_ssh_password";
constexpr const char DefaultSshPasswordOld[] = "default_ssh_pass";
constexpr const char DefaultWifiSsid[] = "default_wifi_ssid";
constexpr const char DefaultWifiPassword[] = "default_wifi_password";
constexpr const char DefaultWifiPasswordOld[] = "default_wifi_pass";
constexpr const char TimeServers[] = "time_servers";
}  // namespace Fields

static const std::string Magic = "magic";
}  // namespace

namespace {

bool setNonEmpty( JsonObject& json, const std::string& field, int value ) {
    if( value == 0 )
        return false;

    json[ field ] = value;
    return true;
}

bool setNonEmpty( JsonObject& json, const std::string& field, const std::string& value ) {
    if( value.empty() )
        return false;

    json[ field ] = value;
    return true;
}

bool setNonEmpty( JsonObject& json, const std::string& field, const std::vector< std::string >& value ) {
    if( value.empty() )
        return false;

    json[ field ] = value;
    return true;
}

bool addNonEmpty( JsonObject& json, const std::string& field, const std::string& value ) {
    if( value.empty() )
        return false;

    JsonArray values;
    values.push_back( value );
    json[ field ] = values;
    return true;
}

}  // namespace

bool setNonEmpty( int& field, int value ) {
    if( value == 0 )
        return false;

    field = value;
    return true;
}

bool setNonEmpty( std::string& field, std::string&& value ) {
    if( value.empty() )
        return false;

    field = std::move( value );
    return true;
}

bool setNonEmpty( std::vector< std::string >& field, std::vector< std::string >&& value ) {
    if( value.empty() )
        return false;

    field = std::move( value );
    return true;
}

ProvisionData::ProvisionData()
: configVersion( 0 )
, id( 0 )
, provisionOrder( 0 ) {}

void ProvisionData::fillFromJson( const JsonObject& json ) {
    setNonEmpty( configVersion, json_get_int( json, Fields::ConfigVersion, 0 ) );
    if( configVersion == 0 )
        setNonEmpty( configVersion, json_get_int( json, Fields::ConfigVersionOld, 0 ) );

    if( json.count( Fields::Id ) != 0 && json.at( Fields::Id ).is_string() ) {
        auto idStr = json_get_str( json, Fields::Id );
        if( !idStr.empty() ) {
            try {
                id = std::stoi( idStr );
            }
            catch( ... ) {
            }
        }
    }
    if( id == 0 )
        setNonEmpty( id, json_get_int( json, Fields::Id, 0 ) );

    setNonEmpty( uuid, json_get_str( json, Fields::Uuid ) );
    setNonEmpty( pkDevice, json_get_str( json, Fields::PkDevice ) );
    setNonEmpty( hardwareRevision, json_get_str( json, Fields::HardwareRevision ) );
    setNonEmpty( cloudUuid, json_get_str( json, Fields::CloudUuid ) );
    setNonEmpty( orderUuid, json_get_str( json, Fields::OrderUuid ) );

    setNonEmpty( cloudServer, json_get_str( json, Fields::CloudServer ) );
    setNonEmpty( provisionServer, json_get_str( json, Fields::ProvisionServer ) );
    setNonEmpty( provisionToken, json_get_str( json, Fields::ProvisionToken ) );
    if( provisionToken.empty() )
        setNonEmpty( provisionToken, json_get_str( json, Fields::ProvisionTokenOld ) );

    setNonEmpty( provisionOrder, json_get_int( json, Fields::ProvisionOrder, 0 ) );
    setNonEmpty( sslPrivateKey, json_get_str( json, Fields::SslPrivateKey ) );
    setNonEmpty( sslPublicKey, json_get_str( json, Fields::SslPublicKey ) );
    setNonEmpty( sslSharedKey, json_get_str( json, Fields::SslSharedKey ) );
    if( sslSharedKey.empty() )
        setNonEmpty( sslSharedKey, json_get_str( json, Fields::SslSharedKeyOld ) );

    setNonEmpty( signingCaCertificate, json_get_str( json, Fields::SigningCaCertificate ) );
    if( signingCaCertificate.empty() )
        setNonEmpty( signingCaCertificate, json_get_str( json, Fields::SigningCaCertificateOld ) );

    JsonArray zwaveRegions;
    if( json_get_arr( json, Fields::ZwaveRegion, zwaveRegions ) ) {
        if( !zwaveRegions.empty() ) {
            auto regionJson = zwaveRegions.front();
            if( regionJson.is_string() )
                setNonEmpty( zwaveRegion, regionJson.get< std::string >() );
        }
    }
    else {
        setNonEmpty( zwaveRegion, json_get_str( json, Fields::ZwaveRegion ) );
    }

    setNonEmpty( defaultSshPassword, json_get_str( json, Fields::DefaultSshPassword ) );
    if( defaultSshPassword.empty() )
        setNonEmpty( defaultSshPassword, json_get_str( json, Fields::DefaultSshPasswordOld ) );

    setNonEmpty( defaultWifiSsid, json_get_str( json, Fields::DefaultWifiSsid ) );
    setNonEmpty( defaultWifiPassword, json_get_str( json, Fields::DefaultWifiPassword ) );
    if( defaultWifiPassword.empty() )
        setNonEmpty( defaultWifiPassword, json_get_str( json, Fields::DefaultWifiPasswordOld ) );

    timeServers = json_get_vec< std::string >( json, Fields::TimeServers );

    alignFormatOfEndLines( sslPrivateKey );
    alignFormatOfEndLines( sslPublicKey );
    alignFormatOfEndLines( sslSharedKey );
    alignFormatOfEndLines( signingCaCertificate );
}

JsonObject ProvisionData::toJson() const {
    JsonObject json;
    if( !isValid() )
        return json;

    setNonEmpty( json, Fields::Id, id );
    setNonEmpty( json, Fields::Uuid, uuid );
    setNonEmpty( json, Fields::HardwareRevision, hardwareRevision );
    setNonEmpty( json, Fields::PkDevice, pkDevice );
    setNonEmpty( json, Fields::CloudUuid, cloudUuid );
    setNonEmpty( json, Fields::OrderUuid, orderUuid );
    setNonEmpty( json, Fields::ConfigVersion, configVersion );
    addNonEmpty( json, Fields::ZwaveRegion, zwaveRegion );
    setNonEmpty( json, Fields::ProvisionServer, provisionServer );
    setNonEmpty( json, Fields::CloudServer, cloudServer );
    setNonEmpty( json, Fields::ProvisionToken, provisionToken );
    setNonEmpty( json, Fields::ProvisionOrder, provisionOrder );
    setNonEmpty( json, Fields::SslPrivateKey, sslPrivateKey );
    setNonEmpty( json, Fields::SslPublicKey, sslPublicKey );
    setNonEmpty( json, Fields::SslSharedKey, sslSharedKey );
    setNonEmpty( json, Fields::SigningCaCertificate, signingCaCertificate );
    setNonEmpty( json, Fields::DefaultWifiSsid, defaultWifiSsid );
    setNonEmpty( json, Fields::DefaultWifiPassword, defaultWifiPassword );
    setNonEmpty( json, Fields::DefaultSshPassword, defaultSshPassword );
    setNonEmpty( json, Fields::TimeServers, timeServers );

    return json;
}

bool ProvisionData::isValid() const {
    return configVersion > 0;
}

const CryptoUtils::KeyAes256* ProvisionData::getControllerKey() const {
    return &m_controllerKey;
}

void ProvisionData::setControllerKey( const char* key ) {
    m_controllerKey = CryptoUtils::KeyAes256( reinterpret_cast< const unsigned char* >( key ) );
}

void ProvisionData::setControllerKey( CryptoUtils::KeyAes256 key ) {
    m_controllerKey = std::move( key );
}

std::string ProvisionData::encryptSensitiveData( const std::string& data ) const {
    if( !getControllerKey() )
        return {};

    return encryptData( *getControllerKey(), data + Magic );
}

std::string ProvisionData::decryptSensitiveData( const std::string& data ) const {
    if( !getControllerKey() )
        return {};

    auto result = decryptData( *getControllerKey(), data );
    if( result.size() < Magic.size() )
        return {};

    if( result.compare( result.size() - Magic.size(), Magic.size(), Magic ) )
        return {};

    return result.substr( 0, result.size() - Magic.size() );
}

void ProvisionData::alignFormatOfEndLines( std::string& content ) {
    std::string output, buffer;
    output.reserve( content.size() );

    for( auto currentSymbol : content ) {
        if( currentSymbol == '\\' ) {
            buffer.push_back( currentSymbol );
        }
        else if( !buffer.empty() ) {
            if( currentSymbol == 'n' ) {
                output.push_back( '\n' );
            }
            else if( currentSymbol != 'r' ) {
                output += buffer + currentSymbol;
            }
            buffer.clear();
        }
        else {
            output.push_back( currentSymbol );
        }
    }

    content = output;
}

}  // namespace ProvisionDataUtils
}  // namespace HubPlatform
