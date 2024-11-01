#include "networkd/configuration/nm_config_file_manager.h"
#include "test_utils/test_environment.h"
#include "system_utils/filesystem.h"
#include "system_utils/system.h"
#include "json_utils/json_utils.h"
#include "log_utils/ezlo_log.h"
#include "config_reader/tests/config_reader_factory.h"
#include <gtest/gtest.h>

/*

TESTPLAN

DONE    1. Network Config Integrity
DONE        1.1 Network Config Integrity: wifi
DONE        1.2 Network Config Integrity: mobile
DONE    2. Network Config Mobile Configuration

*/

namespace HubPlatform {
namespace UT {

namespace Resources {
const char NetworkConfigPath[] = "/system/common/opt/firmware/etc/network.cfg";
}

class NmConfigFileManagerSuite : public ::testing::Test {
public:
    static std::string makeNetworkConfigPath();
    void fillNetworkConfigFile( const std::string& networkConfigPath, int codeLine );
    static std::string makeError( int codeLine );

protected:
    Json m_config;
};

std::string NmConfigFileManagerSuite::makeNetworkConfigPath() {
    return HubPlatform::Tests::getSourceFolder() + Resources::NetworkConfigPath;
}

void NmConfigFileManagerSuite::fillNetworkConfigFile( const std::string& networkConfigPath, int codeLine ) {
    ASSERT_TRUE( filesystem::isFileExist( networkConfigPath ) )
        << "Network config should exist, " << makeError( codeLine );
    auto configData = System::readFile( networkConfigPath );
    ASSERT_FALSE( configData.empty() ) << "network config shouldn't be empty, " << makeError( codeLine );

    m_config = parseJson( configData );
    ASSERT_FALSE( m_config.empty() ) << "Network config should bne a parseable json file,  << makeError( codeLine )";
}

std::string NmConfigFileManagerSuite::makeError( int codeLine ) {
    return std::string{ "at: " }.append( __FILE__ ).append( ":" ).append( std::to_string( codeLine ) );
}

TEST_F( NmConfigFileManagerSuite, networkConfigIntegrity ) /* 1 */ {
    auto networkConfigPath = makeNetworkConfigPath();
    fillNetworkConfigFile( networkConfigPath, __LINE__ );
}

TEST_F( NmConfigFileManagerSuite, checkNetworkConfigWifi ) /* 1.1 */ {
    auto networkConfigPath = makeNetworkConfigPath();
    fillNetworkConfigFile( networkConfigPath, __LINE__ );

    Json wifi;
    ASSERT_TRUE( json_get( m_config, "wifi", wifi ) ) << "network/wifi path should be present";
    {
        EXPECT_FALSE( json_get_str( wifi, "apIp" ).empty() );
        EXPECT_FALSE( json_get_str( wifi, "apNetMask" ).empty() );
        EXPECT_NE( json_get_int( wifi, "apDefaultChannel", -1 ), -1 );
        EXPECT_NE( json_get_int( wifi, "apStartLeasingAddress", -1 ), -1 );
        EXPECT_NE( json_get_int( wifi, "apLimitLeasingAddresses", -1 ), -1 );
        EXPECT_NE( json_get_int( wifi, "apManualScanInterval", -1 ), -1 );
        EXPECT_NE( json_get_int( wifi, "apManualScanDelta", -1 ), -1 );
    }
}

TEST_F( NmConfigFileManagerSuite, checkNetworkConfigMobile ) /* 1.2 */ {
    auto networkConfigPath = makeNetworkConfigPath();
    fillNetworkConfigFile( networkConfigPath, __LINE__ );

    Json mobile;
    ASSERT_TRUE( json_get( m_config, "mobile", mobile ) ) << "network/mobile path should be present";

    {
        Json apnSettings;
        ASSERT_TRUE( json_get_obj( mobile, "apnSettings", apnSettings ) ) << "apnSettings dictionary required";

        JsonArray arr;
        EXPECT_TRUE( json_get_arr( apnSettings, "Hologram", arr ) );
        EXPECT_TRUE( json_get_arr( apnSettings, "T-Mobile", arr ) );
        EXPECT_TRUE( json_get_arr( apnSettings, "AT&T", arr ) );
        EXPECT_TRUE( json_get_arr( apnSettings, "Verizon Wireless", arr ) );

        ASSERT_TRUE( json_get_arr( mobile, "defaultApnSettings", arr ) && !arr.empty() )
            << "defaultApnSettings array required";
    }
}

class ConfigReaderFactoryImpl : public ConfigReaderFactory {
public:
    using ConfigReaderFactory::createConfigReader;
};

TEST_F( NmConfigFileManagerSuite, validateMobileConfiguration ) /* 2 */ {
    auto networkConfigPath = makeNetworkConfigPath();
    fillNetworkConfigFile( networkConfigPath, __LINE__ );

    ConfigReader reader =
        ConfigReaderFactoryImpl::createConfigReader( [this]( const std::string& ) { return m_config.dump(); } );
    Network::NetworkConfigFileManager manager( reader );

    auto config = manager.getMobileConfiguration();

    EXPECT_FALSE( config.m_APNSettings.empty() ) << "APN Settings";
    EXPECT_FALSE( config.m_defaultAPNSettings.empty() ) << "Default APN Settings";
}

}  // namespace UT
}  // namespace HubPlatform