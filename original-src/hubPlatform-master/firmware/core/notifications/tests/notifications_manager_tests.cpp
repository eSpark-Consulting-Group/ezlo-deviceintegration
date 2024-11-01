#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "notifications/notifications_manager/notifications_manager.h"
#include "notifications/resources/notifications_resources.h"
#include "notifications/exceptions.h"

#include "json_utils/json_utils.h"
#include "crypto_utils/digests.h"
#include "database_client/fake_db/fake_db_client.h"
#include "database_client/dbqueryparser.h"
#include "common_utils/cpp14.h"
#include "base_utils/utils.h"
#include "system_utils/filesystem.h"
#include "system_utils/system.h"
#include "test_utils/temporary_folder_wrapper.h"

namespace HubPlatform {
namespace UT {

using namespace Notifications;

class FakeSMTPClient : public SMTPClientInterface {
public:
    struct Email {
        std::string sender;
        std::string recipient;
        std::string topic;
        std::string body;
        std::time_t date;
    };

    bool send( const std::string& sender,
               const std::string& recipient,
               const std::string& topic,
               const std::string& body,
               const std::time_t date ) override {
        ++m_callsNum;
        auto result = resultStrategy();
        if( result ) {
            std::lock_guard< std::mutex > lock( m_mutex );
            m_emails.emplace_back();
            auto& email = m_emails.back();
            email.sender = sender;
            email.recipient = recipient;
            email.topic = topic;
            email.body = body;
            email.date = date;
        }
        return result;
    }

    std::function< bool() > resultStrategy = []() { return true; };

    std::vector< Email > getEmails() {
        std::lock_guard< std::mutex > lock( m_mutex );
        return m_emails;
    }

    size_t getEmailsNum() {
        std::lock_guard< std::mutex > lock( m_mutex );
        return m_emails.size();
    }

    bool waitForEmails( size_t emailsNum ) {
        return waitForCondition( [this, emailsNum]() { return getEmailsNum() >= emailsNum; },
                                 std::chrono::seconds( 5 ),
                                 std::chrono::milliseconds( 50 ) );
    }

    size_t getCallsNum() {
        return m_callsNum;
    }

    bool waitForCalls( size_t callsNum ) {
        return waitForCondition( [this, callsNum]() { return getCallsNum() >= callsNum; },
                                 std::chrono::seconds( 5 ),
                                 std::chrono::milliseconds( 50 ) );
    }

private:
    std::mutex m_mutex;
    std::vector< Email > m_emails;
    std::atomic_size_t m_callsNum{ 0 };
};

class NotificationsManagerTest : public ::testing::Test {
public:
    void SetUp() override {
        m_dbClientPtr = std::make_unique< HubPlatform::UT::FakeDbClient >();
        m_certificatesDir = std::make_unique< Tests::TemporaryFolderWrapper >();
    }

    IDbClient& getDBClient() {
        return *m_dbClientPtr;
    }

    std::string getCertificatesDirPath() {
        return m_certificatesDir->getPath();
    }

    NotificationsManager::Settings getManagerSettings() {
        NotificationsManager::Settings settings;
        settings.certificatesDir = getCertificatesDirPath();
        return settings;
    }

    std::string smtpServerUrl() {
        return "smtps://server:port";
    }

    std::string smtpServerCACertificate() {
        return "<server-ca-certificate>";
    }

    std::string smtpClientCertificate() {
        return "<client-certificate>";
    }

    std::string smtpClientCertificateKey() {
        return "<client-certificate-key>";
    }

    std::string smtpClientUsername() {
        return "<client-username>";
    }

    std::string smtpClientPassword() {
        return "<client-password>";
    }

