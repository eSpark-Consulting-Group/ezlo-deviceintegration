#include "updatecmd/update_executor.h"

#include "updatecmd/tests/resources/update_test_resources.h"

#include "update_manager/resources/um_resources.h"

#include "system_utils/filesystem.h"
#include "test_utils/temporary_folder_wrapper.h"
#include "common_utils/cpp14.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <fstream>

/*

TESTPLAN

DONE    1 Upgrade packages
DONE        1.1 No journal
DONE        1.2 Empty journal
DONE        1.3 Upgrade single package
DONE        1.4 Upgrade next package

*/

using namespace ::testing;
using namespace HubPlatform;
using namespace HubPlatform::UpdateCommand;

class MockCommandRunner : public Opkg::CommandRunner {
public:
    MOCK_CONST_METHOD0( listInstalled, StdOutWithReturnCode() );
    MOCK_CONST_METHOD0( listUpgradable, StdOutWithReturnCode() );
    MOCK_CONST_METHOD0( listAvailable, StdOutWithReturnCode() );
    MOCK_CONST_METHOD1( listDependant, StdOutWithReturnCode( const std::string& ) );

    MOCK_METHOD2( downloadImage, StdOutWithReturnCode( const std::string&, const std::string& ) );
    MOCK_METHOD3( downloadPackage, StdOutWithReturnCode( const std::string&, const std::string&, bool ) );

    MOCK_METHOD0( updateAvailable, StdOutWithReturnCode() );
    MOCK_METHOD0( updateDownloadOnlyAvailable, StdOutWithReturnCode() );
    MOCK_METHOD0( updateNoActionAvailable, StdOutWithReturnCode() );

    MOCK_METHOD2( sysupgrade, StdOutWithReturnCode( const std::string&, const std::string& ) );
    MOCK_METHOD2( upgradePackage, StdOutWithReturnCode( const std::string&, const std::string& ) );
    MOCK_METHOD2( installPackage, StdOutWithReturnCode( const std::string&, const std::string& ) );
    MOCK_METHOD1( removePackage, StdOutWithReturnCode( const std::string& ) );
};

class MockFeedsUpdater : public Opkg::IFeedsUpdater {
public:
    MOCK_METHOD2( update, bool( const std::string&, const int ) );
    MOCK_METHOD1( update, void( const std::string& ) );

    MOCK_METHOD0( resetCurrent, void() );
    MOCK_METHOD0( resetToCurrent, void() );

    MOCK_CONST_METHOD0( getFeedsSource, std::string() );

    MOCK_CONST_METHOD0( getFeedsUpdateVersion, std::string() );
    MOCK_CONST_METHOD0( getExpectedUpdateVersion, std::string() );

    MOCK_CONST_METHOD0( getFeedsSnapshot, int() );
    MOCK_CONST_METHOD0( generateImageUrl, std::string() );
    MOCK_CONST_METHOD0( generateRecoveryImageUrl, std::string() );
};

class UpdateExecutorTestSuite : public Test {
public:
    UpdateExecutorTestSuite() {
        // Do not print errors and warnings on erroneous test cases
        // TODO: implement fake log and check error messages too.
        filesystem::createFolder( m_tmpDir.getPath() + "/home" );
        filesystem::createFolder( m_tmpDir.getPath() + "/var/log/firmware" );
        filesystem::copyFile(
            std::string( Resources::TestFilesPath ) + "/home/" + UpdateManager::Resources::UpdateJournalFileName,
            getJournalFilePath() );
    }

    void removeJournal() {
        remove( getJournalFilePath().c_str() );
    }

    void clearJournal() {
        std::ofstream journal( getJournalFilePath(), std::ofstream::trunc );
    }

    bool journalIsEmpty() {
        std::ifstream journal( getJournalFilePath(), std::ifstream::ate );
        return journal.tellg() == 0;
    }

    std::unique_ptr< UpdateExecutor > createUpdateExecutor() {
        auto packagesManager = std::make_unique< Opkg::PackagesManager >( m_commandRunner );
        auto feedsUpdater = std::make_unique< MockFeedsUpdater >();

        return std::make_unique< UpdateExecutor >( std::move( packagesManager ),
                                                   std::move( feedsUpdater ),
                                                   getJournalFilePath(),
                                                   m_tmpDir.getPath() + "/var/log/firmware/update.log",
                                                   true /*isDaemon*/
        );
    }

protected:
    MockCommandRunner m_commandRunner;

private:
    std::string getJournalFilePath() const {
        return m_tmpDir.getPath() + "/home/" + UpdateManager::Resources::UpdateJournalFileName;
    }

    HubPlatform::Tests::TemporaryFolderWrapper m_tmpDir;
};

TEST_F( UpdateExecutorTestSuite, testNoJournal /*1.1*/ ) {
    removeJournal();

    EXPECT_CALL( m_commandRunner, upgradePackage( "hub-platform-firmware", "" ) ).Times( 0 );

    createUpdateExecutor()->run();
}

TEST_F( UpdateExecutorTestSuite, testEmptyJournal /*1.2*/ ) {
    clearJournal();

    EXPECT_CALL( m_commandRunner, upgradePackage( "hub-platform-firmware", "" ) ).Times( 0 );

    createUpdateExecutor()->run();
}

TEST_F( UpdateExecutorTestSuite, testUpgradeSinglePackage /*1.3*/ ) {
    EXPECT_CALL( m_commandRunner, upgradePackage( "hub-platform-firmware", "" ) )
        .Times( 1 )
        .WillRepeatedly( Return( std::make_pair( "", 0 ) ) );

    createUpdateExecutor()->run( true );
}

TEST_F( UpdateExecutorTestSuite, testUpgradeNextPackage /*1.4*/ ) {
    InSequence callSequence;
    EXPECT_CALL( m_commandRunner, upgradePackage( "hub-platform-firmware", "" ) )
        .Times( 1 )
        .WillRepeatedly( Return( std::make_pair( "", 0 ) ) );
    EXPECT_CALL( m_commandRunner, upgradePackage( "firmware-addons-zwave", "" ) )
        .Times( 1 )
        .WillRepeatedly( Return( std::make_pair( "", 0 ) ) );
    EXPECT_CALL( m_commandRunner, upgradePackage( "firmware-addons-luad", "" ) )
        .Times( 1 )
        .WillRepeatedly( Return( std::make_pair( "", 0 ) ) );

    createUpdateExecutor()->run( true );
    createUpdateExecutor()->run( true );
    createUpdateExecutor()->run( true );
}
