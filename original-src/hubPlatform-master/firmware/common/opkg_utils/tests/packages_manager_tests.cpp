#include "opkg_utils/packages/packages_manager.h"

#include "bus/messagesclasses.h"
#include "json_utils/json_utils.h"
#include "log_utils/ezlo_log.h"
#include "common_utils/cpp14.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

/*

TESTPLAN

DONE    1 Get package lists
DONE        1.1 Get installed packages
DONE        1.2 Get upgradable packages
DONE        1.3 Get available packages
DONE        1.4 Get dependant packages

*/

using namespace ::testing;
using namespace HubPlatform::Opkg;

class FakeCommandRunner : public CommandRunner {
public:
    StdOutWithReturnCode listInstalled() const override {
        return std::make_pair( "firmware-addons-zwave - 1.0.41\n"
                               "firmware-common-system_utils - 1.0.42\n"
                               "hub-platform-firmware - 1.0.43\n"
                               "opkg - 2019-01-18-7708a01a-1\n",
                               0 );
    }

    StdOutWithReturnCode listUpgradable() const override {
        return std::make_pair( "firmware-addons-zwave - 1.0.41 - 1.0.51\n"
                               "firmware-common-system_utils - 1.0.42 - 1.0.52\n"
                               "hub-platform-firmware - 1.0.43 - 1.0.53\n",
                               0 );
    }

    StdOutWithReturnCode listAvailable() const override {
        return std::make_pair( "busybox - 1.30.0-3 - The Swiss Army Knife of embedded Linux.\n"
                               " It slices, it dices, it makes Julian Fries.\n"
                               "firmware-addons-cameras - 1.0.42\n"
                               "firmware-addons-cameras - 1.0.69 - Just firmware-addons-cameras package\n"
                               "opkg - 2019-01-18-7708a01a-1 - Lightweight package management system\n"
                               " opkg is the opkg Package Management System, for handling\n"
                               " installation and removal of packages on a system. It can\n"
                               " recursively follow dependencies and download all packages\n"
                               " necessary to install a particular package.\n"
                               " \n"
                               " opkg knows how to install both .ipk and .deb packages.\n",
                               0 );
    }

    StdOutWithReturnCode listDependant( const std::string& packageName ) const override {
        return std::make_pair( "Root set:\n"
                               "  hub-platform-firmware\n"
                               "What depends on root set\n"
                               "\tfirmware-addons-cameras 1.0.41\tdepends on hub-platform-firmware (= 1.0.42)\n"
                               "\thub-platform-h2 1.0.42\tdepends on firmware-addons-cameras (= 1.0.42)\n"
                               "\tfirmware-addons-luad 1.0.43\tdepends on hub-platform-firmware (= 1.0.42)\n"
                               "\tfirmware-addons-zwave 1.0.44\tdepends on hub-platform-firmware (= 1.0.42)\n"
                               "\tfirmware-addons-luup 1.0.45\tdepends on hub-platform-firmware (= 1.0.42)\n",
                               0 );
    }

    StdOutWithReturnCode downloadImage( const std::string& imageUrl, const std::string& cacheDirectory ) override {
        return {};
    }

    StdOutWithReturnCode downloadPackage( const std::string& packageName,
                                          const std::string& cacheDirectory,
                                          bool withDependencies ) override {
        return {};
    }

    StdOutWithReturnCode updateAvailable() override {
        return {};
    }
    StdOutWithReturnCode updateDownloadOnlyAvailable() {
        return {};
    }
    StdOutWithReturnCode updateNoActionAvailable() {
        return {};
    }

    StdOutWithReturnCode sysupgrade( const std::string& imageUrl, const std::string& cacheDirectory = "" ) override {
        return {};
    }
    StdOutWithReturnCode upgradePackage( const std::string& packageName,
                                         const std::string& cacheDirectory = "" ) override {
        return {};
    }
    StdOutWithReturnCode installPackage( const std::string& packageName,
                                         const std::string& cacheDirectory = "" ) override {
        return {};
    }
    StdOutWithReturnCode removePackage( const std::string& packageName ) override {
        return {};
    }
};

