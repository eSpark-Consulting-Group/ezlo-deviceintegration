#include <gtest/gtest.h>

#include "smtp_client/smtp_client.h"

#include "test_utils/test_environment.h"
#include "system_utils/find_free_local_port.h"
#include "system_utils/process/shared_mutex.h"
#include "system_utils/filesystem.h"
#include "system_utils/process/process_reader.h"
#include "common_utils/cpp14.h"
#include "base_utils/utils.h"
#include "base_utils/string_utils.h"

#include <memory>
#include <thread>
#include <chrono>

#include <cstdlib>

/*

TESTPLAN

DONE    1. SMTP Client tests
DONE        1.1  Without encryption, without auth
DONE        1.2  Without encryption, client authorized
DONE        1.3  Without encryption, invalid auth
DONE        1.4  Encrypted: without verification, without auth
DONE        1.5  Encrypted: client rejects unencrypted server
DONE        1.6  Encrypted: server verified, without auth
DONE        1.7  Encrypted: client rejects fake server
DONE        1.8  Encrypted: server verified, client authorized
DONE        1.9  Encrypted: server verified, invalid auth
DONE        1.10 Encrypted: server verified, client verified, without auth
DONE        1.11 Encrypted: server rejects fake client
DONE        1.12 Encrypted: server verified, client verified, client authorized
DONE        1.13 Encrypted: server verified, client verified, invalid auth

*/

namespace HubPlatform {
namespace UT {
namespace {

const std::string EmailSender = "sender@email";
const std::string EmailRecipient = "recipient@email";
const std::string EmailTopic = "TEST_EMAIL_TOPIC";
const std::string EmailBody = "TEST_EMAIL_BODY";
const std::time_t EmailDate = 1000;

std::string getSmtpServerRoot() {
#ifndef SMTP_SERVER_ROOT
#error Macro SMTP_SERVER_ROOT is not defined. Fix compile definitions
#endif
    return SMTP_SERVER_ROOT;
}

std::string getCertificatesRoot() {
#ifndef SMTP_CERTIFICATES
#error Macro SMTP_CERTIFICATES is not defined. Fix compile definitions
#endif
    return SMTP_CERTIFICATES;
}

std::string getRandomServerPort() {
    Process::SharedMutex processMutex( "uniquePortMutex" );
    std::unique_lock< Process::SharedMutex > lock( processMutex );
    auto port = system_utils::getUniqueFreeLocalPort();
    lock.unlock();
    return std::to_string( port );
}

class SMTPServerException : public std::runtime_error {
public:
    using runtime_error::runtime_error;
    explicit SMTPServerException( const std::string& msg )
    : runtime_error( msg ){};
};

class SMTPServer {
public:
    struct Settings {
        std::string bindAddress = "0.0.0.0";
        std::string port = "9020";
        std::string serverCertificatePath;
        std::string serverCertificateKeyPath;
        std::string clientAuthorityCertificatePath;
        std::string clientUsername;
        std::string clientPassword;
    };

