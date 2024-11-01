#pragma once

#include "json_utils/json_fwd.h"
#include "crypto_utils/crypto_key_aes256.h"

namespace HubPlatform {
namespace ProvisionDataUtils {

struct ProvisionData {
public:
    ProvisionData();
    void fillFromJson( const JsonObject& json );
    JsonObject toJson() const;

    bool isValid() const;

    std::string encryptSensitiveData( const std::string& data ) const;
    std::string decryptSensitiveData( const std::string& data ) const;
    const CryptoUtils::KeyAes256* getControllerKey() const;
    void setControllerKey( const char* key );
    void setControllerKey( CryptoUtils::KeyAes256 key );

private:
    void alignFormatOfEndLines( std::string& content );

private:
    CryptoUtils::KeyAes256 m_controllerKey;

public:
    int configVersion;

    int id;
    std::string pkDevice;
    std::string uuid;
    std::string hardwareRevision;
    std::string cloudUuid;
    std::string orderUuid;

    std::string cloudServer;
    std::string provisionServer;
    std::string provisionToken;
    int provisionOrder;

    std::string sslPrivateKey;
    std::string sslPublicKey;
    std::string sslSharedKey;
    std::string signingCaCertificate;

    std::string zwaveRegion;

    std::string defaultSshPassword;
    std::string defaultWifiSsid;
    std::string defaultWifiPassword;
    std::vector< std::string > timeServers;
};

bool setNonEmpty( int& field, int value );
bool setNonEmpty( std::string& field, std::string&& value );
bool setNonEmpty( std::vector< std::string >& field, std::vector< std::string >&& value );

}  // namespace ProvisionDataUtils
}  // namespace HubPlatform
