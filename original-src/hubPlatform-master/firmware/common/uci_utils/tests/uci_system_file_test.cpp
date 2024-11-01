#include "uci_utils/uci_system_file.h"

#include "system_utils/filesystem.h"
#include "test_utils/temporary_folder_wrapper.h"
#include "log_utils/ezlo_log.h"
#include "json_utils/json_utils.h"
#include "common_utils/cpp14.h"

#include <gtest/gtest.h>

#include <fstream>

/*

TESTPLAN

DONE    1. Load default config
DONE    2. Get default timezone
DONE    3. Set custom timezone
DONE    4. Get custom timezone

*/

using namespace ::testing;

namespace HubPlatform {
namespace UciUtils {
namespace UT {

class UciSystemFileTestSuite : public Test {
public:
    UciSystemFileTestSuite() {
        // Do not print errors and warnings on erroneous test cases
        // TODO: implement fake log and check error messages too.
        Log::setLocalLogLevel( Log::FATAL );

        filesystem::createFolder( m_tmpDir.getPath() + "/etc/config" );
        setSystemConfigContent( "\n"
                                "config system\n"
                                "\toption hostname 'OpenWrt'\n"
                                "\toption timezone 'UTC'\n"
                                "\toption ttylogin '0'\n"
                                "\toption log_size '64'\n"
                                "\toption urandom_seed '0'\n"
                                "\n"
                                "config timeserver 'ntp'\n"
                                "\toption enabled '1'\n"
                                "\toption enable_server '0'\n"
                                "\tlist server '0.openwrt.pool.ntp.org'\n"
                                "\tlist server '1.openwrt.pool.ntp.org'\n"
                                "\tlist server '2.openwrt.pool.ntp.org'\n"
                                "\tlist server '3.openwrt.pool.ntp.org'\n" );
    }

    std::unique_ptr< UciSystemFile > createUciSystemFile() {
        return std::make_unique< UciSystemFile >( getSystemConfigPath() );
    }

    std::string getSystemConfigContent() {
        std::ifstream systemConfig( getSystemConfigPath() );
        std::string content( ( std::istreambuf_iterator< char >( systemConfig ) ), std::istreambuf_iterator< char >() );
        return content;
    }

    void setSystemConfigContent( const std::string& content ) {
        std::ofstream systemConfig( getSystemConfigPath() );
        systemConfig << content;
    }

private:
    std::string getSystemConfigPath() {
        return m_tmpDir.getPath() + "/etc/config/system";
    }

    HubPlatform::Tests::TemporaryFolderWrapper m_tmpDir;
};

TEST_F( UciSystemFileTestSuite, testLoadDefaultConfig /*1*/ ) {
    auto systemConfig = createUciSystemFile();
    EXPECT_TRUE( systemConfig->load() );
}

TEST_F( UciSystemFileTestSuite, testGetDefaultTimezone /*2*/ ) {
    auto systemConfig = createUciSystemFile();
    systemConfig->load();

    EXPECT_TRUE( systemConfig->isTimezoneDefault() );
    EXPECT_EQ( systemConfig->getTimezone(), "UTC" );
}

TEST_F( UciSystemFileTestSuite, testSetCustomTimezone /*3*/ ) {
    auto systemConfig = createUciSystemFile();
    systemConfig->load();

    EXPECT_TRUE( systemConfig->setTimeZone( "Europe/Kiev" ) );

    EXPECT_FALSE( systemConfig->isTimezoneDefault() );
    EXPECT_EQ( systemConfig->getTimezone(), "Europe/Kiev" );

    EXPECT_TRUE( systemConfig->save() );

    EXPECT_EQ( getSystemConfigContent(),
               "config system\n"
               "\toption hostname OpenWrt\n"
               "\toption log_size 64\n"
               "\toption ttylogin 0\n"
               "\toption urandom_seed 0\n"
               "\toption zonename Europe/Kiev\n"
               "\n"
               "config timeserver ntp\n"
               "\toption enable_server 0\n"
               "\toption enabled 1\n"
               "\tlist server 0.openwrt.pool.ntp.org\n"
               "\tlist server 1.openwrt.pool.ntp.org\n"
               "\tlist server 2.openwrt.pool.ntp.org\n"
               "\tlist server 3.openwrt.pool.ntp.org\n"
               "\n" );
}

TEST_F( UciSystemFileTestSuite, testGetCustomTimezone /*4*/ ) {
    setSystemConfigContent( "config system\n"
                            "\toption hostname OpenWrt\n"
                            "\toption log_size 64\n"
                            "\toption ttylogin 0\n"
                            "\toption urandom_seed 0\n"
                            "\toption zonename Europe/Kiev\n"
                            "\n"
                            "config timeserver ntp\n"
                            "\toption enable_server 0\n"
                            "\toption enabled 1\n"
                            "\tlist server 0.openwrt.pool.ntp.org\n"
                            "\tlist server 1.openwrt.pool.ntp.org\n"
                            "\tlist server 2.openwrt.pool.ntp.org\n"
                            "\tlist server 3.openwrt.pool.ntp.org\n"
                            "\n" );

    auto systemConfig = createUciSystemFile();
    systemConfig->load();

    EXPECT_FALSE( systemConfig->isTimezoneDefault() );
    EXPECT_EQ( systemConfig->getTimezone(), "Europe/Kiev" );
}

}  // namespace UT
}  // namespace UciUtils
}  // namespace HubPlatform