#include "cameras/tests/http_file_server.h"
#include "cameras/tests/simple_http_storage.h"

#include "cameras/tests/samples.h"

#include "cameras/input_output/camera_input.h"
#include "cameras/input_output/camera_input_options.h"
#include "cameras/input_output/camera_archiver.h"
#include "cameras/input_output/camera_output_options.h"
#include "cameras/input_output/camera_output.h"
#include "cameras/packet/packet_publisher.h"
#include "cameras/storages/local_storage.h"
#include "cameras/storages/local_file_sender.h"
#include "cameras/storages/cloud_storage.h"
#include "system_utils/filesystem.h"
#include "test_utils/temporary_folder_wrapper.h"
#include "json_utils/json_wrapper.h"

#include "common_utils/cpp14.h"

#include <gtest/gtest.h>

using namespace ::testing;

namespace HubPlatform {
namespace UT {

class TestLocalStorage : public LocalStorage {
public:
    FileSenderSharedPtr createFileSender( const std::string& fileName ) override {
        std::lock_guard< std::mutex > lock( m_mutex );
        m_filePaths.push_back( fileName );
        auto sender = std::make_unique< LocalFileSender >( fileName );
        return std::move( sender );
    }
    const std::vector< std::string >& getPaths() const {
        std::lock_guard< std::mutex > lock( m_mutex );
        return m_filePaths;
    }

private:
    mutable std::mutex m_mutex;
    std::vector< std::string > m_filePaths;
};

class CameraArchiverTestSuite : public Test {
public:
    void SetUp() override {
        CameraInputOptions::setMaxDelayBetweenPacketsMsec( 100 );
        CameraInputOptions::setPacketProcessingDelayMsec( 0 );
        CameraInputOptions::setAccumulatedDelayThresholdMsec( DefaultAccumulatedDelayThresholdMsec );
        CameraOutputOptions::setPreRecordBufferSize( 2000 );
        CameraOutputOptions::setRecordBufferSize( 1000 );
        ILocalStorage::setMaxStorageVolume( 100 );
        packetHandler = std::make_shared< PacketHandler >();
        cameraInput = std::make_unique< CameraInput >( packetHandler, PathVideo20secondsMp4 );

        filesystem::createFolder( getTempFolder() + "/sizes" );
    }

    void startArchiver( StorageSharedPtr storage ) const {
        auto archiver = std::make_unique< CameraArchiver >(
            std::move( storage ), 5 /*seconds*/, 5 /*seconds*/, 0, "", packetHandler, getTempFolder() + "/test" );
        constexpr auto duration = std::chrono::seconds( 15 ) /*seconds*/;
        auto future = archiver->init( *cameraInput );
        archiver->start( duration );
        std::this_thread::sleep_for( std::chrono::seconds( 2 ) );
        archiver.reset();
        auto futureStatus = future.wait_for( duration + std::chrono::seconds( 2 ) );
        EXPECT_EQ( futureStatus, std::future_status::ready );
        if( futureStatus == std::future_status::ready ) {
            EXPECT_EQ( future.get(), 0 );
        }
        cameraInput->stop();
    }

    CameraArchiverUniquePtr createArchiverForMetaDataChecking( const Json& userMetaData,
                                                               SaveFileCallback saveFileCallback,
                                                               int segmentsLimit ) const {
        auto storage = std::make_shared< CloudStorage >( getTempFolder() + "/test" );
        storage->setMetaData( userMetaData );
        storage->setSaveFileCallback( std::move( saveFileCallback ) );

        const int prerecordedDurationSec = 0;
        const int segmentDurationSec = 5;
        return std::make_unique< CameraArchiver >(
            std::move( storage ), prerecordedDurationSec, segmentDurationSec, segmentsLimit, "", packetHandler, "" );
    }