    Json smtpSettingsData() {
        namespace Fields = Resources::DB::Settings::FieldsSMTP;
        Json data;
        data[ Fields::SMTPServerUrl ] = smtpServerUrl();
        data[ Fields::Encryption ] = true;
        data[ Fields::VerifyServerCertificate ] = true;
        data[ Fields::VerifyServerHostname ] = false;
        data[ Fields::CertificatesVersion ] = 1;
        data[ Fields::ServerCACertificate ] = smtpServerCACertificate();
        data[ Fields::ClientCertificate ] = smtpClientCertificate();
        data[ Fields::ClientCertificateKey ] = smtpClientCertificateKey();
        data[ Fields::ClientUsername ] = smtpClientUsername();
        data[ Fields::ClientPassword ] = smtpClientPassword();

        return data;
    }

protected:
    std::unique_ptr< IDbClient > m_dbClientPtr;
    std::unique_ptr< Tests::TemporaryFolderWrapper > m_certificatesDir;
};

TEST_F( NotificationsManagerTest, initWithoutSMTPSettings ) {
    auto managerSettings = getManagerSettings();

    size_t factoryCalls = 0;
    NotificationsManager::SMTPClientFactory factory = [&factoryCalls]( SMTPClient::Settings s ) {
        ++factoryCalls;
        return nullptr;
    };

    { NotificationsManager manager( managerSettings, getDBClient(), factory ); }

    EXPECT_EQ( factoryCalls, 0 );
}

TEST_F( NotificationsManagerTest, initWithSMTPSettings ) {
    FakeSMTPClient* smtpClient = nullptr;
    auto managerSettings = getManagerSettings();

    size_t factoryCalls = 0;
    NotificationsManager::SMTPClientFactory factory = [&factoryCalls, &smtpClient]( SMTPClient::Settings s ) {
        ++factoryCalls;
        auto ptr = std::make_unique< FakeSMTPClient >();
        smtpClient = ptr.get();
        return ptr;
    };

    NotificationsManager manager( managerSettings, getDBClient(), factory );
    manager.updateSMTPSettings( smtpSettingsData() );

    ASSERT_EQ( factoryCalls, 1 );
    ASSERT_NE( smtpClient, nullptr );

    auto smtpSettings = manager.getSMTPClientSettings();
    EXPECT_EQ( smtpSettings.smtpServerUrl, smtpServerUrl() );
    EXPECT_EQ( smtpSettings.encryption, true );
    EXPECT_EQ( smtpSettings.verifyServerCertificateOrigin, true );
    EXPECT_EQ( smtpSettings.verifyServerHostname, false );
    EXPECT_EQ( smtpSettings.clientUsername, smtpClientUsername() );
    EXPECT_EQ( smtpSettings.clientPassword, smtpClientPassword() );
    EXPECT_FALSE( smtpSettings.serverCertificateAuthorityFile.empty() );
    EXPECT_FALSE( smtpSettings.clientCertificateFile.empty() );
    EXPECT_FALSE( smtpSettings.clientCertificateKeyFile.empty() );

    auto certificates = filesystem::findAllFiles( getCertificatesDirPath() );
    ASSERT_EQ( certificates.size(), 3 );

    EXPECT_EQ( System::readFile( smtpSettings.serverCertificateAuthorityFile ), smtpServerCACertificate() );
    EXPECT_EQ( System::readFile( smtpSettings.clientCertificateFile ), smtpClientCertificate() );
    EXPECT_EQ( System::readFile( smtpSettings.clientCertificateKeyFile ), smtpClientCertificateKey() );
}

TEST_F( NotificationsManagerTest, sendOneEmail ) {
    FakeSMTPClient* smtpClient = nullptr;
    auto managerSettings = getManagerSettings();
    managerSettings.smtpSendRetryLimit = 10;
    managerSettings.smtpSendInterval = std::chrono::milliseconds( 1 );

    NotificationsManager::SMTPClientFactory factory = [&smtpClient]( SMTPClient::Settings s ) {
        auto ptr = std::make_unique< FakeSMTPClient >();
        smtpClient = ptr.get();
        return ptr;
    };

    NotificationsManager manager( managerSettings, getDBClient(), factory );
    manager.updateSMTPSettings( smtpSettingsData() );

    ASSERT_NE( smtpClient, nullptr );
    manager.sendEmail( "sender@mail.none", "recipient@mail.none", "_TOPIC_", "_BODY_", 12345 );

    EXPECT_TRUE( smtpClient->waitForEmails( 1 ) );
    std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );

    ASSERT_EQ( smtpClient->getCallsNum(), 1 ) << "Email should be send only once";

