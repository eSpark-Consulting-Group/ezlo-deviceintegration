#pragma once

#include "metrics/internal_metrics/internal_metrics.h"
#include "json_utils/json_utils.h"
#include "wire.pb.h"
#include "supervisor.pb.h"

extern "C" {
#include <openssl/ssl.h>
#include <openssl/evp.h>
#include <openssl/conf.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/bio.h>
#include <openssl/x509.h>
}

namespace HubPlatform {
namespace Metrics {

struct MetricsWire {
    std::string signature;
    std::string encryptedKey;
    std::string iv;
    std::string encryptedBytes;
};

enum CERTIFICATE_SOURCE {
    FILE_SOURCE,
    STRING_SOURCE,
};

class DataSender {
public:
    explicit DataSender( InternalMetricsPtr metricsPtr );
    ~DataSender();

    bool sendData( const std::string& data );
    bool sendData( const Json& data );

    void setServerUrlAndPort( const std::string& newServerUrl, std::vector< size_t > newServerPorts );
    void setCertifications( const std::string& serverCrt, const std::string& clientCrt, const std::string& clientKey );
    void setSendInterval( const uint64_t& interval );

    void setClientCertificate( const std::string& clientCertificate, CERTIFICATE_SOURCE sourceType );
    void setClientPrivateKey( const std::string& clientPrivateKey, CERTIFICATE_SOURCE sourceType );
    void setServerPublicKey( const std::string& serverPublicKey, CERTIFICATE_SOURCE sourceType );

private:
    static int encrypt( const unsigned char* plainText,
                        int plainTextLen,
                        const unsigned char* key,
                        const unsigned char* iv,
                        unsigned char* cipherText );
    bool encryptAndSignMessage( const std::string& serializedMessage, MetricsWire& wireOut );
    bool udpSendData( const char* data, size_t dataLen );

    bool checkIfDataSendable( const Report& protobufData ) const;
    static bool checkAnomaly( const Report& protobufData );

    void readClientCrtBytes( const std::string& file, CERTIFICATE_SOURCE sourceType );
    EVP_PKEY* readPrivateKey( const std::string& keySource, CERTIFICATE_SOURCE sourceType );
    EVP_PKEY* readPublicKey( const std::string& keyFile, CERTIFICATE_SOURCE sourceType );

    static std::string generateRandString( int size );
    void jsonToReportMsg( Report& report, const Json& watcherReport, bool reduceReport ) const;
    static std::string ucharToStdString( unsigned char* data, int dataLen );
    static const unsigned char* toBytePointer( const std::string& str );
    static void fillReportHubInfo( const Json& json, bool reducedReport, Report& report );
    static void fillHubErrors( const JsonArray& hubErrors, Report& report );
    static void fillSshTunnelsInfo( const JsonArray& sshTunnels, Report& report );

    Json getInnerJson( const Json& json, const std::vector< Json >& nestedJsonKeys );
    static void jsonToReportSystem( const Json& json, Report& report );
    static void jsonToReportNetwork( const Json& json, Report& report );
    static void jsonToReportTime( const Json& json, Report& report );
    static void jsonToReportProcesses( const Json& json, Report& report );
    static void jsonToReportUIService( const Json& data, Report& report );
    size_t getServerPort();
    static void fillHttpServerReport( const JsonObject& data, Report& report );
    static void fillNmaReport( const JsonObject& data, Report& report );
    void buildInternalMetricsReport( Report& report ) const;
    static void jsonToReportZwaveService( const Json& data, Report& report );
    static void fillZwaveCompletedCommandsReport( const JsonObject& data, Report& report );
    static void fillZwaveFailedCommandsReport( const JsonObject& data, Report& report );

private:
    using X509Ptr = std::unique_ptr< X509, decltype( &X509_free ) >;
    using BioPtr = std::unique_ptr< BIO, decltype( &BIO_free_all ) >;
    using EvpMdCtxPtr = std::unique_ptr< EVP_MD_CTX, decltype( &EVP_MD_CTX_destroy ) >;
    using EvpPkeyCtxPtr = std::unique_ptr< EVP_PKEY_CTX, decltype( &EVP_PKEY_CTX_free ) >;
    using EvpCipherCtxPtr = std::unique_ptr< EVP_CIPHER_CTX, decltype( &EVP_CIPHER_CTX_free ) >;

    std::string m_serverUrl;
    std::vector< size_t > m_serverPorts;
    size_t m_serverPortIndex;
    uint64_t m_lastSendTime = 0;
    uint64_t m_sendInterval;
    const uint32_t m_protobufVersion;

    std::string m_serverCrtFile;
    std::string m_clientCrtFile;
    std::string m_clientKeyFile;
    bool m_clientPrivateKeyRead = false;
    bool m_serverPublicKeyRead = false;
    bool m_isClientCrtBytesRead = false;

    std::string m_clientCrtBytes;
    int m_clientCrtBytesLen = 0;
    EVP_PKEY* m_clientPrivateKey = nullptr;
    EVP_PKEY* m_serverPublicKey = nullptr;

    static constexpr std::size_t m_ivSize = 128u / 8u;
    static constexpr uint16_t m_loadAverageThreshold = 4;
    static constexpr uint16_t m_uptime5MinThreshold = 300;
    static constexpr uint16_t m_availMemoryThreshold = 20;
    static constexpr std::size_t m_AES256keySize = 256u / 8u;

    InternalMetricsPtr m_internalMetrics;
};

}  // namespace Metrics
}  // namespace HubPlatform
