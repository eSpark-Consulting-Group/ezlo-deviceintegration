#include "uid/extensions/cameras/cameras_audio_stream_manager.h"
#include "service_interfaces/tests/fakes/fake_service.h"

#include "database_client/dbqueryparser.h"
#include "service_interfaces/service.h"
#include "database_client/idbclient.h"
#include "json_utils/json_utils.h"

#include <gtest/gtest.h>
#include <fstream>
#include <thread>
#include <map>
#include <utility>

namespace HubPlatform {
namespace Cameras {
namespace UT {

/*

TESTPLAN

MANUAL  1. Send AAC file to VC702 camera
MANUAL  2. Send WAV file to VC703 camera

DONE    3. Send stream interrupted notifications at a startup

*/

using namespace ::testing;

class StreamEventsHandlerFake final : public StreamEventsHandler {
public:
    void onStreamStarted( const std::string& streamId, const JsonObject& userData ) override {
        Log::info( cc::info( "onStreamStarted " ) + cc::note( streamId ) );
        streamStartedNotifications[ streamId ] = userData;
    }
    void onStreamStopped( const std::string& streamId, const JsonObject& userData ) override {
        Log::info( cc::info( "onStreamStopped " ) + cc::note( streamId ) );
        streamStoppedNotifications[ streamId ] = userData;
    }
    void onStreamError( const std::string& streamId,
                        const JsonObject& userData,
                        const std::string& errorMessage ) override {
        Log::info( cc::info( "onStreamError " ) + cc::note( streamId ) + " " + cc::error( errorMessage ) );
        streamErrorNotifications[ streamId ] = std::make_pair( userData, errorMessage );
    }

    std::map< std::string, Json > streamStartedNotifications;
    std::map< std::string, Json > streamStoppedNotifications;
    std::map< std::string, std::pair< Json, std::string > > streamErrorNotifications;
};

class AudioStreamManagerTestSuite : public Test {
public:
    AudioStreamManagerTestSuite()
    : m_manager( m_streamEventsHandler, m_fakeService ) {}

    StreamEventsHandlerFake& eventsHandler() {
        return m_streamEventsHandler;
    }

