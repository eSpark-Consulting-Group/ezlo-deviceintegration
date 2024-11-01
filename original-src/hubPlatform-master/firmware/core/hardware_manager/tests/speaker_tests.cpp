#ifdef __HUB_WITH_SPEAKER__

/*

TESTPLAN

DONE    1. Speaker playback and events
DONE        1.1 Play command
DONE        1.2 Play continuous command
DONE        1.3 Play and stop
DONE        1.4 Play command which should fail in command handler
DONE        1.5 Play and get command info

*/

#include "hardware_manager/tests/fixtures/speaker_fixture.h"
#include "hardware_manager/speaker/remote/speaker_remote_interface.h"

#include <gtest/gtest.h>
#include <deque>
#include <string>
#include <thread>

std::ostream& operator<<( std::ostream& stream, const HubPlatform::HardwareManager::Speaker::PlayingStatus& status ) {
    stream << HubPlatform::HardwareManager::Speaker::toString( status );
    return stream;
}

namespace HubPlatform {
namespace HardwareManager {
namespace UT {

using ::testing::AtLeast;

class SpeakerTests : public SpeakerFixture {};

TEST_F( SpeakerTests, playCommand ) /*1.1*/ {
    pid_t playingPid = 13;
    std::string handlerPath = "HUB:soma_path_for_handler";
    Json userData = { { "id", "some_id" } };

    EXPECT_CALL( m_environment, fork )
        .WillOnce(
            [this, playingPid]( const std::string& shellCommand, Process::Forker::OnProcessCompletion callBack ) {
                addForkProcess( playingPid, callBack );
                return playingPid;
            } );

    EXPECT_CALL( m_environment, sendExecutePluginScript )
        .WillOnce( [&]( const std::string& script, const Json& scriptParams ) {
            ASSERT_EQ( script, handlerPath );
            ASSERT_EQ( scriptParams[ "status" ], "playing" );
            ASSERT_EQ( scriptParams[ "user_data" ], userData );
        } );

    // if time is 0 than play sound only once
    auto commandId = m_speaker->play( "HUB:some_path", 0, 100, "siren", handlerPath, userData );
    ASSERT_FALSE( commandId.empty() ) << "speaker.play should return commandId: string";

    EXPECT_CALL( m_environment, sendExecutePluginScript )
        .WillOnce( [&]( const std::string& script, const Json& scriptParams ) {
            ASSERT_EQ( script, handlerPath );
            ASSERT_EQ( scriptParams[ "status" ], "finished" );
            ASSERT_EQ( scriptParams[ "user_data" ], userData );
        } );

    finishForkProcess( playingPid, Process::Forker::Status::Ok, 0, {} );
}

TEST_F( SpeakerTests, playContinuousCommand ) /*1.2*/ {
    pid_t playingPid = 13;
    std::string handlerPath = "HUB:soma_path_for_handler";
    Json userData = { { "id", "some_id" } };

    EXPECT_CALL( m_environment, fork )
        .WillOnce(
            [this, playingPid]( const std::string& shellCommand, Process::Forker::OnProcessCompletion callBack ) {
                addForkProcess( playingPid, callBack );
                return playingPid;
            } );

    EXPECT_CALL( m_environment, sendExecutePluginScript )
        .WillOnce( [&]( const std::string& script, const Json& scriptParams ) {
            ASSERT_EQ( script, handlerPath );
            ASSERT_EQ( scriptParams[ "status" ], "playing" );
            ASSERT_EQ( scriptParams[ "user_data" ], userData );
        } );

    // if time is -1 then play sound infinity time
    auto commandId = m_speaker->play( "HUB:some_path", -1, 100, "siren", handlerPath, userData );
    ASSERT_FALSE( commandId.empty() ) << "speaker.play should return commandId: string";

    pid_t playingPid2 = 131;

    EXPECT_CALL( m_environment, fork )
        .WillOnce(
            [this, playingPid2]( const std::string& shellCommand, Process::Forker::OnProcessCompletion callBack ) {
                addForkProcess( playingPid2, callBack );
                return playingPid2;
            } );

    finishForkProcess( playingPid, Process::Forker::Status::Ok, 0, {} );
}

TEST_F( SpeakerTests, playAndStopCommand ) /*1.3*/ {
    pid_t playingPid = 13;
    std::string handlerPath = "HUB:soma_path_for_handler";
    Json userData = { { "id", "some_id" } };

    EXPECT_CALL( m_environment, fork )
        .WillOnce(
            [this, playingPid]( const std::string& shellCommand, Process::Forker::OnProcessCompletion callBack ) {
                addForkProcess( playingPid, callBack );
                return playingPid;
            } );

    EXPECT_CALL( m_environment, sendExecutePluginScript )
        .WillOnce( [&]( const std::string& script, const Json& scriptParams ) {
            ASSERT_EQ( script, handlerPath );
            ASSERT_EQ( scriptParams[ "status" ], "playing" );
            ASSERT_EQ( scriptParams[ "user_data" ], userData );
        } );

    auto commandId = m_speaker->play( "HUB:some_path", -1, 100, "siren", handlerPath, userData );
    ASSERT_FALSE( commandId.empty() ) << "speaker.play should return commandId: string";

    EXPECT_CALL( m_environment, kill( playingPid, SIGTERM ) ).Times( 1 ).WillOnce( [this]( pid_t pid, int signal ) {
        finishForkProcess( pid, Process::Forker::Status::TermSignal, -1, {} );
    } );

    EXPECT_CALL( m_environment, sendExecutePluginScript )
        .WillOnce( [&]( const std::string& script, const Json& scriptParams ) {
            ASSERT_EQ( script, handlerPath );
            ASSERT_EQ( scriptParams[ "status" ], "stopped" );
            ASSERT_EQ( scriptParams[ "user_data" ], userData );
        } );

    ASSERT_TRUE( m_speaker->stop( commandId ) );
}

TEST_F( SpeakerTests, playCommandWhichShouldFail ) /*1.4*/ {
    std::string handlerPath = "HUB:soma_path_for_handler";
    Json userData = { { "id", "some_id" } };

    EXPECT_CALL( m_environment, fork )
        .WillOnce( []( const std::string& shellCommand, Process::Forker::OnProcessCompletion callBack ) -> pid_t {
            throw Process::ForkError( "Cannot run process" );
        } );

    EXPECT_CALL( m_environment, sendExecutePluginScript )
        .WillOnce( [&]( const std::string& script, const Json& scriptParams ) {
            ASSERT_EQ( script, handlerPath );
            ASSERT_EQ( scriptParams[ "status" ], "failed" );
            ASSERT_EQ( scriptParams[ "user_data" ], userData );
        } );

    auto commandId = m_speaker->play( "HUB:some_path", 0, 100, "siren", handlerPath, userData );
    ASSERT_TRUE( commandId.empty() );
}

TEST_F( SpeakerTests, playCommandAndGetPlayingInfo ) /*1.5*/ {
    pid_t playingPid = 13;
    std::string handlerPath = "HUB:soma_path_for_handler";
    Json userData = { { "id", "some_id" } };

    EXPECT_CALL( m_environment, fork )
        .WillOnce(
            [this, playingPid]( const std::string& shellCommand, Process::Forker::OnProcessCompletion callBack ) {
                addForkProcess( playingPid, callBack );
                return playingPid;
            } );

    EXPECT_CALL( m_environment, sendExecutePluginScript )
        .WillOnce( [&]( const std::string& script, const Json& scriptParams ) {
            ASSERT_EQ( script, handlerPath );
            ASSERT_EQ( scriptParams[ "status" ], "playing" );
            ASSERT_EQ( scriptParams[ "user_data" ], userData );
        } );

    // if time is 0 than play sound only once
    auto commandId = m_speaker->play( "HUB:some_path", 0, 100, "siren", handlerPath, userData );
    ASSERT_FALSE( commandId.empty() ) << "speaker.play should return commandId: string";

    Speaker::SpeakerRemoteImpl remoteInterface( *m_service );

    {
        auto startedPlayingInfo = remoteInterface.getPlayingInfo( commandId );
        ASSERT_EQ( startedPlayingInfo.getId(), commandId );
        ASSERT_EQ( startedPlayingInfo.getHandlerScriptPath(), handlerPath );
        ASSERT_EQ( startedPlayingInfo.getUserData(), userData );
        ASSERT_EQ( startedPlayingInfo.getPlayingTime(), 0 );
        ASSERT_EQ( startedPlayingInfo.getVolume(), 100 );
        ASSERT_EQ( startedPlayingInfo.getStatus(), Speaker::PlayingStatus::Playing );
    }

    EXPECT_CALL( m_environment, sendExecutePluginScript )
        .WillOnce( [&]( const std::string& script, const Json& scriptParams ) {
            ASSERT_EQ( script, handlerPath );
            ASSERT_EQ( scriptParams[ "status" ], "finished" );
            ASSERT_EQ( scriptParams[ "user_data" ], userData );
        } );

    finishForkProcess( playingPid, Process::Forker::Status::Ok, 0, {} );

    {
        auto finishedPlayingInfo = remoteInterface.getPlayingInfo( commandId );
        ASSERT_EQ( finishedPlayingInfo.getId(), commandId );
        ASSERT_EQ( finishedPlayingInfo.getHandlerScriptPath(), handlerPath );
        ASSERT_EQ( finishedPlayingInfo.getUserData(), userData );
        ASSERT_EQ( finishedPlayingInfo.getPlayingTime(), 0 );
        ASSERT_EQ( finishedPlayingInfo.getVolume(), 100 );
        ASSERT_EQ( finishedPlayingInfo.getStatus(), Speaker::PlayingStatus::Finished );
    }
}

}  // namespace UT
}  // namespace HardwareManager
}  // namespace HubPlatform

#endif