#include "cloud_storage/upload_context.h"
#include <iostream>
#include <fstream>
#include "log_utils/ezlo_log.h"
#include "base_utils/utils.h"
#include "tiny/tiny_consolecolors.h"
#include "system_utils/filesystem.h"
#include <gtest/gtest.h>

namespace HubPlatform {
namespace Common {
namespace UT {

class UploadContextTests : public ::testing::Test {
public:
    static std::string makeTempFileName();
};

std::string UploadContextTests::makeTempFileName() {
    char tempFileName[] = "/tmp/fw_tests_upload_context_tmp.XXXXXX";
    close( mkstemp( tempFileName ) );
    return tempFileName;
}

namespace {
struct StreamIterator {
    size_t totalData = 0;
    size_t currentPos = 0;
};
}  // namespace

TEST_F( UploadContextTests, uploadFromFileContext ) {
    using namespace std;
    std::string fileName = makeTempFileName();
    utils::AfterReturn onExit( [fileName]() {
        LOGGER_DEBUG_RAW( "remove file: " + fileName );
        filesystem::removeFile( fileName );
    } );

    const int portion = 1024;
    const int chunks = 3;

    StreamIterator iter;
    std::vector< StreamIterator > fetchedPortions;

    fstream output;
    output.open( fileName, ios::out | ios::binary );

    if( !output ) {
        output.close();
        FAIL() << "failed to fulfill a data file: " << fileName;
    }

    for( int i = 0; i < chunks; i++ ) {
        std::string data( portion, '\0' );
        output << data;
        iter.totalData += data.size();
    }
    output.close();

    using namespace HubPlatform::Common::CloudStorageInterface;
    auto uploadContext = makeFileUploadContext( fileName );

    ASSERT_TRUE( uploadContext != nullptr );
    EXPECT_FALSE( uploadContext->error() );
    EXPECT_FALSE( uploadContext->endOfStream() );

    // reading
    while( !uploadContext->endOfStream() && !uploadContext->error() ) {
        const size_t PORTION = 1000;
        std::string buffer = uploadContext->readPortion( PORTION );
        auto bytesRead = buffer.size();
        iter.currentPos += bytesRead;

        fetchedPortions.push_back( iter );

        if( buffer.size() < PORTION || uploadContext->endOfStream() ) {
            LOGGER_DEBUG( "End of stream by expected input size, last=true" );
            break;
        }
        else {
            LOGGER_DEBUG( "Continue streaming, last=false" );
        }
    }
    EXPECT_FALSE( uploadContext->error() );
    EXPECT_TRUE( uploadContext->endOfStream() );
    EXPECT_EQ( iter.currentPos, portion * chunks ) << "Data put to a file matches data length read from a file";
    EXPECT_EQ( fetchedPortions.size(), 4 ) << "number of portions should match: (1000, 1000, 1000, 72)";
}

TEST_F( UploadContextTests, uploadFromEmptyFile ) {
    using namespace std;
    std::string fileName = makeTempFileName();
    utils::AfterReturn onExit( [fileName]() {
        LOGGER_DEBUG_RAW( "remove file: " + fileName );
        filesystem::removeFile( fileName );
    } );
    ASSERT_EQ( filesystem::getFileSize( fileName ), 0 ) << "file size should be 0";

    using namespace HubPlatform::Common::CloudStorageInterface;
    auto uploadContext = makeFileUploadContext( fileName );

    ASSERT_TRUE( uploadContext != nullptr );
    EXPECT_FALSE( uploadContext->error() );
    EXPECT_TRUE( uploadContext->endOfStream() ) << "Nothing to read";
}

}  // namespace UT
}  // namespace Common
}  // namespace HubPlatform