    SMTPServer( const Settings& settings ) {
        if( !filesystem::isDirectoryExist( getSmtpServerRoot() ) ) {
            throw SMTPServerException( "Server root directory not found: " + getSmtpServerRoot() );
        }

        setenv( "PYTHONPATH", getSmtpServerRoot().c_str(), true /*overwrite*/ );
        std::stringstream cmd;
        cmd << "python3 -B -u -m smtp_server";
        cmd << " --bind " << settings.bindAddress;
        cmd << " --port " << settings.port;

        if( !settings.serverCertificatePath.empty() && !settings.serverCertificateKeyPath.empty() ) {
            if( !filesystem::isExist( settings.serverCertificatePath ) ) {
                throw SMTPServerException( "Server certificate not found: " + settings.serverCertificatePath );
            }

            if( !filesystem::isExist( settings.serverCertificateKeyPath ) ) {
                throw SMTPServerException( "Server certificate key not found: " + settings.serverCertificateKeyPath );
            }

            cmd << " --server-cert " << settings.serverCertificatePath;
            cmd << " --server-cert-key " << settings.serverCertificateKeyPath;
        }

        if( !settings.clientAuthorityCertificatePath.empty() ) {
            if( !filesystem::isExist( settings.clientAuthorityCertificatePath ) ) {
                throw SMTPServerException( "Client authority certificate not found: " +
                                           settings.clientAuthorityCertificatePath );
            }

            cmd << " --client-ca-cert " << settings.clientAuthorityCertificatePath;
        }

        if( !settings.clientUsername.empty() && !settings.clientPassword.empty() ) {
            cmd << " --client-username " << settings.clientUsername;
            cmd << " --client-password " << settings.clientPassword;
        }

        auto stdoutEvent = [this]( const char* bytes, size_t n ) {
            std::lock_guard< std::mutex > lock( m_mutex );
            m_output.append( bytes, n );
            captureMessages();
        };

        auto stderrEvent = [this]( const char* bytes, size_t n ) {
            std::lock_guard< std::mutex > lock( m_mutex );
            m_output.append( bytes, n );
        };

        m_processReader = std::make_unique< Process::ProcessReader >( cmd.str(), stdoutEvent, stderrEvent );

        auto serverStarted = [this]() {
            std::lock_guard< std::mutex > lock( m_mutex );
            return !m_processReader->isFinished() && m_output.find( "SERVER_STARTED" ) != std::string::npos;
        };

        if( !waitForCondition( serverStarted, std::chrono::seconds( 10 ), std::chrono::milliseconds( 50 ) ) ) {
            std::unique_lock< std::mutex > lock( m_mutex );
            ADD_FAILURE() << "Unable to start SMTP server: " << m_output;
            lock.unlock();
            exit( EXIT_FAILURE );
        }
    }

    ~SMTPServer() {
        m_processReader->stop();
    }

    std::string getOutput() {
        std::lock_guard< std::mutex > lock( m_mutex );
        return m_output;
    }

    size_t authCompletedCount() {
        std::lock_guard< std::mutex > lock( m_mutex );
        return StringUtils::countTokens( m_output, "AUTH_COMPLETED" );
    }

    size_t authRejectedCount() {
        std::lock_guard< std::mutex > lock( m_mutex );
        return StringUtils::countTokens( m_output, "AUTH_REJECTED" );
    }

    std::vector< std::string > getMessages() {
        std::lock_guard< std::mutex > lock( m_mutex );
        return m_messages;
    }

    static bool matchMessage( const std::string& message,
                              const std::string& sender,
                              const std::string& recipient,
                              const std::string& subject,
                              const std::string& body ) {
        if( message.find( "From: " + sender ) == std::string::npos )
            return false;
        if( message.find( "To: " + recipient ) == std::string::npos )
            return false;
        if( message.find( "Subject: " + subject ) == std::string::npos )
            return false;
        if( message.find( body ) == std::string::npos )
            return false;
        return true;
    }

private:
    void captureMessages() {
        //---------- MESSAGE FOLLOWS ----------
        // Date: Wed, 21 Aug 2024 18:24:24 +0300
        // To: receiver@mail.none
        // From: sender@mail.none
        // Subject: TOPIC
        // X-Peer: ('127.0.0.1', 43740)
        //
        // BODY
        //------------ END MESSAGE ------------

        auto beg = m_output.find( "---------- MESSAGE FOLLOWS ----------", m_lastMessageBeg );
        auto end = m_output.find( "------------ END MESSAGE ------------", m_lastMessageEnd );

        if( beg == std::string::npos || end == std::string::npos )
            return;

        if( end != m_lastMessageEnd ) {
            m_lastMessageBeg = beg;
            m_lastMessageEnd = end;

            beg = m_output.find( '\n', beg );
            m_messages.emplace_back( m_output.substr( beg + 1, end ) );
        }
    }

private:
    std::unique_ptr< Process::ProcessReader > m_processReader;
    std::mutex m_mutex;
    std::string m_output;
    std::vector< std::string > m_messages;

