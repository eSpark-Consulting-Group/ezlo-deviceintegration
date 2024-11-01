#include "cameras/tests/http_file_server.h"
#include "cameras/tests/memory_file_sender.h"

#include "cameras/tests/samples.h"

#include "cameras/input_output/camera_input.h"
#include "cameras/input_output/camera_input_options.h"
#include "cameras/input_output/camera_archiver.h"
#include "cameras/input_output/camera_snapshoter.h"
#include "cameras/input_output/camera_snapshoter_options.h"
#include "cameras/input_output/resolution.h"
#include "cameras/packet/packet_publisher.h"
#include "cameras/storages/local_storage.h"
#include "cameras/storages/cloud_storage.h"
#include "cameras/defs.h"
#include "system_utils/filesystem.h"
#include "test_utils/temporary_folder_wrapper.h"
#include "json_utils/json_utils.h"

#include <memory>
#include <chrono>
#include <utility>

#include <gmock/gmock.h>

using namespace ::testing;
namespace HubPlatform {
namespace UT {

Resolution getJpegResolution( const std::string& jpegFilePath );

class CameraSnapshotTestSuite : public Test {
public:
    void SetUp() override {
        CameraInputOptions::setMaxDelayBetweenPacketsMsec( 1000 );
        CameraInputOptions::setPacketProcessingDelayMsec( 0 );
        CameraInputOptions::setAccumulatedDelayThresholdMsec( DefaultAccumulatedDelayThresholdMsec );
        CameraSnapshoterOptions::setBufferSize( 100 );
        CameraSnapshoterOptions::setQueueLimit( 20 );
        CameraSnapshoterOptions::setTimeoutSec( 10 );
        packetHandler = std::make_shared< PacketHandler >();
        cameraInput = std::unique_ptr< CameraInput >( new CameraInput( packetHandler, PathVideo20secondsMp4 ) );
        filesystem::createFolder( getTempFolder() + "/sizes" );
    }

    std::string getTempFolder() const {
        return m_folderWrapper.getPath();
    }

    std::string getSizesFolder() const {
        return getTempFolder() + "/sizes";
    }

    void testShapshot( CameraSnapshoter& snapshoter,
                       int snapshotNumber,
                       const std::shared_ptr< Resolution >& resolution,
                       int codeLine ) {
        std::string codeLineStr = "code line: " + std::to_string( codeLine );
        auto promise = std::make_shared< std::promise< Json > >();
        auto storage = std::make_shared< CloudStorage >( getTempFolder() );
        FileRemoverSharedPtr fileRemover;
        storage->setSaveFileCallback( [&fileRemover]( const FilePath&,
                                                      const ResultCallback& resultCallback,
                                                      FileRemoverSharedPtr remover,
                                                      const Json& /*metaData*/ ) {
            fileRemover = std::move( remover );
            resultCallback( "success" );
        } );
        ASSERT_TRUE( snapshoter.makeSnapshot(
            storage, [promise]( const Json& message ) { promise->set_value( message ); }, "", resolution ) )
            << codeLineStr;
        std::this_thread::sleep_for( std::chrono::milliseconds( 50 ) );
        cameraInput->run();
        auto future = promise->get_future();
        auto futureStatus = future.wait_for( std::chrono::seconds( 2 ) );
        ASSERT_EQ( futureStatus, std::future_status::ready ) << codeLineStr;
        EXPECT_EQ( future.get(), "success" ) << codeLineStr;
        std::vector< std::string > files;
        ASSERT_NO_THROW( files = filesystem::findAllFiles( getTempFolder() ) ) << codeLineStr;
        ASSERT_EQ( files.size(), 1 ) << codeLineStr;
        ASSERT_NE( files[ 0 ].find( std::to_string( snapshotNumber ) + ".jpg" ), std::string::npos ) << codeLineStr;
        if( resolution ) {
            auto path = getTempFolder() + "/" + files[ 0 ];
            auto sizeFilePath = getSizesFolder() + "/" + files[ 0 ] + ".size";
            filesystem::createFolder( sizeFilePath );
            auto jpegRes = getJpegResolution( path );
            if( resolution->width != 0 ) {
                EXPECT_EQ( jpegRes.width, resolution->width ) << codeLineStr;
            }
            if( resolution->height != 0 ) {
                EXPECT_EQ( jpegRes.height, resolution->height ) << codeLineStr;
            }
        }
    };

