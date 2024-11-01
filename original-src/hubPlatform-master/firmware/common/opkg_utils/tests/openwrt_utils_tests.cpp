#include "opkg_utils/packages/openwrt_utils.h"

#include "system_utils/filesystem.h"
#include "test_utils/temporary_folder_wrapper.h"

#include "log_utils/ezlo_log.h"
#include "common_utils/cpp14.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <fstream>

/*

TESTPLAN

DONE    1 Checksum tests
DONE        1.1 Replace MD5 checksum path
DONE        1.2 Verify MD5 checksum
DONE    2 Utils tests
DONE        2.1 Download sysupgrade image

*/

namespace HubPlatform {
namespace UT {

using namespace ::testing;

class OpenwrtUtilsTestSuite : public Test {
public:
    void writeFile( const std::string& content, const std::string& filePath ) {
        std::ofstream file( filePath, std::ofstream::out );
        file << content;
        file.flush();
        file.close();
    }
};

TEST_F( OpenwrtUtilsTestSuite, replaceMD5ChecksumPath /*1.1*/ ) {
    HubPlatform::Tests::TemporaryFolderWrapper tmpDir;

    std::string checksumFile = tmpDir.getPath() + "/checksum.md5";

    std::string checksum = "f2396be5c8592146475d4a75d59f40c0";
    std::string contentA = checksum + "  imageA.img\n";
    writeFile( contentA, checksumFile );

    auto result = OpenwrtUtils::replaceMD5ChecksumPath( checksumFile, "/path/to/imageB.img" );
    EXPECT_EQ( result.second, 0 ) << "ERROR: " + result.first;

    std::string line;
    std::ifstream file( checksumFile );
    EXPECT_TRUE( std::getline( file, line ) );
    std::string contentB = checksum + "  /path/to/imageB.img";
    EXPECT_EQ( line.find( contentB ), 0 );
}

TEST_F( OpenwrtUtilsTestSuite, virifyMD5Checksum /*1.2*/ ) {
    HubPlatform::Tests::TemporaryFolderWrapper tmpDir;
    std::string checksumPath = tmpDir.getPath() + "/checksum.md5";
    std::string filePath = tmpDir.getPath() + "/file.img";

    std::string fileData = "dummy_data\n";
    std::string checksum = "f2396be5c8592146475d4a75d59f40c0";

    writeFile( checksum + "  " + filePath, checksumPath );

    {
        writeFile( fileData, filePath );
        auto result = OpenwrtUtils::verifyMD5Checksum( checksumPath );
        EXPECT_EQ( result.second, 0 ) << "ERROR: " + result.first;
    }

    {
        writeFile( "corrupted_file", filePath );
        auto result = OpenwrtUtils::verifyMD5Checksum( checksumPath );
        EXPECT_NE( result.second, 0 );
    }
}

TEST_F( OpenwrtUtilsTestSuite, downloadSysupgradeImage /*2.1*/ ) {
    HubPlatform::Tests::TemporaryFolderWrapper remoteDirectory;

    std::string fileData = "dummy_data\n";
    std::string fileDataMD5Checksum = "f2396be5c8592146475d4a75d59f40c0";
    std::string filename = "sysupgrade.img.gz";
    std::string remoteFilePath = remoteDirectory.getPath() + "/" + filename;

    auto downloader = []( const std::string& url, const std::string& file ) -> OpenwrtUtils::StdOutWithReturnCode {
        if( !filesystem::isFileExist( url ) ) {
            return { "Fake downloader: file not found: " + url, 1 };
        }
        HubPlatform::filesystem::copyFile( url, file );
        return { "\n", 0 };
    };

    writeFile( fileData, remoteFilePath );

    {
        HubPlatform::Tests::TemporaryFolderWrapper cacheDirectory;
        auto result = OpenwrtUtils::downloadImage( downloader, remoteFilePath, cacheDirectory.getPath() );
        EXPECT_NE( result.second, 0 ) << "Checksum file does not exist";
    }

    writeFile( fileDataMD5Checksum + "  bundle_image_name.img.gz", remoteFilePath + ".md5" );

    {
        HubPlatform::Tests::TemporaryFolderWrapper cacheDirectory;
        auto result = OpenwrtUtils::downloadImage( downloader, remoteFilePath, cacheDirectory.getPath() );
        EXPECT_EQ( result.second, 0 );
    }

    {
        HubPlatform::Tests::TemporaryFolderWrapper cacheDirectory;
        writeFile( "invalid_data", remoteFilePath );
        auto result = OpenwrtUtils::downloadImage( downloader, remoteFilePath, cacheDirectory.getPath() );
        EXPECT_NE( result.second, 0 );
    }
}

}  // namespace UT
}  // namespace HubPlatform