    AudioStreamManager& manager() {
        return m_manager;
    }

private:
    Services::UT::FakeService m_fakeService;
    StreamEventsHandlerFake m_streamEventsHandler;
    AudioStreamManager m_manager;
};

TEST_F( AudioStreamManagerTestSuite, DISABLED_testSendAacFile /*1*/ ) {
    auto streamId = "5f8ef923129e0b6e6cfc168b";
    AudioStreamInfo info;
    info.streamId = streamId;
    info.requestInfo.format = RequestInfo::Format::Hikvision;
    info.requestInfo.hv = std::make_unique< RequestHVInfo >();
    info.requestInfo.hv->user = "admin";
    info.requestInfo.hv->password = "<password>";
    info.requestInfo.hv->url = "http://<address>:3580/ISAPI/System/TwoWayAudio/channels/1/audioData";
    info.requestInfo.hv->headers.emplace_back( "Content-Type", "application/octet-stream" );
    info.requestInfo.hv->headers.emplace_back( "Content-Length", "0" );
    info.requestInfo.hv->headers.emplace_back( "Connection", "keep-alive" );

    Log::info( cc::info( "Creating stream" ) );
    auto streamPtr = manager().createInputAudioStream( std::move( info ) );
    ASSERT_TRUE( streamPtr );
    Log::info( cc::info( "Created" ) );

    ASSERT_TRUE( streamPtr->startDataSending() );
    std::array< char, 1024 * 64 > buffer;
    std::ifstream file( "<filepath>/ff-16b-1c-16000hz_max.aac" );
    while( true ) {
        if( !file.read( buffer.data(), 7 ) )
            break;

        int32_t payloadLength =
            ( static_cast< uint8_t >( buffer[ 4 ] ) << 3u ) + ( static_cast< uint8_t >( buffer[ 5 ] ) >> 5u ) - 7;

        if( !file.read( buffer.data() + 7, payloadLength ) )
            break;

        int32_t frameLength = payloadLength + 7;
        if( !streamPtr->send( reinterpret_cast< char* >( &frameLength ), 4 ) ) {
            ADD_FAILURE() << "Can not send frame length";
            break;
        }
        if( !streamPtr->send( buffer.data(), frameLength ) ) {
            ADD_FAILURE() << "Can not send frame";
            break;
        }

        std::this_thread::sleep_for(
            std::chrono::milliseconds( frameLength < 100 ? 1 : ( frameLength < 400 ? 35 : 60 ) ) );
    }

    ASSERT_TRUE( streamPtr->stopDataSending() );

    std::this_thread::sleep_for( std::chrono::seconds( 1 ) );
    streamPtr->stop();
    manager().deleteInputAudioStream( streamId );
}

TEST_F( AudioStreamManagerTestSuite, DISABLED_testSendWavFile /*2*/ ) {
    auto streamId = "60827d5394f1af5b85aebd61";
    AudioStreamInfo info;
    info.streamId = streamId;
    info.requestInfo.format = RequestInfo::Format::Foscam;
    info.requestInfo.fc = std::make_unique< RequestFCInfo >();
    info.requestInfo.fc->address = "<address>";
    info.requestInfo.fc->port = 49587;
    info.requestInfo.fc->magic = "EZLO";
    info.requestInfo.fc->user = "admin";
    info.requestInfo.fc->password = "<password>";

    Log::info( cc::info( "Creating stream" ) );
    auto streamPtr = manager().createInputAudioStream( std::move( info ) );
    ASSERT_TRUE( streamPtr );
    Log::info( cc::info( "Created" ) );

    ASSERT_TRUE( streamPtr->startDataSending() );
    constexpr size_t BufferSize = 960;
    std::array< char, BufferSize > buffer;
    std::ifstream file( "<filepath>/pcm1608m.wav" );
    while( true ) {
        if( !file.read( buffer.data(), BufferSize ) )
            break;

        if( !streamPtr->send( buffer.data(), BufferSize ) ) {
            ADD_FAILURE() << "Can not send input data chunk";
            break;
        }

        std::this_thread::sleep_for( std::chrono::milliseconds( 50 ) );
    }

    ASSERT_TRUE( streamPtr->stopDataSending() );

    std::this_thread::sleep_for( std::chrono::seconds( 1 ) );
    streamPtr->stop();
    manager().deleteInputAudioStream( streamId );
}

TEST( AudioStreamManagerTestSuite_, testSendStreamInterruptedNotifications /*3*/ ) {
    Services::UT::FakeService fakeService;
    StreamEventsHandlerFake streamEventsHandler;

    fakeService.getDBClient().insert( UiExtensionsCamerasDBName,
                                      JSON( db::ID_FIELD_NAME << "123"
                                                              << "userData" << JSON( "a" << 1 ) ) );
    fakeService.getDBClient().insert( UiExtensionsCamerasDBName,
                                      JSON( db::ID_FIELD_NAME << "456"
                                                              << "userData" << JSON( "a" << 2 ) ) );

    AudioStreamManager m_manager( streamEventsHandler, fakeService );

    ASSERT_TRUE( streamEventsHandler.streamErrorNotifications.count( "123" ) == 1 );
    ASSERT_TRUE( streamEventsHandler.streamErrorNotifications.count( "456" ) == 1 );

    EXPECT_EQ( streamEventsHandler.streamErrorNotifications[ "123" ].first, JSON( "a" << 1 ) );
    EXPECT_EQ( streamEventsHandler.streamErrorNotifications[ "123" ].second, "Stream was interrupted" );

    EXPECT_EQ( streamEventsHandler.streamErrorNotifications[ "456" ].first, JSON( "a" << 2 ) );
    EXPECT_EQ( streamEventsHandler.streamErrorNotifications[ "456" ].second, "Stream was interrupted" );
}

}  // namespace UT
}  // namespace Cameras
}  // namespace HubPlatform