    void testShapshot( CameraSnapshoter& snapshoter, int snapshotNumber, int codeLine ) {
        testShapshot( snapshoter, snapshotNumber, nullptr, codeLine );
    }

    PacketHandlerSharedPtr packetHandler;
    CameraInputUniquePtr cameraInput;

private:
    HubPlatform::Tests::TemporaryFolderWrapper m_folderWrapper;
};

class TestFileSender : public FileSender {
public:
    bool finalizeFile() override {
        m_resultCallback( "success" );
        return true;
    }
    bool sendData( uint8_t* /*data*/, size_t /*size*/ ) override {
        return true;
    }
};

class TestStorage : public Storage {
public:
    using Bytes = std::atomic_llong;
    FileSenderSharedPtr createFileSender( const std::string& /*fileName*/ ) override {
        return std::make_shared< TestFileSender >();
    }

    void setProperties( const JsonObject& ) override {}
    void clearStorage() override {}

    int64_t getMaxStorageVolume() override {
        return 0;
    }
};

Resolution getJpegResolution( const std::string& jpegFilePath ) {
    int height = 0, width = 0, pos = 0;
    FILE* fp = fopen( jpegFilePath.c_str(), "rb" );
    fseek( fp, 0, SEEK_END );
    size_t len = ftell( fp );
    fseek( fp, 0, SEEK_SET );

    auto* imageBuffer = (unsigned char*) malloc( len + 1 );
    fread( imageBuffer, 1, len, fp );
    fclose( fp );

    /*Extract start of frame marker(FFCO) of width and hight and get the position*/
    for( size_t i = 0; i < len; i++ ) {
        if( ( imageBuffer[ i ] == 0xFF ) && ( imageBuffer[ i + 1 ] == 0xC0 ) ) {
            pos = i;
            break;
        }
    }
    /*Moving to the particular byte position and assign byte value to pointer variable*/
    pos = pos + 5;
    height = imageBuffer[ pos ] << 8 | imageBuffer[ pos + 1 ];
    width = imageBuffer[ pos + 2 ] << 8 | imageBuffer[ pos + 3 ];
    free( imageBuffer );
    return { width, height };
}

TEST_F( CameraSnapshotTestSuite, localStorage ) {
    CameraSnapshoter snapshoter( "id1" );
    cameraInput->pause();
    ASSERT_TRUE( cameraInput->start() );
    ASSERT_NO_THROW( snapshoter.init( *cameraInput ) );
    auto promise = std::make_shared< std::promise< Json > >();
    auto storage = std::make_shared< LocalStorage >( getTempFolder() );
    snapshoter.makeSnapshot( storage, [promise]( const Json& message ) { promise->set_value( message ); }, "" );
    std::this_thread::sleep_for( std::chrono::milliseconds( 50 ) );
    cameraInput->run();
    auto future = promise->get_future();
    auto futureStatus = future.wait_for( std::chrono::seconds( 2 ) );
    ASSERT_EQ( futureStatus, std::future_status::ready );
    auto message = future.get();
    auto filePath = json_get_str( message, "filePath" );
    ASSERT_NE( filePath.find( getTempFolder() ), std::string::npos );
    ASSERT_NE( filePath.find( "0.jpg" ), std::string::npos );
    HubPlatform::filesystem::removeFile( filePath.c_str() );
}

TEST_F( CameraSnapshotTestSuite, successCase ) {
    CameraSnapshoter snapshoter( "id1" );
    cameraInput->pause();
    ASSERT_TRUE( cameraInput->start() );
    ASSERT_NO_THROW( snapshoter.init( *cameraInput ) );
    testShapshot( snapshoter, 0, __LINE__ );

    cameraInput->pause();
    ASSERT_TRUE( cameraInput->start() );
    ASSERT_NO_THROW( snapshoter.init( *cameraInput ) );
    testShapshot( snapshoter, 1, __LINE__ );

    cameraInput->pause();
    ASSERT_TRUE( cameraInput->start() );
    ASSERT_NO_THROW( snapshoter.init( *cameraInput ) );
    testShapshot( snapshoter, 2, __LINE__ );
}

TEST_F( CameraSnapshotTestSuite, timeout ) {
    auto storage = std::make_shared< CloudStorage >( getTempFolder() );
    FileRemoverSharedPtr fileRemover;
    storage->setSaveFileCallback( [&fileRemover]( const FilePath&,
                                                  const ResultCallback& resultCallback,
                                                  FileRemoverSharedPtr remover,
                                                  const Json& /*metaData*/ ) {
        fileRemover = std::move( remover );
        resultCallback( "success" );
    } );
    CameraSnapshoterOptions::setTimeoutSec( 1 );
    CameraSnapshoter snapshoter( "id1" );
    ASSERT_TRUE( cameraInput->start() );
    ASSERT_NO_THROW( snapshoter.init( *cameraInput ) );
    std::this_thread::sleep_for( std::chrono::milliseconds( 50 ) );
    auto promise = std::make_shared< std::promise< Json > >();
    ASSERT_TRUE(
        snapshoter.makeSnapshot( storage, [promise]( const Json& message ) { promise->set_value( message ); }, "" ) );
    auto future = promise->get_future();
    auto futureStatus = future.wait_for( std::chrono::seconds( 3 ) );
    ASSERT_EQ( futureStatus, std::future_status::ready );
    EXPECT_EQ( future.get(),
               JSON( "error" << JSON( "message"
                                      << "snapshot timeout" ) ) );
    std::this_thread::sleep_for( std::chrono::milliseconds( 50 ) );
    auto files = filesystem::findAllFiles( getTempFolder() );
    ASSERT_EQ( files.size(), 0 );
}

TEST_F( CameraSnapshotTestSuite, snapshotsInSameTime ) {
    CameraSnapshoter snapshoter1( "id1" );
    CameraSnapshoter snapshoter2( "id2" );
    auto cameraInput2 =
        std::unique_ptr< CameraInput >( new CameraInput( std::make_shared< PacketHandler >(), PathVideo20secondsMp4 ) );
    cameraInput->pause();
    cameraInput2->pause();
    ASSERT_TRUE( cameraInput->start() );
    ASSERT_TRUE( cameraInput2->start() );
    ASSERT_NO_THROW( snapshoter1.init( *cameraInput ) );
    ASSERT_NO_THROW( snapshoter2.init( *cameraInput2 ) );
    auto promise1 = std::make_shared< std::promise< Json > >();
    auto promise2 = std::make_shared< std::promise< Json > >();
    auto storage = std::make_shared< LocalStorage >( getTempFolder() );
    snapshoter1.makeSnapshot( storage, [promise1]( const Json& message ) { promise1->set_value( message ); }, "" );
    snapshoter2.makeSnapshot( storage, [promise2]( const Json& message ) { promise2->set_value( message ); }, "" );
    std::this_thread::sleep_for( std::chrono::milliseconds( 50 ) );
    cameraInput->run();
    cameraInput2->run();

    auto future1 = promise1->get_future();
    auto futureStatus1 = future1.wait_for( std::chrono::seconds( 3 ) );
    ASSERT_EQ( futureStatus1, std::future_status::ready );

    auto future2 = promise2->get_future();
    auto futureStatus2 = future2.wait_for( std::chrono::seconds( 3 ) );
    ASSERT_EQ( futureStatus2, std::future_status::ready );

    std::vector< std::string > files;
    ASSERT_NO_THROW( files = filesystem::findAllFiles( getTempFolder() ) );
    ASSERT_EQ( files.size(), 2 );
    ASSERT_TRUE( files[ 0 ].find( "id1" ) != std::string::npos || files[ 1 ].find( "id1" ) != std::string::npos );
    ASSERT_TRUE( files[ 0 ].find( "id2" ) != std::string::npos || files[ 1 ].find( "id2" ) != std::string::npos );
}

TEST_F( CameraSnapshotTestSuite, differentResolutions ) {
    CameraSnapshoter snapshoter( "id1" );
    cameraInput->pause();
    ASSERT_TRUE( cameraInput->start() );
    ASSERT_NO_THROW( snapshoter.init( *cameraInput ) );
    auto resolution = std::make_shared< Resolution >( 800, 600 );
    testShapshot( snapshoter, 0, resolution, __LINE__ );

    cameraInput->pause();
    ASSERT_TRUE( cameraInput->start() );
    ASSERT_NO_THROW( snapshoter.init( *cameraInput ) );
    resolution = std::make_shared< Resolution >( 340, 220 );
    testShapshot( snapshoter, 1, resolution, __LINE__ );

    cameraInput->pause();
    ASSERT_TRUE( cameraInput->start() );
    ASSERT_NO_THROW( snapshoter.init( *cameraInput ) );
    resolution = std::make_shared< Resolution >( 2000, 1500 );
    testShapshot( snapshoter, 2, resolution, __LINE__ );

    cameraInput->pause();
    ASSERT_TRUE( cameraInput->start() );
    ASSERT_NO_THROW( snapshoter.init( *cameraInput ) );
    resolution = std::make_shared< Resolution >( 256, 0 );
    testShapshot( snapshoter, 3, resolution, __LINE__ );

    cameraInput->pause();
    ASSERT_TRUE( cameraInput->start() );
    ASSERT_NO_THROW( snapshoter.init( *cameraInput ) );
    cameraInput->run();
    resolution = std::make_shared< Resolution >( 0, 256 );
    testShapshot( snapshoter, 4, resolution, __LINE__ );
}

TEST_F( CameraSnapshotTestSuite, withoutSavingCallback ) {
    auto storage = std::make_shared< CloudStorage >( getTempFolder() );
    CameraSnapshoter snapshoter( "id1" );
    cameraInput->pause();
    ASSERT_TRUE( cameraInput->start() );
    ASSERT_NO_THROW( snapshoter.init( *cameraInput ) );
    auto promise = std::make_shared< std::promise< Json > >();
    Log::info( cc::info( "makeSnapshot" ) );
    ASSERT_TRUE(
        snapshoter.makeSnapshot( storage, [promise]( const Json& message ) { promise->set_value( message ); }, "" ) );
    Log::info( cc::info( "sleep_for" ) );
    std::this_thread::sleep_for( std::chrono::milliseconds( 50 ) );
    Log::info( cc::info( "run" ) );
    cameraInput->run();
    auto future = promise->get_future();
    Log::info( cc::info( "wait_for" ) );
    auto futureStatus = future.wait_for( std::chrono::seconds( 2 ) );
    Log::info( cc::info( "futureStatus" ) );
    ASSERT_EQ( futureStatus, std::future_status::ready );
    EXPECT_EQ( future.get(),
               JSON( "error" << JSON( "message"
                                      << "no file saving callback" ) ) );
    LOGGER_INFO( cc::info( "finish" ) );
}

TEST_F( CameraSnapshotTestSuite, returnCallbackNotCalled ) {
    auto storage = std::make_shared< CloudStorage >( getTempFolder() );
    storage->setSaveFileCallback(
        []( const FilePath&, const ResultCallback&, const FileRemoverSharedPtr&, const Json& /*metaData*/ ) {} );
    CameraSnapshoter snapshoter( "id1" );
    cameraInput->pause();
    ASSERT_TRUE( cameraInput->start() );
    ASSERT_NO_THROW( snapshoter.init( *cameraInput ) );
    auto promise = std::make_shared< std::promise< Json > >();
    ASSERT_TRUE(
        snapshoter.makeSnapshot( storage, [promise]( const Json& message ) { promise->set_value( message ); }, "" ) );
    std::this_thread::sleep_for( std::chrono::milliseconds( 50 ) );
    cameraInput->run();
    auto future = promise->get_future();
    auto futureStatus = future.wait_for( std::chrono::seconds( 1 ) );
    ASSERT_EQ( futureStatus, std::future_status::timeout );
}

TEST_F( CameraSnapshotTestSuite, DISABLED_memoryLeakTest ) {
    CameraSnapshoter snapshoter( "id1" );
    auto testStorage = std::make_shared< TestStorage >();
    for( std::size_t i = 1; i < 20000000L; ++i ) {
        cameraInput = std::unique_ptr< CameraInput >( new CameraInput( packetHandler, PathVideo20secondsMp4 ) );
        cameraInput->pause();
        ASSERT_TRUE( cameraInput->start() );
        ASSERT_NO_THROW( snapshoter.init( *cameraInput ) );
        auto resolution = std::make_shared< Resolution >( 2000, 1500 );
        auto promise = std::make_shared< std::promise< Json > >();
        ASSERT_TRUE( snapshoter.makeSnapshot(
            testStorage, [promise]( const Json& message ) { promise->set_value( message ); }, "", resolution ) );
        std::this_thread::sleep_for( std::chrono::milliseconds( 50 ) );
        cameraInput->run();
        auto future = promise->get_future();
        auto futureStatus = future.wait_for( std::chrono::seconds( 5 ) );
        ASSERT_EQ( futureStatus, std::future_status::ready );
        EXPECT_EQ( future.get(), "success" );
    }
}

}  // namespace UT
}  // namespace HubPlatform
