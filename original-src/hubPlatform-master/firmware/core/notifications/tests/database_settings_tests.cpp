#include <gtest/gtest.h>
#include "notifications/database_settings/database_settings.h"
#include "notifications/resources/notifications_resources.h"
#include "notifications/exceptions.h"

#include "json_utils/json_utils.h"
#include "crypto_utils/digests.h"
#include "database_client/fake_db/fake_db_client.h"
#include "database_client/dbqueryparser.h"
#include "common_utils/cpp14.h"

namespace HubPlatform {
namespace UT {

using namespace Notifications;

class DatabaseSettingsTest : public ::testing::Test {
public:
    void SetUp() override {
        m_dbClientPtr = std::make_unique< HubPlatform::UT::FakeDbClient >();
    }

    IDbClient& getDBClient() {
        return *m_dbClientPtr;
    }

    Json smtpSettingsTestData() {
        namespace Fields = Resources::DB::Settings::FieldsSMTP;
        Json data;
        data[ Fields::SMTPServerUrl ] = "smtps://server:port";
        data[ Fields::Encryption ] = true;
        data[ Fields::VerifyServerCertificate ] = true;
        data[ Fields::VerifyServerHostname ] = true;
        data[ Fields::CertificatesVersion ] = 1;
        data[ Fields::ServerCACertificate ] = "<server-ca-certificate>";
        data[ Fields::ClientCertificate ] = "<client-certificate>";
        data[ Fields::ClientCertificateKey ] = "<client-certificate>";
        data[ Fields::ClientUsername ] = "<client-username>";
        data[ Fields::ClientPassword ] = "<client-password>";

        return data;
    }

protected:
    std::unique_ptr< IDbClient > m_dbClientPtr;
};

TEST_F( DatabaseSettingsTest, smtpWriteAndReadFullData ) {
    namespace Fields = Resources::DB::Settings::FieldsSMTP;
    const auto data = smtpSettingsTestData();

    DatabaseSettings dbSettings( getDBClient() );

    EXPECT_NO_THROW( dbSettings.setSMTPSettings( data ) );
    auto retrievedData = dbSettings.getSMTPSettings();

    EXPECT_EQ( data, retrievedData );

    auto rawDbData = getDBClient().findOne(
        HaNotificationsSettingsDbName, JSON( db::ID_FIELD_NAME << Resources::DB::Settings::SMTPSettingsId ), nullptr );

    EXPECT_FALSE( rawDbData.empty() );

    EXPECT_NE( json_get_str( rawDbData, Fields::ClientUsername ), json_get_str( data, Fields::ClientUsername ) )
        << "Fields must be encrypted";
    EXPECT_NE( json_get_str( rawDbData, Fields::ClientPassword ), json_get_str( data, Fields::ClientPassword ) )
        << "Fields must be encrypted";
    EXPECT_NE( json_get_str( rawDbData, Fields::ServerCACertificate ),
               json_get_str( data, Fields::ServerCACertificate ) )
        << "Fields must be encrypted";
    EXPECT_NE( json_get_str( rawDbData, Fields::ClientCertificate ), json_get_str( data, Fields::ClientCertificate ) )
        << "Fields must be encrypted";
    EXPECT_NE( json_get_str( rawDbData, Fields::ClientCertificateKey ),
               json_get_str( data, Fields::ClientCertificateKey ) )
        << "Fields must be encrypted";
}

TEST_F( DatabaseSettingsTest, smtpWriteAndReadPartialData ) {
    namespace Fields = Resources::DB::Settings::FieldsSMTP;

    Json data;
    data[ Fields::SMTPServerUrl ] = "smtp://server:port";
    data[ Fields::Encryption ] = false;

    DatabaseSettings dbSettings( getDBClient() );

    EXPECT_NO_THROW( dbSettings.setSMTPSettings( data ) );
    auto retrievedData = dbSettings.getSMTPSettings();

    EXPECT_EQ( data, retrievedData );
}

TEST_F( DatabaseSettingsTest, smtpRejectInvalidData ) {
    namespace Fields = Resources::DB::Settings::FieldsSMTP;

    DatabaseSettings dbSettings( getDBClient() );

    {  // Missing required field
        auto data = smtpSettingsTestData();
        json_remove( data, Fields::SMTPServerUrl );
        EXPECT_THROW( dbSettings.setSMTPSettings( data ), SettingsError );
    }

    {  // Missing required field
        auto data = smtpSettingsTestData();
        json_remove( data, Fields::Encryption );
        EXPECT_THROW( dbSettings.setSMTPSettings( data ), SettingsError );
    }

    {  // Invalid type
        auto data = smtpSettingsTestData();
        data[ Fields::SMTPServerUrl ] = int( 1000 );
        EXPECT_THROW( dbSettings.setSMTPSettings( data ), SettingsError );
    }

    {  // Invalid type
        auto data = smtpSettingsTestData();
        data[ Fields::Encryption ] = "string";
        EXPECT_THROW( dbSettings.setSMTPSettings( data ), SettingsError );
    }

    {  // Invalid type
        auto data = smtpSettingsTestData();
        data[ Fields::VerifyServerCertificate ] = "string";
        EXPECT_THROW( dbSettings.setSMTPSettings( data ), SettingsError );
    }

    {  // Invalid type
        auto data = smtpSettingsTestData();
        data[ Fields::VerifyServerHostname ] = "string";
        EXPECT_THROW( dbSettings.setSMTPSettings( data ), SettingsError );
    }

    {  // Invalid type
        auto data = smtpSettingsTestData();
        data[ Fields::CertificatesVersion ] = "string";
        EXPECT_THROW( dbSettings.setSMTPSettings( data ), SettingsError );
    }

    {  // Invalid type
        auto data = smtpSettingsTestData();
        data[ Fields::ServerCACertificate ] = bool( false );
        EXPECT_THROW( dbSettings.setSMTPSettings( data ), SettingsError );
    }

    {  // Invalid type
        auto data = smtpSettingsTestData();
        data[ Fields::ClientCertificate ] = bool( false );
        EXPECT_THROW( dbSettings.setSMTPSettings( data ), SettingsError );
    }

    {  // Invalid type
        auto data = smtpSettingsTestData();
        data[ Fields::ClientCertificateKey ] = bool( false );
        EXPECT_THROW( dbSettings.setSMTPSettings( data ), SettingsError );
    }

    {  // Invalid type
        auto data = smtpSettingsTestData();
        data[ Fields::ClientUsername ] = bool( false );
        EXPECT_THROW( dbSettings.setSMTPSettings( data ), SettingsError );
    }

    {  // Invalid type
        auto data = smtpSettingsTestData();
        data[ Fields::ClientPassword ] = bool( false );
        EXPECT_THROW( dbSettings.setSMTPSettings( data ), SettingsError );
    }
}

TEST_F( DatabaseSettingsTest, smtpHashedSettings ) {
    namespace Fields = Resources::DB::Settings::FieldsSMTP;
    const auto data = smtpSettingsTestData();

    DatabaseSettings dbSettings( getDBClient() );

    EXPECT_NO_THROW( dbSettings.setSMTPSettings( data ) );
    auto retrievedData = dbSettings.getHashedSMTPSettings();

    EXPECT_NE( data, retrievedData );

    auto findKey = [&retrievedData]( const std::string& key ) {
        return retrievedData.find( key ) != retrievedData.cend();
    };

    EXPECT_FALSE( findKey( Fields::ClientUsername ) ) << "Sensitive field must be removed";
    EXPECT_FALSE( findKey( Fields::ClientPassword ) ) << "Sensitive field must be removed";
    EXPECT_FALSE( findKey( Fields::ServerCACertificate ) ) << "Sensitive field must be removed";
    EXPECT_FALSE( findKey( Fields::ClientCertificate ) ) << "Sensitive field must be removed";
    EXPECT_FALSE( findKey( Fields::ClientCertificateKey ) ) << "Sensitive field must be removed";

    auto validateHashedKey = [&data, &retrievedData]( const std::string& key ) -> bool {
        auto hashedValue = json_get_str( retrievedData, key + "MD5" );

        if( hashedValue.empty() ) {
            ADD_FAILURE() << "Missing or empty value for hashed key: " + key;
            return false;
        }

        if( json_get_str( data, key ) == hashedValue ) {
            ADD_FAILURE() << "Plain and hashed value are equal. Key: " + key;
            return false;
        }

        return true;
    };

    EXPECT_TRUE( validateHashedKey( Fields::ClientUsername ) );
    EXPECT_TRUE( validateHashedKey( Fields::ClientPassword ) );
    EXPECT_TRUE( validateHashedKey( Fields::ServerCACertificate ) );
    EXPECT_TRUE( validateHashedKey( Fields::ClientCertificate ) );
    EXPECT_TRUE( validateHashedKey( Fields::ClientCertificateKey ) );
}

}  // namespace UT
}  // namespace HubPlatform