class PackagesManagerTestSuite : public Test {
public:
    PackagesManagerTestSuite() {
        // Do not print errors and warnings on erroneous test cases
        // TODO: implement fake log and check error messages too.
        Log::setLocalLogLevel( Log::FATAL );
    }

    std::unique_ptr< IPackagesManager > createPackagesManager() {
        return std::make_unique< PackagesManager >( m_commandRunner );
    }

private:
    FakeCommandRunner m_commandRunner;
};

TEST_F( PackagesManagerTestSuite, testGetInstalledPackages /*1.1*/ ) {
    auto packageManager = createPackagesManager();

    auto packages = packageManager->getInstalledPackages();

    EXPECT_EQ( packages.size(), 4 );
    EXPECT_EQ( packages[ 0 ].name, "firmware-addons-zwave" );
    EXPECT_EQ( packages[ 0 ].version, "1.0.41" );
    EXPECT_EQ( packages[ 1 ].name, "firmware-common-system_utils" );
    EXPECT_EQ( packages[ 1 ].version, "1.0.42" );
    EXPECT_EQ( packages[ 2 ].name, "hub-platform-firmware" );
    EXPECT_EQ( packages[ 2 ].version, "1.0.43" );
    EXPECT_EQ( packages[ 3 ].name, "opkg" );
    EXPECT_EQ( packages[ 3 ].version, "2019-01-18-7708a01a-1" );
}

TEST_F( PackagesManagerTestSuite, testGetUpgradablePackages /*1.2*/ ) {
    auto packageManager = createPackagesManager();

    auto packages = packageManager->getUpgradablePackages();

    EXPECT_EQ( packages.size(), 3 );
    EXPECT_EQ( packages[ 0 ].summary.name, "firmware-addons-zwave" );
    EXPECT_EQ( packages[ 0 ].summary.version, "1.0.41" );
    EXPECT_EQ( packages[ 0 ].availableVersion, "1.0.51" );
    EXPECT_EQ( packages[ 1 ].summary.name, "firmware-common-system_utils" );
    EXPECT_EQ( packages[ 1 ].summary.version, "1.0.42" );
    EXPECT_EQ( packages[ 1 ].availableVersion, "1.0.52" );
    EXPECT_EQ( packages[ 2 ].summary.name, "hub-platform-firmware" );
    EXPECT_EQ( packages[ 2 ].summary.version, "1.0.43" );
    EXPECT_EQ( packages[ 2 ].availableVersion, "1.0.53" );
}

TEST_F( PackagesManagerTestSuite, testGetAvailablePackages /*1.3*/ ) {
    auto packageManager = createPackagesManager();

    auto packages = packageManager->getAvailablePackages();

    EXPECT_EQ( packages.size(), 3 );
    EXPECT_EQ( packages[ 0 ].name, "busybox" );
    EXPECT_EQ( packages[ 0 ].version, "1.30.0-3" );
    EXPECT_EQ( packages[ 1 ].name, "firmware-addons-cameras" );
    EXPECT_EQ( packages[ 1 ].version, "1.0.69" );
    EXPECT_EQ( packages[ 2 ].name, "opkg" );
    EXPECT_EQ( packages[ 2 ].version, "2019-01-18-7708a01a-1" );
}

TEST_F( PackagesManagerTestSuite, testGetDependantPackages /*1.4*/ ) {
    auto packageManager = createPackagesManager();

    auto packages = packageManager->getDependantPackages( "hub-platform-firmware" );

    EXPECT_EQ( packages.size(), 5 );
    EXPECT_EQ( packages[ 0 ].name, "firmware-addons-cameras" );
    EXPECT_EQ( packages[ 0 ].version, "1.0.41" );
    EXPECT_EQ( packages[ 1 ].name, "hub-platform-h2" );
    EXPECT_EQ( packages[ 1 ].version, "1.0.42" );
    EXPECT_EQ( packages[ 2 ].name, "firmware-addons-luad" );
    EXPECT_EQ( packages[ 2 ].version, "1.0.43" );
    EXPECT_EQ( packages[ 3 ].name, "firmware-addons-zwave" );
    EXPECT_EQ( packages[ 3 ].version, "1.0.44" );
    EXPECT_EQ( packages[ 4 ].name, "firmware-addons-luup" );
    EXPECT_EQ( packages[ 4 ].version, "1.0.45" );
}