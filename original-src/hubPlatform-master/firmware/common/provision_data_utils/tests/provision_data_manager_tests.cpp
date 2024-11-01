#include "provision_data_utils/provision_data_manager.h"

#include "test_utils/temporary_folder_wrapper.h"
#include "test_utils/sample_factory_data.h"
#include "system_utils/environment.h"

#include <gtest/gtest.h>

#include <fstream>

/*

TESTPLAN

DONE    1. No provision data
DONE    2. Original provision data
DONE    3. Modified provision data
DONE    4. Init provision data
DONE    5. Update provision data
DONE    6. Re-update provision data
DONE    7. Read SSID and password from provisioning data
DONE    8. Encrypt and decrypt sensitive data
DONE    9. Decrypt broken data
DONE   10. Different required action after updating provisioning data

*/

using namespace ::testing;
using namespace HubPlatform;
using namespace HubPlatform::ProvisionDataUtils;

std::ostream& operator<<( std::ostream& stream, UpdateStatus const& status ) {
    switch( status ) {
    case UpdateStatus::RestartRequired:
        stream << "UpdateStatus::RestartRequired";
        break;

    case UpdateStatus::ReconnectRequired:
        stream << "UpdateStatus::ReconnectRequired";
        break;

    case UpdateStatus::NothingRequired:
        stream << "UpdateStatus::NothingRequired";
        break;

    case UpdateStatus::Failed:
        stream << "UpdateStatus::Failed";
        break;
    };

    return stream;
}

class FakeDirectory {
public:
    const std::string& path() {
        return m_tmpDir.getPath();
    }

    std::string readFile( const std::string& name ) {
        std::ifstream ifs( m_tmpDir.getPath() + "/" + name );
        std::string content( ( std::istreambuf_iterator< char >( ifs ) ), std::istreambuf_iterator< char >() );
        return content;
    }

    void writeFile( const std::string& name, const std::string& data ) {
        std::ofstream ofs( m_tmpDir.getPath() + "/" + name, std::ofstream::trunc );
        ofs << data;
        ofs.flush();
    }

private:
    HubPlatform::Tests::TemporaryFolderWrapper m_tmpDir;
};

template < typename Base >
class ProvisionDataManagerFixture : public Base {
public:
    std::unique_ptr< ProvisionDataManager > createManager() {
        HubPlatform::Environment::setProvisionDataFolder( m_runtimeDir.path() );
        HubPlatform::Environment::setDataFolder( m_dataDir.path() );
        return std::make_unique< ProvisionDataManager >();
    }

    FakeDirectory& runtimeDir() {
        return m_runtimeDir;
    }
    FakeDirectory& dataDir() {
        return m_dataDir;
    }

private:
    FakeDirectory m_runtimeDir;
    FakeDirectory m_dataDir;
};

struct UpdateProvisioningTestData {
    using ApplyNewProvisioningCall = std::function< void( ProvisionData& ) >;

    ApplyNewProvisioningCall applyNewProvisioning;
    ProvisionDataUtils::UpdateStatus expectedStatus = ProvisionDataUtils::UpdateStatus::Failed;

    std::string errorMessage;

    UpdateProvisioningTestData( ApplyNewProvisioningCall func,
                                ProvisionDataUtils::UpdateStatus status,
                                std::string&& error = std::string() )
    : applyNewProvisioning( std::move( func ) )
    , expectedStatus( status )
    , errorMessage( std::move( error ) ) {}
};

std::ostream& operator<<( std::ostream& stream, const UpdateProvisioningTestData& data ) {
    stream << data.errorMessage << "(" << data.expectedStatus << ")";
    return stream;
}

using ProvisionDataManagerTestSuite = ProvisionDataManagerFixture< ::testing::Test >;
using ProvisionSaveDataTestSuite =
    ProvisionDataManagerFixture< ::testing::TestWithParam< UpdateProvisioningTestData > >;

TEST_F( ProvisionDataManagerTestSuite, NoProvisionData /*1*/ ) {
    auto manager = createManager();

    EXPECT_FALSE( manager->getProvisionData().isValid() );
}

TEST_F( ProvisionDataManagerTestSuite, PreInitProvisionData /*2*/ ) {
    runtimeDir().writeFile(
        "factory.dat",
        "RrR0LqSkWCqlxLPfoXacDt5mOZhRYmAoVWojI8JZ7YYHDkPFT1TPEd2dsB5+q26+gl6QjkPF63Nz6hSc11b2BY5ygSqo3tfw+3fIy+"
        "7T8gq1fGZ3wk5SujIjVIXXX6MVrrsKmGwAgoWNpiTfZsK5ZmJz/OuIC2HBadw7T0GKz19HsTcg+OujeaT3iPcgE+vp" );

    auto manager = createManager();
    auto& data = manager->getProvisionData();

    EXPECT_TRUE( data.isValid() );
    EXPECT_EQ( data.configVersion, 1 );
    EXPECT_EQ( data.provisionServer, "https://req-disp-at0m.mios.com/" );
    EXPECT_EQ( data.cloudServer, "https://cloud.ezlo.com:7000" );
}

