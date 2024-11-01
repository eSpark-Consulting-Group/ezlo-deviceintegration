
#include "hardware_manager/speaker/remote/speaker_remote_interface.h"
#include "hardware_manager/speaker/model/resources.h"

#include "database_client/dbqueryparser.h"
#include "log_utils/ezlo_log.h"
#include "tiny/tiny_consolecolors.h"
#include "json_utils/json_utils.h"

namespace HubPlatform {
namespace HardwareManager {
namespace Speaker {

SpeakerRemoteImpl::SpeakerRemoteImpl( Services::Service& hostService )
: m_hostService( hostService ) {}

namespace {
std::string prefix( const std::string& value ) {
    return "SpeakerRemote::" + value + ": ";
}
}  // namespace

SpeakerRemote::SoundId SpeakerRemoteImpl::play( std::string songFilePath,
                                                std::int64_t time,
                                                std::uint8_t volume,
                                                std::string plugin,
                                                std::string handlerScript,
                                                Json userData ) {
    auto requestJson =
        JSON( Speaker::Fields::path << songFilePath << Speaker::Fields::time << time << Speaker::Fields::volume
                                    << volume << Speaker::Fields::plugin << plugin << Speaker::Fields::handler
                                    << handlerScript << Speaker::Fields::userData << userData );

    auto answer =
        m_hostService.sendRequest( GroupsNames::HARDWARE_MANAGER, requestJson, MessageSubClasses::SPEAKER_PLAY );
    LOGGER_TRACE_RAW( prefix( "play" ) + cc::j( requestJson ) + ": result obtained: " + cc::j( answer ) );

    Json result;
    if( answer.is_object() && json_get( answer, "result", result ) ) {
        return json_get_str( result, Fields::id );
    }

    return "";
}

bool SpeakerRemoteImpl::stop( const SoundId& commandId ) {
    auto answer = m_hostService.sendRequest(
        GroupsNames::HARDWARE_MANAGER, JSON( Fields::id << commandId ), MessageSubClasses::SPEAKER_STOP );
    LOGGER_TRACE( cc::trace( prefix( "stop" ) ) + cc::note( commandId ) + cc::trace( " answer: " ) + cc::j( answer ) );

    Json stoppedFlag;
    std::vector< Json > keyPath{ "result", "stopped" };
    auto saveResult = [&stoppedFlag]( const Json& result ) { stoppedFlag = result; };
    if( answer.is_object() && json_get( answer, keyPath, std::move( saveResult ) ) ) {
        if( stoppedFlag.is_boolean() )
            return stoppedFlag;
    }

    return false;
}

PlayingInfo SpeakerRemoteImpl::getPlayingInfo( const SoundId& commandId ) {
    const auto playingInfoJson =
        m_hostService.getDBClient().findOne( TempPlayingInfoDBName, JSON( Fields::id << commandId ), nullptr );

    if( !playingInfoJson.empty() ) {
        PlayingInfo info;
        if( info.fromJson( playingInfoJson ) ) {
            return info;
        }
    }

    return {};
}

}  // namespace Speaker
}  // namespace HardwareManager
}  // namespace HubPlatform