    auto metrics = manager.getMetrics();
    EXPECT_EQ( metrics.emailsTotal, 1 );
    EXPECT_EQ( metrics.emailsSent, 1 );
    EXPECT_EQ( metrics.emailsFailed, 0 );
    EXPECT_EQ( metrics.emailsDropped, 0 );
    EXPECT_EQ( metrics.emailsPending, 0 );
}

TEST_F( NotificationsManagerTest, sendOneEmailAfterTwoFails ) {
    FakeSMTPClient* smtpClient = nullptr;
    auto managerSettings = getManagerSettings();
    managerSettings.smtpSendRetryLimit = 10;
    managerSettings.smtpSendInterval = std::chrono::milliseconds( 1 );

    NotificationsManager::SMTPClientFactory factory = [&smtpClient]( SMTPClient::Settings s ) {
        auto ptr = std::make_unique< FakeSMTPClient >();
        smtpClient = ptr.get();
        return ptr;
    };

    NotificationsManager manager( managerSettings, getDBClient(), factory );
    manager.updateSMTPSettings( smtpSettingsData() );

    ASSERT_NE( smtpClient, nullptr );

    int count = 0;
    smtpClient->resultStrategy = [&count]() { return ++count == 3; };  // Allow third attempt

    manager.sendEmail( "sender@mail.none", "recipient@mail.none", "_TOPIC_", "_BODY_", 12345 );

    EXPECT_TRUE( smtpClient->waitForCalls( 3 ) );
    EXPECT_EQ( smtpClient->getEmailsNum(), 1 ) << "Email should be send only once";

    auto metrics = manager.getMetrics();
    EXPECT_EQ( metrics.emailsTotal, 1 );
    EXPECT_EQ( metrics.emailsSent, 1 );
    EXPECT_EQ( metrics.emailsFailed, 0 );
    EXPECT_EQ( metrics.emailsDropped, 0 );
    EXPECT_EQ( metrics.emailsPending, 0 );
}

TEST_F( NotificationsManagerTest, emailSendFails ) {
    FakeSMTPClient* smtpClient = nullptr;
    auto managerSettings = getManagerSettings();
    managerSettings.smtpSendRetryLimit = 3;
    managerSettings.smtpSendInterval = std::chrono::milliseconds( 1 );

    NotificationsManager::SMTPClientFactory factory = [&smtpClient]( SMTPClient::Settings s ) {
        auto ptr = std::make_unique< FakeSMTPClient >();
        smtpClient = ptr.get();
        return ptr;
    };

    NotificationsManager manager( managerSettings, getDBClient(), factory );
    manager.updateSMTPSettings( smtpSettingsData() );

    ASSERT_NE( smtpClient, nullptr );

    smtpClient->resultStrategy = []() { return false; };

    manager.sendEmail( "sender@mail.none", "recipient@mail.none", "_TOPIC_", "_BODY_", 12345 );

    EXPECT_TRUE( smtpClient->waitForCalls( 3 ) );
    std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );

    EXPECT_EQ( smtpClient->getEmailsNum(), 0 );

    auto metrics = manager.getMetrics();
    EXPECT_EQ( metrics.emailsTotal, 1 );
    EXPECT_EQ( metrics.emailsSent, 0 );
    EXPECT_EQ( metrics.emailsFailed, 1 );
    EXPECT_EQ( metrics.emailsDropped, 0 );
    EXPECT_EQ( metrics.emailsPending, 0 );
}

TEST_F( NotificationsManagerTest, sendMultipleEmails ) {
    FakeSMTPClient* smtpClient = nullptr;
    auto managerSettings = getManagerSettings();
    managerSettings.smtpSendRetryLimit = 10;
    managerSettings.smtpSendInterval = std::chrono::milliseconds( 1 );

    NotificationsManager::SMTPClientFactory factory = [&smtpClient]( SMTPClient::Settings s ) {
        auto ptr = std::make_unique< FakeSMTPClient >();
        smtpClient = ptr.get();
        return ptr;
    };

    NotificationsManager manager( managerSettings, getDBClient(), factory );
    manager.updateSMTPSettings( smtpSettingsData() );

    ASSERT_NE( smtpClient, nullptr );
    manager.sendEmail( "sender@mail.none", "recipient@mail.none", "_TOPIC_1_", "_BODY_1_", 12345 );
    manager.sendEmail( "sender@mail.none", "recipient@mail.none", "_TOPIC_2_", "_BODY_2_", 12345 );

    EXPECT_TRUE( smtpClient->waitForEmails( 2 ) );
    std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );

    ASSERT_EQ( smtpClient->getCallsNum(), 2 );
    auto emails = smtpClient->getEmails();
    ASSERT_EQ( emails.size(), 2 );

    EXPECT_EQ( emails.at( 0 ).body, "_BODY_1_" );
    EXPECT_EQ( emails.at( 1 ).body, "_BODY_2_" );

    auto metrics = manager.getMetrics();
    EXPECT_EQ( metrics.emailsTotal, 2 );
    EXPECT_EQ( metrics.emailsSent, 2 );
    EXPECT_EQ( metrics.emailsFailed, 0 );
    EXPECT_EQ( metrics.emailsDropped, 0 );
    EXPECT_EQ( metrics.emailsPending, 0 );
}

