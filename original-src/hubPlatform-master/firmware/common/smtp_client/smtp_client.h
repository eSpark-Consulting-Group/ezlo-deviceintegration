#pragma once

#include <string>
#include <ctime>
#include <chrono>
#include <stdexcept>

namespace HubPlatform {

class SMTPClientError : public std::runtime_error {
public:
    using runtime_error::runtime_error;
    explicit SMTPClientError( const std::string& msg )
    : runtime_error( msg ){};
};

class SMTPClientSettingsError : public SMTPClientError {
public:
    explicit SMTPClientSettingsError( const std::string& msg )
    : SMTPClientError( msg ){};
};

class SMTPClientInterface {
public:
    virtual bool send( const std::string& sender,
                       const std::string& recipient,
                       const std::string& topic,
                       const std::string& body,
                       const std::time_t date ) = 0;
};

class SMTPClient : public SMTPClientInterface {
public:
    struct Settings {
        // Server URL: smtp://hostname:port or smtps://hostname:port
        std::string smtpServerUrl;

        // Use TLS encryption
        bool encryption = true;

        // Server certificate must be signed by ca-certificate known to client
        bool verifyServerCertificateOrigin = true;

        // Optional: Check if server certificate is signed by this x509 ca-certificate
        std::string serverCertificateAuthorityFile;

        // Actual server hostname must be mentioned in the CommonName (CN)
        // or SubjectAlternativeName (SAN) field of it's certificate
        bool verifyServerHostname = true;

        // Optional: Use client's x509 certificate (signed by ca-certificate known to server)
        // to allow SMTP server to vefiry the client
        std::string clientCertificateFile;
        std::string clientCertificateKeyFile;

        // Optional:
        std::string clientUsername;
        std::string clientPassword;

        std::chrono::milliseconds timeout{ 10000 };
        bool verbose = false;
    };

    SMTPClient( Settings settings );
    bool send( const std::string& sender,
               const std::string& recipient,
               const std::string& topic,
               const std::string& body,
               const std::time_t date ) override;

private:
    Settings m_settings;
};

}  // namespace HubPlatform