    size_t m_lastMessageBeg = 0;
    size_t m_lastMessageEnd = 0;
};

}  // namespace

class SMTPClientTests : public ::testing::Test {
public:
    std::string certificateAuthority() {
        return getCertificatesRoot() + "/ca_cert.pem";
    }

    std::string serverCertificate() {
        return getCertificatesRoot() + "/server_cert.pem";
    }

    std::string serverCertificateKey() {
        return getCertificatesRoot() + "/server_key.pem";
    }

    std::string clientCertificate() {
        return getCertificatesRoot() + "/client_cert.pem";
    }

    std::string clientCertificateKey() {
        return getCertificatesRoot() + "/client_key.pem";
    }

    std::string fakeServerCertificate() {
        return getCertificatesRoot() + "/fake_server_cert.pem";
    }

    std::string fakeServerCertificateKey() {
        return getCertificatesRoot() + "/fake_server_key.pem";
    }

    std::string fakeClientCertificate() {
        return getCertificatesRoot() + "/fake_client_cert.pem";
    }

    std::string fakeClientCertificateKey() {
        return getCertificatesRoot() + "/fake_client_key.pem";
    }

    bool waitForMessages( SMTPServer& server,
                          size_t messagesCount,
                          std::chrono::milliseconds timeout = std::chrono::seconds( 3 ) ) {
        return waitForCondition( [&server, &messagesCount]() { return server.getMessages().size() >= messagesCount; },
                                 timeout,
                                 std::chrono::milliseconds( 50 ) );
    }
};

TEST_F( SMTPClientTests, WithoutEncryptionWithoutAuth ) /* 1.1 */ {
    SMTPServer::Settings serverSettings;
    serverSettings.bindAddress = "127.0.0.1";
    serverSettings.port = getRandomServerPort();

    SMTPClient::Settings clientSettings;
    clientSettings.verbose = true;
    clientSettings.encryption = false;
    clientSettings.smtpServerUrl = "smtp://" + serverSettings.bindAddress + ":" + serverSettings.port;

    SMTPServer server( serverSettings );
    SMTPClient client( clientSettings );

    ASSERT_TRUE( client.send( EmailSender, EmailRecipient, EmailTopic, EmailBody, EmailDate ) )
        << "Server output: " << server.getOutput();
    ASSERT_TRUE( waitForMessages( server, 1 ) ) << "Server output: " << server.getOutput();
    ASSERT_TRUE(
        SMTPServer::matchMessage( server.getMessages().at( 0 ), EmailSender, EmailRecipient, EmailTopic, EmailBody ) )
        << "Server output: " << server.getOutput();
}

TEST_F( SMTPClientTests, WithoutEncryptedWithAuth ) /* 1.2 */ {
    SMTPServer::Settings serverSettings;
    serverSettings.bindAddress = "127.0.0.1";
    serverSettings.port = getRandomServerPort();
    serverSettings.clientUsername = "client.username";
    serverSettings.clientPassword = "client.password";

    SMTPClient::Settings clientSettings;
    clientSettings.verbose = true;
    clientSettings.encryption = false;
    clientSettings.smtpServerUrl = "smtp://" + serverSettings.bindAddress + ":" + serverSettings.port;
    clientSettings.clientUsername = serverSettings.clientUsername;
    clientSettings.clientPassword = serverSettings.clientPassword;

    SMTPServer server( serverSettings );
    SMTPClient client( clientSettings );

    ASSERT_TRUE( client.send( EmailSender, EmailRecipient, EmailTopic, EmailBody, EmailDate ) )
        << "Server output: " << server.getOutput();
    ASSERT_TRUE( waitForMessages( server, 1 ) ) << "Server output: " << server.getOutput();
    ASSERT_TRUE(
        SMTPServer::matchMessage( server.getMessages().at( 0 ), EmailSender, EmailRecipient, EmailTopic, EmailBody ) )
        << "Server output: " << server.getOutput();

    ASSERT_EQ( server.authCompletedCount(), 1 );
    ASSERT_EQ( server.authRejectedCount(), 0 );
}

TEST_F( SMTPClientTests, WithoutEncryptedInvalidAuth ) /* 1.3 */ {
    SMTPServer::Settings serverSettings;
    serverSettings.bindAddress = "127.0.0.1";
    serverSettings.port = getRandomServerPort();
    serverSettings.clientUsername = "client.username";
    serverSettings.clientPassword = "client.password";

    SMTPClient::Settings clientSettings;
    clientSettings.verbose = true;
    clientSettings.encryption = false;
    clientSettings.smtpServerUrl = "smtp://" + serverSettings.bindAddress + ":" + serverSettings.port;
    clientSettings.clientUsername = "invalid.username";
    clientSettings.clientPassword = "invalid.password";

    SMTPServer server( serverSettings );
    SMTPClient client( clientSettings );

    ASSERT_FALSE( client.send( EmailSender, EmailRecipient, EmailTopic, EmailBody, EmailDate ) )
        << "Server output: " << server.getOutput();
    ASSERT_TRUE( waitForCondition( [&server]() { return server.authRejectedCount() == 1; },
                                   std::chrono::seconds( 3 ),
                                   std::chrono::milliseconds( 50 ) ) );
    ASSERT_EQ( server.authCompletedCount(), 0 );
}

TEST_F( SMTPClientTests, WithoutVerificationWithoutAuth ) /* 1.4 */ {
    SMTPServer::Settings serverSettings;
    serverSettings.bindAddress = "127.0.0.1";
    serverSettings.port = getRandomServerPort();
    serverSettings.serverCertificatePath = serverCertificate();
    serverSettings.serverCertificateKeyPath = serverCertificateKey();

    SMTPClient::Settings clientSettings;
    clientSettings.verbose = true;
    clientSettings.encryption = true;
    clientSettings.verifyServerHostname = false;
    clientSettings.verifyServerCertificateOrigin = false;
    clientSettings.smtpServerUrl = "smtps://" + serverSettings.bindAddress + ":" + serverSettings.port;

    SMTPServer server( serverSettings );
    SMTPClient client( clientSettings );

    ASSERT_TRUE( client.send( EmailSender, EmailRecipient, EmailTopic, EmailBody, EmailDate ) )
        << "Server output: " << server.getOutput();
    ASSERT_TRUE( waitForMessages( server, 1 ) ) << "Server output: " << server.getOutput();
    ASSERT_TRUE(
        SMTPServer::matchMessage( server.getMessages().at( 0 ), EmailSender, EmailRecipient, EmailTopic, EmailBody ) )
        << "Server output: " << server.getOutput();
}

TEST_F( SMTPClientTests, ClientRejectsUnencryptedServer ) /* 1.5 */ {
    SMTPServer::Settings serverSettings;
    serverSettings.bindAddress = "127.0.0.1";
    serverSettings.port = getRandomServerPort();

    SMTPClient::Settings clientSettings;
    clientSettings.verbose = true;
    clientSettings.encryption = true;
    clientSettings.verifyServerHostname = false;
    clientSettings.verifyServerCertificateOrigin = false;
    clientSettings.smtpServerUrl = "smtps://" + serverSettings.bindAddress + ":" + serverSettings.port;

    SMTPServer server( serverSettings );
    SMTPClient client( clientSettings );

    ASSERT_FALSE( client.send( EmailSender, EmailRecipient, EmailTopic, EmailBody, EmailDate ) )
        << "Server output: " << server.getOutput();
    ASSERT_EQ( server.getMessages().size(), 0 ) << "Server output: " << server.getOutput();
}

TEST_F( SMTPClientTests, ServerVerifiedWithoutAuth ) /* 1.6 */ {
    SMTPServer::Settings serverSettings;
    serverSettings.bindAddress = "127.0.0.1";
    serverSettings.port = getRandomServerPort();
    serverSettings.serverCertificatePath = serverCertificate();
    serverSettings.serverCertificateKeyPath = serverCertificateKey();

    SMTPClient::Settings clientSettings;
    clientSettings.verbose = true;
    clientSettings.encryption = true;
    clientSettings.verifyServerHostname = false;
    clientSettings.verifyServerCertificateOrigin = true;
    clientSettings.serverCertificateAuthorityFile = certificateAuthority();
    clientSettings.smtpServerUrl = "smtps://" + serverSettings.bindAddress + ":" + serverSettings.port;

    SMTPServer server( serverSettings );
    SMTPClient client( clientSettings );

    ASSERT_TRUE( client.send( EmailSender, EmailRecipient, EmailTopic, EmailBody, EmailDate ) )
        << "Server output: " << server.getOutput();
    ASSERT_TRUE( waitForMessages( server, 1 ) ) << "Server output: " << server.getOutput();
    ASSERT_TRUE(
        SMTPServer::matchMessage( server.getMessages().at( 0 ), EmailSender, EmailRecipient, EmailTopic, EmailBody ) )
        << "Server output: " << server.getOutput();
}

TEST_F( SMTPClientTests, ClientRejectsFakeServer ) /* 1.7 */ {
    SMTPServer::Settings serverSettings;
    serverSettings.bindAddress = "127.0.0.1";
    serverSettings.port = getRandomServerPort();
    serverSettings.serverCertificatePath = fakeServerCertificate();
    serverSettings.serverCertificateKeyPath = fakeServerCertificateKey();

    SMTPClient::Settings clientSettings;
    clientSettings.verbose = true;
    clientSettings.encryption = true;
    clientSettings.verifyServerHostname = false;
    clientSettings.verifyServerCertificateOrigin = true;
    clientSettings.serverCertificateAuthorityFile = certificateAuthority();
    clientSettings.smtpServerUrl = "smtps://" + serverSettings.bindAddress + ":" + serverSettings.port;

    SMTPServer server( serverSettings );
    SMTPClient client( clientSettings );

    ASSERT_FALSE( client.send( EmailSender, EmailRecipient, EmailTopic, EmailBody, EmailDate ) )
        << "Server output: " << server.getOutput();
    ASSERT_EQ( server.getMessages().size(), 0 ) << "Server output: " << server.getOutput();
}

TEST_F( SMTPClientTests, ServerVerifiedClientAuthorized ) /* 1.8 */ {
    SMTPServer::Settings serverSettings;
    serverSettings.bindAddress = "127.0.0.1";
    serverSettings.port = getRandomServerPort();
    serverSettings.serverCertificatePath = serverCertificate();
    serverSettings.serverCertificateKeyPath = serverCertificateKey();
    serverSettings.clientUsername = "client.username";
    serverSettings.clientPassword = "client.password";

    SMTPClient::Settings clientSettings;
    clientSettings.verbose = true;
    clientSettings.encryption = true;
    clientSettings.verifyServerHostname = false;
    clientSettings.verifyServerCertificateOrigin = true;
    clientSettings.serverCertificateAuthorityFile = certificateAuthority();
    clientSettings.clientUsername = serverSettings.clientUsername;
    clientSettings.clientPassword = serverSettings.clientPassword;
    clientSettings.smtpServerUrl = "smtps://" + serverSettings.bindAddress + ":" + serverSettings.port;

    SMTPServer server( serverSettings );
    SMTPClient client( clientSettings );

    ASSERT_TRUE( client.send( EmailSender, EmailRecipient, EmailTopic, EmailBody, EmailDate ) )
        << "Server output: " << server.getOutput();
    ASSERT_TRUE( waitForMessages( server, 1 ) ) << "Server output: " << server.getOutput();
    ASSERT_TRUE(
        SMTPServer::matchMessage( server.getMessages().at( 0 ), EmailSender, EmailRecipient, EmailTopic, EmailBody ) )
        << "Server output: " << server.getOutput();
}

TEST_F( SMTPClientTests, ServerVerifiedInvalidAuth ) /* 1.9 */ {
    SMTPServer::Settings serverSettings;
    serverSettings.bindAddress = "127.0.0.1";
    serverSettings.port = getRandomServerPort();
    serverSettings.serverCertificatePath = serverCertificate();
    serverSettings.serverCertificateKeyPath = serverCertificateKey();
    serverSettings.clientUsername = "client.username";
    serverSettings.clientPassword = "client.password";

    SMTPClient::Settings clientSettings;
    clientSettings.verbose = true;
    clientSettings.encryption = true;
    clientSettings.verifyServerHostname = false;
    clientSettings.verifyServerCertificateOrigin = true;
    clientSettings.serverCertificateAuthorityFile = certificateAuthority();
    clientSettings.clientUsername = "invalid.username";
    clientSettings.clientPassword = "invalid.password";
    clientSettings.smtpServerUrl = "smtps://" + serverSettings.bindAddress + ":" + serverSettings.port;

    SMTPServer server( serverSettings );
    SMTPClient client( clientSettings );

    ASSERT_FALSE( client.send( EmailSender, EmailRecipient, EmailTopic, EmailBody, EmailDate ) )
        << "Server output: " << server.getOutput();
    ASSERT_TRUE( waitForCondition( [&server]() { return server.authRejectedCount() == 1; },
                                   std::chrono::seconds( 3 ),
                                   std::chrono::milliseconds( 50 ) ) );
    ASSERT_EQ( server.authCompletedCount(), 0 );
}

TEST_F( SMTPClientTests, MutuallyVerifiedWithoutAuth ) /* 1.10 */ {
    SMTPServer::Settings serverSettings;
    serverSettings.bindAddress = "127.0.0.1";
    serverSettings.port = getRandomServerPort();
    serverSettings.serverCertificatePath = serverCertificate();
    serverSettings.serverCertificateKeyPath = serverCertificateKey();
    serverSettings.clientAuthorityCertificatePath = certificateAuthority();

    SMTPClient::Settings clientSettings;
    clientSettings.verbose = true;
    clientSettings.encryption = true;
    clientSettings.verifyServerHostname = false;
    clientSettings.verifyServerCertificateOrigin = true;
    clientSettings.serverCertificateAuthorityFile = certificateAuthority();
    clientSettings.clientCertificateFile = clientCertificate();
    clientSettings.clientCertificateKeyFile = clientCertificateKey();
    clientSettings.smtpServerUrl = "smtps://" + serverSettings.bindAddress + ":" + serverSettings.port;

    SMTPServer server( serverSettings );
    SMTPClient client( clientSettings );

    ASSERT_TRUE( client.send( EmailSender, EmailRecipient, EmailTopic, EmailBody, EmailDate ) )
        << "Server output: " << server.getOutput();
    ASSERT_TRUE( waitForMessages( server, 1 ) ) << "Server output: " << server.getOutput();
    ASSERT_TRUE(
        SMTPServer::matchMessage( server.getMessages().at( 0 ), EmailSender, EmailRecipient, EmailTopic, EmailBody ) )
        << "Server output: " << server.getOutput();
}

TEST_F( SMTPClientTests, FakeClientRejected ) /* 1.11 */ {
    SMTPServer::Settings serverSettings;
    serverSettings.bindAddress = "127.0.0.1";
    serverSettings.port = getRandomServerPort();
    serverSettings.serverCertificatePath = serverCertificate();
    serverSettings.serverCertificateKeyPath = serverCertificateKey();
    serverSettings.clientAuthorityCertificatePath = certificateAuthority();

    SMTPClient::Settings clientSettings;
    clientSettings.verbose = true;
    clientSettings.encryption = true;
    clientSettings.verifyServerHostname = false;
    clientSettings.verifyServerCertificateOrigin = true;
    clientSettings.serverCertificateAuthorityFile = certificateAuthority();
    clientSettings.clientCertificateFile = fakeClientCertificate();
    clientSettings.clientCertificateKeyFile = fakeClientCertificateKey();
    clientSettings.smtpServerUrl = "smtps://" + serverSettings.bindAddress + ":" + serverSettings.port;

    SMTPServer server( serverSettings );
    SMTPClient client( clientSettings );

    ASSERT_FALSE( client.send( EmailSender, EmailRecipient, EmailTopic, EmailBody, EmailDate ) )
        << "Server output: " << server.getOutput();
    ASSERT_EQ( server.getMessages().size(), 0 ) << "Server output: " << server.getOutput();
}

TEST_F( SMTPClientTests, MutuallyVerifiedClientAuthorized ) /* 1.12 */ {
    SMTPServer::Settings serverSettings;
    serverSettings.bindAddress = "127.0.0.1";
    serverSettings.port = getRandomServerPort();
    serverSettings.serverCertificatePath = serverCertificate();
    serverSettings.serverCertificateKeyPath = serverCertificateKey();
    serverSettings.clientAuthorityCertificatePath = certificateAuthority();
    serverSettings.clientUsername = "client.username";
    serverSettings.clientPassword = "client.password";

    SMTPClient::Settings clientSettings;
    clientSettings.verbose = true;
    clientSettings.encryption = true;
    clientSettings.verifyServerHostname = false;
    clientSettings.verifyServerCertificateOrigin = true;
    clientSettings.serverCertificateAuthorityFile = certificateAuthority();
    clientSettings.clientCertificateFile = clientCertificate();
    clientSettings.clientCertificateKeyFile = clientCertificateKey();
    clientSettings.clientUsername = serverSettings.clientUsername;
    clientSettings.clientPassword = serverSettings.clientPassword;
    clientSettings.smtpServerUrl = "smtps://" + serverSettings.bindAddress + ":" + serverSettings.port;

    SMTPServer server( serverSettings );
    SMTPClient client( clientSettings );

    ASSERT_TRUE( client.send( EmailSender, EmailRecipient, EmailTopic, EmailBody, EmailDate ) )
        << "Server output: " << server.getOutput();
    ASSERT_TRUE( waitForMessages( server, 1 ) ) << "Server output: " << server.getOutput();
    ASSERT_TRUE(
        SMTPServer::matchMessage( server.getMessages().at( 0 ), EmailSender, EmailRecipient, EmailTopic, EmailBody ) )
        << "Server output: " << server.getOutput();
}

TEST_F( SMTPClientTests, MutuallyVerifiedInvalidAuth ) /* 1.13 */ {
    SMTPServer::Settings serverSettings;
    serverSettings.bindAddress = "127.0.0.1";
    serverSettings.port = getRandomServerPort();
    serverSettings.serverCertificatePath = serverCertificate();
    serverSettings.serverCertificateKeyPath = serverCertificateKey();
    serverSettings.clientAuthorityCertificatePath = certificateAuthority();
    serverSettings.clientUsername = "client.username";
    serverSettings.clientPassword = "client.password";

    SMTPClient::Settings clientSettings;
    clientSettings.verbose = true;
    clientSettings.encryption = true;
    clientSettings.verifyServerHostname = false;
    clientSettings.verifyServerCertificateOrigin = true;
    clientSettings.serverCertificateAuthorityFile = certificateAuthority();
    clientSettings.clientCertificateFile = clientCertificate();
    clientSettings.clientCertificateKeyFile = clientCertificateKey();
    clientSettings.clientUsername = "invalid.username";
    clientSettings.clientPassword = "invalid.password";
    clientSettings.smtpServerUrl = "smtps://" + serverSettings.bindAddress + ":" + serverSettings.port;

    SMTPServer server( serverSettings );
    SMTPClient client( clientSettings );

    ASSERT_FALSE( client.send( EmailSender, EmailRecipient, EmailTopic, EmailBody, EmailDate ) )
        << "Server output: " << server.getOutput();
    ASSERT_TRUE( waitForCondition( [&server]() { return server.authRejectedCount() == 1; },
                                   std::chrono::seconds( 3 ),
                                   std::chrono::milliseconds( 50 ) ) );
    ASSERT_EQ( server.authCompletedCount(), 0 );
}

}  // namespace UT
}  // namespace HubPlatform
