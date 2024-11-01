#include "device_firmware_update_manager/private/default_downloader.h"
#include "test_utils/temporary_folder_wrapper.h"
#include "system_utils/filesystem.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <fstream>
#include <iostream>
#include <condition_variable>
#include <curl/curl.h>
#include <openssl/ssl.h>

namespace HubPlatform {
namespace UT {

class DefaultDownloaderTestSuite : public testing::Test {
public:
    const std::string& getTempFolder() {
        return m_folderWrapper.getPath();
    }

private:
    Tests::TemporaryFolderWrapper m_folderWrapper;
};

TEST_F( DefaultDownloaderTestSuite, fileCopy ) {
    const auto& tempFolder = getTempFolder();
    auto srcFilePath = tempFolder + "/src.txt";
    auto dstFilePath = tempFolder + "/dst.txt";
    {
        std::ofstream file( srcFilePath );
        file << "testdata";
    }

    std::condition_variable condVar;
    std::mutex mutex;
    std::atomic_bool done( false );
    DefaultDownloader downloader(
        { srcFilePath, "" },
        dstFilePath,
        [&done, &condVar, &mutex]( const DeviceFirmwareUpdateManager::Downloader& downloader ) {
            if( downloader.isOk() && downloader.getProgress() == 100 ) {
                {
                    std::lock_guard< std::mutex > lock( mutex );
                    done = true;
                }
                condVar.notify_all();
            }
        } );
    std::unique_lock< std::mutex > lock( mutex );
    ASSERT_TRUE( condVar.wait_for( lock, std::chrono::seconds( 1 ), [&done]() -> bool { return done; } ) );
    EXPECT_TRUE( downloader.isOk() );
    EXPECT_EQ( downloader.getProgress(), 100 );
    std::string data;
    {
        std::ifstream file( dstFilePath );
        file >> data;
    }
    EXPECT_EQ( data, "testdata" );
}

TEST_F( DefaultDownloaderTestSuite, DISABLED_downloadFromHttp ) {
    ::SSL_library_init();
    ::SSL_load_error_strings();
    ::ERR_load_BIO_strings();
    ::OpenSSL_add_all_algorithms();
    ::OpenSSL_add_all_digests();
    curl_global_init( CURL_GLOBAL_DEFAULT );

    const auto& tempFolder = getTempFolder();
    auto dstFilePath = tempFolder + "/MultiSensor 6_US_v_1.13.hec";
    std::cout << tempFolder << std::endl << std::flush;
    auto imageLink = "http://127.0.0.1:8080/MultiSensor%206_US_v_1.13.hec";
    auto checksum = "http://127.0.0.1:8080/checksum.sh1";
    std::condition_variable condVar;
    std::mutex mutex;
    std::atomic_bool done( false );
    DefaultDownloader downloader( { imageLink, checksum },
                                  dstFilePath,
                                  [&done, &condVar]( const DeviceFirmwareUpdateManager::Downloader& downloader ) {
                                      if( downloader.getProgress() == 100 || !downloader.isOk() ) {
                                          done = true;
                                          condVar.notify_all();
                                      }
                                  } );
    std::unique_lock< std::mutex > lock( mutex );
    ASSERT_TRUE( condVar.wait_for( lock, std::chrono::seconds( 10 ), [&done]() -> bool { return done; } ) );
    EXPECT_TRUE( downloader.isOk() );
    EXPECT_EQ( downloader.getProgress(), 100 );
    EXPECT_TRUE( filesystem::isExist( dstFilePath ) );
}

}  // namespace UT
}  // namespace HubPlatform
