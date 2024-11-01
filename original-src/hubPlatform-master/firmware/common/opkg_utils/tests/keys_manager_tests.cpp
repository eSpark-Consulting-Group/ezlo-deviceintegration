#include "opkg_utils/keys/keys_manager.h"

#include "opkg_utils/tests/resources/opkg_test_resources.h"

#include "system_utils/filesystem.h"
#include "test_utils/temporary_folder_wrapper.h"

#include "bus/messagesclasses.h"
#include "json_utils/json_utils.h"
#include "log_utils/ezlo_log.h"
#include "common_utils/cpp14.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

/*

TESTPLAN

DONE    1 Read fingerprints
DONE        1.1 Get default fingerprints
DONE    2 Read key files
DONE        2.1 Get key file by existing fingerprint
DONE        2.2 Get key file by incorrect fingerprint
DONE    3 Manage key files
DONE        3.1 Clear key files
DONE        3.2 Remove existing key files
DONE        3.3 Remove key files by incorrect fingerprint
DONE        3.4 Add key files

*/

using namespace ::testing;
using namespace HubPlatform;
using namespace HubPlatform::Opkg;

class KeysManagerTestSuite : public Test {
public:
    KeysManagerTestSuite() {
        // Do not print errors and warnings on erroneous test cases
        // TODO: implement fake log and check error messages too.
        Log::setLocalLogLevel( Log::FATAL );

        filesystem::createFolder( m_tmpDir.getPath() + "/etc/opkg/keys" );
        filesystem::copyFile( std::string( Resources::TestFilesPath ) + "/etc/opkg/keys/1f73e8b83819ae5",
                              m_tmpDir.getPath() + "/etc/opkg/keys/1f73e8b83819ae5" );
        filesystem::copyFile( std::string( Resources::TestFilesPath ) + "/etc/opkg/keys/1035ac73cc4e59e3",
                              m_tmpDir.getPath() + "/etc/opkg/keys/1035ac73cc4e59e3" );
    }

    std::unique_ptr< KeysManager > createKeysManager() {
        return std::make_unique< KeysManager >( m_tmpDir.getPath() + "/etc/opkg/keys" );
    }

private:
    HubPlatform::Tests::TemporaryFolderWrapper m_tmpDir;
};

TEST_F( KeysManagerTestSuite, testGetDefaultFingerprints /*1.1*/ ) {
    auto keysManager = createKeysManager();
    auto fingerprints = keysManager->getFingerprints();
    std::sort( fingerprints.begin(), fingerprints.end() );
    ASSERT_THAT( fingerprints, ElementsAre( "1035ac73cc4e59e3", "1f73e8b83819ae5" ) );
}

TEST_F( KeysManagerTestSuite, testGetKeyFileByExistingFingerprint /*2.1*/ ) {
    auto keysManager = createKeysManager();

    auto keyFile = keysManager->getKeyFile( "1f73e8b83819ae5" );

    EXPECT_TRUE( keyFile.isValid() );
    EXPECT_EQ( keyFile.fingerprint, "1f73e8b83819ae5" );
    EXPECT_EQ( keyFile.comment, "eZLO Innovation OpenWrt signature" );
    EXPECT_EQ( keyFile.key, "RWQB9z6Lg4Ga5QzcBzVINgMgvKwj4INe2JWc3iLIFFapvkBqBwOPCpnQ" );
}

TEST_F( KeysManagerTestSuite, testGetKeyFileByIncorrectFingerprint /*2.2*/ ) {
    auto keysManager = createKeysManager();

    auto keyFile = keysManager->getKeyFile( "incorrect_fingerprint" );

    EXPECT_FALSE( keyFile.isValid() );
}

TEST_F( KeysManagerTestSuite, testClearKeyFiles /*3.1*/ ) {
    auto keysManager = createKeysManager();

    auto keyFile = keysManager->clearKeyFiles();

    auto fingerprints = keysManager->getFingerprints();
    EXPECT_TRUE( fingerprints.empty() );
}

TEST_F( KeysManagerTestSuite, testRemoveExistingKeyFiles /*3.2*/ ) {
    auto keysManager = createKeysManager();

    EXPECT_TRUE( keysManager->removeKeyFile( "1f73e8b83819ae5" ) );

    ASSERT_THAT( keysManager->getFingerprints(), ElementsAre( "1035ac73cc4e59e3" ) );
}

TEST_F( KeysManagerTestSuite, testRemoveKeyFilesByIncorrectFingerprint /*3.3*/ ) {
    auto keysManager = createKeysManager();

    EXPECT_FALSE( keysManager->removeKeyFile( "incorrect_fingerprint" ) );

    auto fingerprints = keysManager->getFingerprints();
    std::sort( fingerprints.begin(), fingerprints.end() );
    ASSERT_THAT( fingerprints, ElementsAre( "1035ac73cc4e59e3", "1f73e8b83819ae5" ) );
}

TEST_F( KeysManagerTestSuite, testAddKeyFiles /*3.4*/ ) {
    auto keysManager = createKeysManager();

    keysManager->clearKeyFiles();

    EXPECT_TRUE( keysManager->addKeyFile( { "fingerprint", "comment", "key" } ) );

    ASSERT_THAT( keysManager->getFingerprints(), ElementsAre( "fingerprint" ) );

    auto keyFile = keysManager->getKeyFile( "fingerprint" );

    EXPECT_TRUE( keyFile.isValid() );
    EXPECT_EQ( keyFile.fingerprint, "fingerprint" );
    EXPECT_EQ( keyFile.comment, "comment" );
    EXPECT_EQ( keyFile.key, "key" );
}