TEST_F( NotificationsManagerTest, sendPendingEmails ) {
    FakeSMTPClient* smtpClient = nullptr;
    auto managerSettings = getManagerSettings();
    managerSettings.smtpSendRetryLimit = 10;
    managerSettings.smtpSendInterval = std::chrono::seconds( 30 );

    NotificationsManager::SMTPClientFactory factory = [&smtpClient]( SMTPClient::Settings s ) {
        auto ptr = std::make_unique< FakeSMTPClient >();
        smtpClient = ptr.get();
        return ptr;
    };

    NotificationsManager manager( managerSettings, getDBClient(), factory );

    // NOTE: SMTP client settings are still not provided, thus emails are pending
    manager.sendEmail( "sender@mail.none", "recipient@mail.none", "_TOPIC_1_", "_BODY_1_", 12345 );
    manager.sendEmail( "sender@mail.none", "recipient@mail.none", "_TOPIC_2_", "_BODY_2_", 12345 );

    EXPECT_EQ( manager.getMetrics().emailsPending, 2 );

    manager.updateSMTPSettings( smtpSettingsData() );
    ASSERT_NE( smtpClient, nullptr );
    auto beg = std::chrono::steady_clock::now();

    EXPECT_TRUE( smtpClient->waitForEmails( 2 ) );
    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast< std::chrono::milliseconds >( end - beg );

    EXPECT_LT( duration.count(), managerSettings.smtpSendInterval.count() )
        << "Pending emails must be send right after client is initialized";
    EXPECT_EQ( smtpClient->getEmailsNum(), 2 );

    auto metrics = manager.getMetrics();
    EXPECT_EQ( metrics.emailsTotal, 2 );
    EXPECT_EQ( metrics.emailsSent, 2 );
    EXPECT_EQ( metrics.emailsFailed, 0 );
    EXPECT_EQ( metrics.emailsDropped, 0 );
    EXPECT_EQ( metrics.emailsPending, 0 );
}

TEST_F( NotificationsManagerTest, pendingEmailsOverflow ) {
    FakeSMTPClient* smtpClient = nullptr;
    auto managerSettings = getManagerSettings();
    managerSettings.smtpSendRetryLimit = 2;
    managerSettings.smtpSendInterval = std::chrono::milliseconds( 1 );
    managerSettings.smtpSendPendingLimit = 10;

    NotificationsManager::SMTPClientFactory factory = [&smtpClient]( SMTPClient::Settings s ) {
        auto ptr = std::make_unique< FakeSMTPClient >();
        smtpClient = ptr.get();
        return ptr;
    };

    NotificationsManager manager( managerSettings, getDBClient(), factory );

    const size_t totalEmails = 15;
    for( size_t i = 0; i < totalEmails; ++i ) {
        manager.sendEmail( "sender@mail.none", "recipient@mail.none", "_TOPIC_", "_BODY_", 12345 );
    }

    // NOTE: SMTP client settings are still not provided, thus emails are pending
    manager.updateSMTPSettings( smtpSettingsData() );
    ASSERT_NE( smtpClient, nullptr );

    EXPECT_TRUE( smtpClient->waitForEmails( 10 ) );
    std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
    EXPECT_EQ( smtpClient->getEmailsNum(), 10 );

    auto metrics = manager.getMetrics();
    EXPECT_EQ( metrics.emailsTotal, 15 );
    EXPECT_EQ( metrics.emailsSent, 10 );
    EXPECT_EQ( metrics.emailsFailed, 0 );
    EXPECT_EQ( metrics.emailsDropped, 5 );
    EXPECT_EQ( metrics.emailsPending, 0 );
}

}  // namespace UT
}  // namespace HubPlatform