    bool checkArchiverAndReadMetaData( const Json& userMetaData,
                                       Json& outMetaData,
                                       int64_t& recordingStartTimestamp ) const {
        auto metaDataPromise = std::make_shared< std::promise< Json > >();
        std::future< Json > metaDataFuture = metaDataPromise->get_future();
        auto saveFileCallback = [metaDataPromise](
                                    const FilePath&, ResultCallback, FileRemoverSharedPtr, const Json& metaData ) {
            metaDataPromise->set_value( metaData );
        };

        const int segmentsLimit = 1;
        auto archiver = createArchiverForMetaDataChecking( userMetaData, saveFileCallback, segmentsLimit );
        archiver->init( *cameraInput );

        const auto timeSinceEpoch = std::chrono::system_clock::now().time_since_epoch();
        recordingStartTimestamp = std::chrono::duration_cast< std::chrono::seconds >( timeSinceEpoch ).count();

        const std::chrono::seconds recordingDuration( 6 );
        archiver->start( recordingDuration );

        const std::future_status status = metaDataFuture.wait_for( recordingDuration );
        EXPECT_EQ( status, std::future_status::ready );
        if( status == std::future_status::ready ) {
            outMetaData = metaDataFuture.get();
        }
        return status == std::future_status::ready;
    }

    std::string getTempFolder() const {
        return m_folderWrapper.getPath();
    }

    CameraOutput& getCameraOutput( CameraArchiver& archiver ) {
        return *archiver.m_cameraOutput;
    }

    void addListenerWithOffset( CameraArchiver& archiver, int64_t offset ) {
        archiver.m_packetHandler->addListener( [&archiver, offset]( const AVPacketPrototype& packet ) {
            auto pkt = packet.clone();
            auto& avPkt = pkt.getPacket();
            avPkt.pts += offset;
            avPkt.dts += offset;
            archiver.m_cameraOutput->receivePacket( pkt );
        } );
    }