TEST_F( ProvisionDataManagerTestSuite, PreInitAndModifiedProvisionData /*3*/ ) {
    runtimeDir().writeFile(
        "factory.dat",
        "RrR0LqSkWCqlxLPfoXacDt5mOZhRYmAoVWojI8JZ7YYHDkPFT1TPEd2dsB5+q26+gl6QjkPF63Nz6hSc11b2BY5ygSqo3tfw+3fIy+"
        "7T8gq1fGZ3wk5SujIjVIXXX6MVrrsKmGwAgoWNpiTfZsK5ZmJz/OuIC2HBadw7T0GKz19HsTcg+OujeaT3iPcgE+vp" );
    runtimeDir().writeFile(
        "factory2.dat",
        "5eHvbEdQsVGTJ1fdxfx5+5npmzGdV1OJPABBf6ICHXFze6juwby3/RjwxywdD00g3FK1IgyF1v7O20RZPmXreikDKWg/"
        "vjt4zks4ouxXSUvI4xxmZnferuOVTMh+fLbL" );

    auto manager = createManager();
    auto& data = manager->getProvisionData();

    EXPECT_TRUE( data.isValid() );
    EXPECT_EQ( data.configVersion, 2 );
    EXPECT_EQ( data.provisionServer, "https://req-disp-at0m.mios.com/" );
    EXPECT_EQ( data.cloudServer, "https://cloud-qa.ezlo.com:7000" );
}

TEST_F( ProvisionDataManagerTestSuite, InitProvisionData /*4*/ ) {
    ProvisionData data;
    data.configVersion = 1;
    data.provisionServer = "https://req-disp-at0m.mios.com/";
    data.cloudServer = "https://cloud.ezlo.com:7000";

    {
        auto manager = createManager();
        EXPECT_TRUE( manager->init( data ) );
    }

    EXPECT_FALSE( runtimeDir().readFile( "factory.dat" ).empty() );
    EXPECT_TRUE( runtimeDir().readFile( "factory2.dat" ).empty() );
    EXPECT_TRUE( dataDir().readFile( "factory2.dat" ).empty() );

    {
        auto manager = createManager();
        data = manager->getProvisionData();
    }

    EXPECT_TRUE( data.isValid() );
    EXPECT_EQ( data.configVersion, 1 );
    EXPECT_EQ( data.provisionServer, "https://req-disp-at0m.mios.com/" );
    EXPECT_EQ( data.cloudServer, "https://cloud.ezlo.com:7000" );
}

TEST_F( ProvisionDataManagerTestSuite, UpdateProvisionData /*5*/ ) {
    std::string factoryData =
        "RrR0LqSkWCqlxLPfoXacDt5mOZhRYmAoVWojI8JZ7YYHDkPFT1TPEd2dsB5+q26+gl6QjkPF63Nz6hSc11b2BY5ygSqo3tfw+3fIy+"
        "7T8gq1fGZ3wk5SujIjVIXXX6MVrrsKmGwAgoWNpiTfZsK5ZmJz/OuIC2HBadw7T0GKz19HsTcg+OujeaT3iPcgE+vp";
    runtimeDir().writeFile( "factory.dat", factoryData );
    ProvisionData data;

    {
        auto manager = createManager();
        data = manager->getProvisionData();
    }

    {
        data.configVersion = 2;
        data.cloudServer = "https://cloud-qa.ezlo.com:7000";

        auto manager = createManager();
        EXPECT_EQ( manager->update( data ), UpdateStatus::ReconnectRequired );
    }

    EXPECT_EQ( runtimeDir().readFile( "factory.dat" ), factoryData );
    EXPECT_FALSE( runtimeDir().readFile( "factory2.dat" ).empty() );
    EXPECT_FALSE( dataDir().readFile( "factory2.dat" ).empty() );

    {
        auto manager = createManager();
        data = manager->getProvisionData();
    }

    EXPECT_TRUE( data.isValid() );
    EXPECT_EQ( data.configVersion, 2 );
    EXPECT_EQ( data.provisionServer, "https://req-disp-at0m.mios.com/" );
    EXPECT_EQ( data.cloudServer, "https://cloud-qa.ezlo.com:7000" );
}

