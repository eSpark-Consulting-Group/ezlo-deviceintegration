
#include "test_utils/temporary_folder_wrapper.h"

#include "system_utils/file_lock.h"
#include "system_utils/filesystem.h"

#include "common_utils/cpp14.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <fstream>
#include <atomic>
#include <thread>
#include <chrono>
#include <mutex>
#include <vector>

/*

TESTPLAN

DONE    1. File lock tests
DONE        1.0 File Lock API
DONE        1.1 Concurrent access

*/

namespace HubPlatform {
namespace UT {

class FileLockTest : public ::testing::Test {
public:
    const std::string& getWorkPath() const {
        return m_temporaryFolder.getPath();
    }

private:
    Tests::TemporaryFolderWrapper m_temporaryFolder;
};

TEST_F( FileLockTest, fileLockAPI /*1.0*/ ) {
    const std::string lockPath = getWorkPath() + "/test_lock";
    auto fileLock = std::make_unique< filesystem::FileLock >( lockPath );

    EXPECT_NO_THROW( fileLock->lock() );
    EXPECT_TRUE( filesystem::isFileExist( lockPath ) );

    EXPECT_FALSE( fileLock->tryLock() );

    EXPECT_NO_THROW( fileLock->unlock() );

    EXPECT_NO_THROW( fileLock->lock() ) << "Lock after unlock";
    EXPECT_NO_THROW( fileLock->unlock() );

    EXPECT_TRUE( fileLock->tryLock() );
    EXPECT_THROW( fileLock->lock(), filesystem::FileLockError ) << "Already locked";

    EXPECT_NO_THROW( fileLock.reset() ) << "Auto unlock";

    fileLock = std::make_unique< filesystem::FileLock >( lockPath );
    EXPECT_TRUE( fileLock->tryLock() ) << "Aquire new lock";
    EXPECT_NO_THROW( fileLock.reset() );
}

TEST_F( FileLockTest, concurrentAccess /*1.1*/ ) {
    const std::string lockPath = getWorkPath() + "/test_lock";
    filesystem::FileLock fileLock1( lockPath );
    filesystem::FileLock fileLock2( lockPath );
    std::mutex mutex;
    std::atomic_bool isLocked{ false };

    std::vector< int > result;
    result.reserve( 2 );

    auto thread = std::thread( [&]() {
        fileLock1.lock();
        isLocked = true;

        // Second file lock should be called during this time
        std::this_thread::sleep_for( std::chrono::milliseconds{ 250 } );

        std::unique_lock< std::mutex > lock( mutex );
        result.emplace_back( 1 );
        lock.unlock();

        fileLock1.unlock();
    } );

    while( !isLocked ) {
        std::this_thread::sleep_for( std::chrono::milliseconds{ 20 } );
    }

    fileLock2.lock();

    std::unique_lock< std::mutex > lock( mutex );
    result.emplace_back( 2 );
    lock.unlock();

    const std::vector< int > expectedResult = { 1, 2 };

    EXPECT_EQ( result, expectedResult );

    if( thread.joinable() )
        thread.join();
}

}  // namespace UT
}  // namespace HubPlatform