    PacketHandlerSharedPtr packetHandler;
    CameraInputUniquePtr cameraInput;

private:
    HubPlatform::Tests::TemporaryFolderWrapper m_folderWrapper;
};

TEST_F( CameraArchiverTestSuite, testLocalFileStorage ) {
    ASSERT_TRUE( cameraInput->start() );
    auto storage = std::make_shared< TestLocalStorage >();
    startArchiver( storage );
    for( const auto& file : storage->getPaths() ) {
        HubPlatform::filesystem::removeFile( file.c_str() );
    }
}

TEST_F( CameraArchiverTestSuite, longSteaming ) {
    ASSERT_TRUE( cameraInput->start() );
    auto storage = std::make_shared< TestLocalStorage >();
    auto archiver = std::make_unique< CameraArchiver >(
        storage, 5 /*seconds*/, 5 /*seconds*/, 0, "", packetHandler, getTempFolder() + "/test" );
    if( auto video = cameraInput->getVideoStream() ) {
        auto resolution = Resolution::fromStream( *video );
        getCameraOutput( *archiver ).addVideoStream( video->index, *video, resolution );
    }
    if( auto audio = cameraInput->getAudioStream() ) {
        getCameraOutput( *archiver ).addAudioStream( audio->index, *audio );
    }
    addListenerWithOffset( *archiver, INT32_MAX );
    auto future = getCameraOutput( *archiver ).allowRecord();

    constexpr auto duration = std::chrono::seconds( 15 ) /*seconds*/;
    archiver->start( duration );
    std::this_thread::sleep_for( std::chrono::seconds( 2 ) );
    archiver.reset();
    auto futureStatus = future.wait_for( duration + std::chrono::seconds( 2 ) );
    EXPECT_EQ( futureStatus, std::future_status::ready );
    if( futureStatus == std::future_status::ready ) {
        EXPECT_EQ( future.get(), 0 );
    }
    cameraInput->stop();
    for( const auto& file : storage->getPaths() ) {
        HubPlatform::filesystem::removeFile( file.c_str() );
    }
}

TEST_F( CameraArchiverTestSuite, audioConverter ) {
    cameraInput = std::make_unique< CameraInput >( packetHandler, MulawAudioTrack );
    auto storage = std::make_shared< TestLocalStorage >();
    auto outputRecord = getTempFolder() + "/audioAAC";
    CameraOutput cameraOutput( storage,
                               outputRecord,
                               /*prerecordedDuration*/ std::chrono::seconds( 0 ),
                               /*segmentDuration*/ std::chrono::seconds( 32 ),
                               /*segmentsLimit*/ 1,
                               /*format*/ "" );
    cameraInput->initStreams( ICameraInput::RtspTransport::Default );

    ASSERT_TRUE( cameraInput->getVideoStream() );
    auto video = cameraInput->getVideoStream();
    auto outputResolution = Resolution::fromStream( *video );
    cameraOutput.addVideoStream( video->index, *video, outputResolution );

    if( auto audio = cameraInput->getAudioStream() ) {
        cameraOutput.addAudioStream( audio->index, *audio );
    }

    cameraOutput.start( std::chrono::seconds( 35 ) );
    cameraOutput.startSegment();
    int ret = 0;
    while( ret == 0 ) {
        AVPacket avPacket;
        ret = cameraInput->readNextPacket( avPacket );
        AVPacketPrototype packetWrapper( avPacket );
        if( ret == 0 ) {
            cameraOutput.processPacket( packetWrapper );
        }
    }
    cameraOutput.stopSegment();

    auto& paths = storage->getPaths();
    ASSERT_EQ( paths.size(), 1 );
    cameraInput = std::make_unique< CameraInput >( packetHandler, paths[ 0 ] );
    cameraInput->initStreams( ICameraInput::RtspTransport::Default );
    auto audio = cameraInput->getAudioStream();
    ASSERT_TRUE( audio );
    EXPECT_EQ( audio->codecpar->codec_id, AV_CODEC_ID_AAC );
    for( const auto& file : paths ) {
        HubPlatform::filesystem::removeFile( file.c_str() );
    }
}

TEST_F( CameraArchiverTestSuite, DISABLED_audioConverterMemoryLeak ) {
    while( true ) {
        cameraInput = std::make_unique< CameraInput >( packetHandler, MulawAudioTrack );
        auto storage = std::make_shared< TestLocalStorage >();
        auto outputRecord = getTempFolder() + "/audioAAC";
        CameraOutput cameraOutput( storage,
                                   outputRecord,
                                   /*prerecordedDuration*/ std::chrono::seconds( 0 ),
                                   /*segmentDuration*/ std::chrono::seconds( 32 ),
                                   /*segmentsLimit*/ 1,
                                   /*format*/ "" );
        cameraInput->initStreams( ICameraInput::RtspTransport::Default );

        ASSERT_TRUE( cameraInput->getVideoStream() );
        auto video = cameraInput->getVideoStream();
        auto outputResolution = Resolution::fromStream( *video );
        cameraOutput.addVideoStream( video->index, *video, outputResolution );

        if( auto audio = cameraInput->getAudioStream() ) {
            cameraOutput.addAudioStream( audio->index, *audio );
        }

        cameraOutput.start( std::chrono::seconds( 35 ) );
        cameraOutput.startSegment();
        int ret = 0;
        while( ret == 0 ) {
            AVPacket avPacket;
            ret = cameraInput->readNextPacket( avPacket );
            AVPacketPrototype packetWrapper( avPacket );
            if( ret == 0 ) {
                cameraOutput.processPacket( packetWrapper );
            }
        }
        cameraOutput.stopSegment();
        storage->getStorageVolume() = 0;

        auto& paths = storage->getPaths();
        ASSERT_EQ( paths.size(), 1 );
        cameraInput = std::make_unique< CameraInput >( packetHandler, paths[ 0 ] );
        cameraInput->initStreams( ICameraInput::RtspTransport::Default );
        auto audio = cameraInput->getAudioStream();
        ASSERT_TRUE( audio );
        EXPECT_EQ( audio->codecpar->codec_id, AV_CODEC_ID_AAC );
        for( const auto& file : paths ) {
            HubPlatform::filesystem::removeFile( file.c_str() );
        }
    }
}

TEST_F( CameraArchiverTestSuite, metaData_storingCorrectStartTimestamp ) {
    ASSERT_TRUE( cameraInput->start() );
    Json metaData;
    int64_t recordingStartTimestamp;
    const bool success = checkArchiverAndReadMetaData( Json(), metaData, recordingStartTimestamp );

    if( success ) {
        EXPECT_TRUE( metaData.is_object() );
        EXPECT_EQ( metaData.size(), 1 );

        auto it = metaData.find( "timestamp" );
        EXPECT_TRUE( it != metaData.end() );
        if( it != metaData.end() ) {
            const Json& timestampValueJson = it.value();
            EXPECT_TRUE( timestampValueJson.is_number() );
            if( timestampValueJson.is_number() ) {
                const int64_t timestampRes = timestampValueJson.get< int64_t >();
                const int64_t diff = timestampRes - recordingStartTimestamp;
                EXPECT_LE( std::abs( diff ), 1 );
            }
        }
    }

    cameraInput->stop();
}

TEST_F( CameraArchiverTestSuite, metaData_overriding ) {
    ASSERT_TRUE( cameraInput->start() );
    const int64_t timestampInitialValue = 0;
    const Json userMetaData = JSON( "timestamp" << timestampInitialValue );

    Json metaData;
    int64_t recordingStartTimestamp;
    const bool success = checkArchiverAndReadMetaData( userMetaData, metaData, recordingStartTimestamp );

    if( success ) {
        EXPECT_TRUE( metaData.is_object() );
        EXPECT_EQ( metaData.size(), 1 );

        auto it = metaData.find( "timestamp" );
        EXPECT_TRUE( it != metaData.end() );
        if( it != metaData.end() ) {
            const Json& timestampValueJson = it.value();
            EXPECT_TRUE( timestampValueJson.is_number() );
            if( timestampValueJson.is_number() ) {
                const int64_t timestampRes = timestampValueJson.get< int64_t >();
                EXPECT_NE( timestampRes, timestampInitialValue );
            }
        }
    }

    cameraInput->stop();
}

TEST_F( CameraArchiverTestSuite, metaData_merging ) {
    ASSERT_TRUE( cameraInput->start() );
    const std::string testProp = "property";
    const std::string testValue = "value";
    const Json userMetaData = JSON( testProp << testValue );

    Json metaData;
    int64_t recordingStartTimestamp;
    const bool success = checkArchiverAndReadMetaData( userMetaData, metaData, recordingStartTimestamp );

    if( success ) {
        EXPECT_TRUE( metaData.is_object() );
        EXPECT_EQ( metaData.size(), 2 );

        auto itTimestamp = metaData.find( "timestamp" );
        EXPECT_TRUE( itTimestamp != metaData.end() );
        auto itTestProp = metaData.find( testProp );
        EXPECT_TRUE( itTestProp != metaData.end() );

        if( itTestProp != metaData.end() ) {
            const Json& testPropJson = itTestProp.value();
            EXPECT_TRUE( testPropJson.is_string() );
            if( testPropJson.is_string() ) {
                const std::string outValue = testPropJson.get< std::string >();
                EXPECT_EQ( outValue, testValue );
            }
        }
    }

    cameraInput->stop();
}

TEST_F( CameraArchiverTestSuite, DISABLED_testLocalHttpStorage ) {
    ASSERT_TRUE( cameraInput->start() );
    const std::string storageIpAddress = "127.0.0.1";
    constexpr int storagePort = 23999;
    auto serverThread = std::thread( [&storageIpAddress]() {
        HttpFileServer server;
        ASSERT_EQ( server.start( storageIpAddress, storagePort ), 0 );
    } );
    serverThread.detach();

    auto storage = std::make_shared< SimpleHttpStorage >();
    std::string url = "http://" + storageIpAddress + ":" + std::to_string( storagePort );
    JsonObject properties;
    properties[ "url" ] = url;
    storage->setProperties( properties );
    startArchiver( std::move( storage ) );
}

}  // namespace UT
}  // namespace HubPlatform