TEST_F( ProvisionDataManagerTestSuite, ReupdateProvisionData /*6*/ ) {
    std::string factoryData =
        "RrR0LqSkWCqlxLPfoXacDt5mOZhRYmAoVWojI8JZ7YYHDkPFT1TPEd2dsB5+q26+gl6QjkPF63Nz6hSc11b2BY5ygSqo3tfw+3fIy+"
        "7T8gq1fGZ3wk5SujIjVIXXX6MVrrsKmGwAgoWNpiTfZsK5ZmJz/OuIC2HBadw7T0GKz19HsTcg+OujeaT3iPcgE+vp";
    runtimeDir().writeFile( "factory.dat", factoryData );
    ProvisionData data;

    {
        auto manager = createManager();
        data = manager->getProvisionData();
    }

    {
        data.configVersion = 2;
        data.cloudServer = "https://cloud-qa.ezlo.com:7000";

        auto manager = createManager();
        EXPECT_EQ( manager->update( data ), UpdateStatus::ReconnectRequired );
    }

    {
        data.configVersion = 3;
        data.provisionServer = "https://req-disp-g150.mios.com/";

        auto manager = createManager();
        EXPECT_EQ( manager->update( data ), UpdateStatus::ReconnectRequired );
    }

    EXPECT_EQ( runtimeDir().readFile( "factory.dat" ), factoryData );
    EXPECT_FALSE( runtimeDir().readFile( "factory2.dat" ).empty() );
    EXPECT_FALSE( dataDir().readFile( "factory2.dat" ).empty() );

    {
        auto manager = createManager();
        data = manager->getProvisionData();
    }

    EXPECT_TRUE( data.isValid() );
    EXPECT_EQ( data.configVersion, 3 );
    EXPECT_EQ( data.provisionServer, "https://req-disp-g150.mios.com/" );
    EXPECT_EQ( data.cloudServer, "https://cloud-qa.ezlo.com:7000" );
}

TEST_F( ProvisionDataManagerTestSuite, ReadSSidAndPasswordFromProvisioningData /*7*/ ) {
    const std::string factoryData = Tests::SampleFactoryData;
    runtimeDir().writeFile( "factory.dat", factoryData );
    EXPECT_EQ( runtimeDir().readFile( "factory.dat" ), factoryData );

    const auto manager = createManager();
    const auto& data = manager->getProvisionData();

    EXPECT_EQ( data.defaultWifiSsid, "ezlo_smth" );
    EXPECT_EQ( data.defaultWifiPassword, "1651b3dc65cf3a4691ee8051" );
}

TEST_F( ProvisionDataManagerTestSuite, EncryptAndDecryptSensitiveData /*8*/ ) {
    runtimeDir().writeFile( "factory.dat", Tests::SampleFactoryData );

    const auto manager = createManager();
    const std::string original = "13666";
    const auto encrypted = manager->getProvisionData().encryptSensitiveData( original );
    const auto decrypted = manager->getProvisionData().decryptSensitiveData( encrypted );

    EXPECT_NE( encrypted, original );
    EXPECT_EQ( encrypted.size(), 44 );
    EXPECT_EQ( original, decrypted );
}

TEST_F( ProvisionDataManagerTestSuite, DecryptBrokenData /*9*/ ) {
    runtimeDir().writeFile( "factory.dat", Tests::SampleFactoryData );

    const auto manager = createManager();

    EXPECT_EQ( manager->getProvisionData().decryptSensitiveData( "" ), "" );
    EXPECT_EQ( manager->getProvisionData().decryptSensitiveData( "asd" ), "" );
    EXPECT_EQ( manager->getProvisionData().decryptSensitiveData( "12345678901234567890123456789012345678901234" ), "" );
}

TEST_P( ProvisionSaveDataTestSuite, RequiredActionAfterUpdatingProvisioning /*10*/ ) {
    std::string factoryData =
        "RrR0LqSkWCqlxLPfoXacDt5mOZhRYmAoVWojI8JZ7YYHDkPFT1TPEd2dsB5+q26+gl6QjkPF63Nz6hSc11b2BY5ygSqo3tfw+3fIy+"
        "7T8gq1fGZ3wk5SujIjVIXXX6MVrrsKmGwAgoWNpiTfZsK5ZmJz/OuIC2HBadw7T0GKz19HsTcg+OujeaT3iPcgE+vp";
    runtimeDir().writeFile( "factory.dat", factoryData );
    ProvisionData data;

    {
        auto manager = createManager();
        data = manager->getProvisionData();
    }

    {
        GetParam().applyNewProvisioning( data );
        data.configVersion = 2;

        auto manager = createManager();
        EXPECT_EQ( manager->update( data ), GetParam().expectedStatus ) << GetParam().errorMessage;
    }

    EXPECT_EQ( runtimeDir().readFile( "factory.dat" ), factoryData );
    EXPECT_FALSE( runtimeDir().readFile( "factory2.dat" ).empty() );

    {
        auto manager = createManager();
        data = manager->getProvisionData();
    }

    EXPECT_TRUE( data.isValid() );
    EXPECT_EQ( data.configVersion, 2 );
}

INSTANTIATE_TEST_CASE_P(
    LeapYearTests,
    ProvisionSaveDataTestSuite,
    ::testing::Values(
        UpdateProvisioningTestData{ []( ProvisionData& data ) { data.provisionServer = "https://my.cloud.com/"; },
                                    UpdateStatus::ReconnectRequired,
                                    "Reconnect should be enough if provisioning server was changed!" },

        UpdateProvisioningTestData{ []( ProvisionData& data ) { data.pkDevice = "00000007"; },
                                    UpdateStatus::RestartRequired,
                                    "Restart should be required if serial was changed!" },

        UpdateProvisioningTestData{ []( ProvisionData& data ) {
                                       data.provisionServer = "https://my.cloud.com/";
                                       data.pkDevice = "00000007";
                                   },
                                    UpdateStatus::RestartRequired,
                                    "If a few fields was changed the highest requirement should be returned!" },

        UpdateProvisioningTestData{ []( ProvisionData& data ) { data.uuid = "f1187280-0f5e-4eb8-b69f-960f42f50124"; },
                                    UpdateStatus::RestartRequired,
                                    "Restart should be required if UUID of controller was changed!" },

        UpdateProvisioningTestData{
            []( ProvisionData& data ) { data.cloudUuid = "f1187280-0f5e-4eb8-b69f-960f42f50124"; },
            UpdateStatus::NothingRequired,
            "Nothing should be required if UUID of Cloud was changed!(Not used data)" },

        UpdateProvisioningTestData{
            []( ProvisionData& data ) { data.orderUuid = "f1187280-0f5e-4eb8-b69f-960f42f50124"; },
            UpdateStatus::NothingRequired,
            "Nothing should be required if UUID of order was changed!(Not used data)" },

        UpdateProvisioningTestData{
            []( ProvisionData& data ) { data.cloudServer = "https://new.coordinator.com:7000/"; },
            UpdateStatus::ReconnectRequired,
            "Reconnect should be enough if coordinator server was changed!" },

        UpdateProvisioningTestData{ []( ProvisionData& data ) { data.provisionToken = "NewToken"; },
                                    UpdateStatus::NothingRequired,
                                    "Nothing should be required if provisioning token was changed!" },

        UpdateProvisioningTestData{ []( ProvisionData& data ) { data.sslPrivateKey = "key"; },
                                    UpdateStatus::ReconnectRequired,
                                    "Reconnect should be enough if SSL private key was changed!" },

        UpdateProvisioningTestData{ []( ProvisionData& data ) { data.sslPublicKey = "key"; },
                                    UpdateStatus::ReconnectRequired,
                                    "Reconnect should be enough if SSL public key was changed!" },

        UpdateProvisioningTestData{ []( ProvisionData& data ) { data.sslSharedKey = "key"; },
                                    UpdateStatus::ReconnectRequired,
                                    "Reconnect should be enough if shared SSL key was changed!" },

        UpdateProvisioningTestData{ []( ProvisionData& data ) { data.signingCaCertificate = "key"; },
                                    UpdateStatus::ReconnectRequired,
                                    "Reconnect should be enough if Cloud certificate was changed!" },

        UpdateProvisioningTestData{ []( ProvisionData& data ) { data.zwaveRegion = "UKR"; },
                                    UpdateStatus::RestartRequired,
                                    "Restart should be required if Z-Wave region was changed!" },

        UpdateProvisioningTestData{ []( ProvisionData& data ) { data.defaultSshPassword = "NewPassword"; },
                                    UpdateStatus::RestartRequired,
                                    "Restart should be required if SSH password was changed!" },

        UpdateProvisioningTestData{ []( ProvisionData& data ) { data.defaultWifiSsid = "WiFiPassword"; },
                                    UpdateStatus::RestartRequired,
                                    "Restart should be required if WiFi password was changed!" },

        UpdateProvisioningTestData{ []( ProvisionData& data ) {
                                       data.timeServers = { { "ntp.server1.ua" }, { "ntp.server2.ua" } };
                                   },
                                    UpdateStatus::RestartRequired,
                                    "Restart should be required if NTP server was changed!" }

        ) );
