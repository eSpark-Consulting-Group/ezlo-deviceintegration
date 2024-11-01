
#include "hardware_manager/speaker/speaker.h"
#include "hardware_manager/speaker/environment/speaker_ienvironment.h"
#include "hardware_manager/hm_service.h"

#include "database_client/dbid.h"

#include "log_utils/ezlo_log.h"
#include "tiny/tiny_consolecolors.h"

#include "system_utils/system.h"

#include <csignal>

namespace HubPlatform {
namespace HardwareManager {
namespace Speaker {
namespace {

constexpr std::chrono::seconds StoppedTaskLifetime{ 180 };
}

Speaker::Speaker( IEnvironment& environment )
: m_environment( environment )
, m_luaMessenger( environment )
, m_storage( m_luaMessenger, environment.getService().getDBClient() ) {}

bool Speaker::setUp() {
    if( !m_cleanupTask ) {
        killAllStartedProcess( ProcessKillPolicy::KillAndUpdateStatus );

        {
            std::unique_lock< std::mutex > scopeLock( m_timestampsMutex );

            auto currentTime = std::chrono::steady_clock().now();
            m_storage.processAllPlayingInfo( [this, &currentTime]( const PlayingInfo& playingInfo ) {
                m_finishedTimestamps[ playingInfo.getId() ] = currentTime;
                if( playingInfo.getStatus() == PlayingStatus::Playing ) {
                    m_storage.setInfoStatus( playingInfo.getId(), PlayingStatus::Stopped );
                }
            } );
        }

        m_cleanupTask = m_environment.getService().createPostponedTask(
            [this]( Services::PostponedTask& task ) { cleanupFinishedPlayingInfo(); } );

        m_cleanupTask->start( StoppedTaskLifetime, StoppedTaskLifetime );
    }

    auto result = true;
#if defined( __HUB_TYPE_H2_SECURE__ )
    // configure amixer to hear the sounds: [configure]
    std::vector< std::string > amixerConfiguration = {
        "amixer cset name='Line Out Playback Switch' on", "amixer cset name='Line Out Playback Volume' 31",
        "amixer cset name='DAC Playback Switch' on",      "amixer cset name='DAC Playback Volume' 59",
        "amixer cset name='Mic1 Capture Switch' on",      "amixer cset name='Mic1 Boost Volume' 7",
        "amixer cset name='ADC Gain Capture Volume' 7"
    };

    for( const auto& option : amixerConfiguration ) {
        auto commandResult = System::getCommandOutput( option.c_str(), true );

        if( commandResult.second != 0 ) {
            LOGGER_ERROR( cc::error( "amixer configuration failed: " ) + cc::note( option ) +
                          cc::error( ", exit code: " ) + cc::num10( commandResult.second ) );
            LOGGER_ERROR( cc::error( "   result is: " ) + cc::note( commandResult.first ) );
            result = false;
        }
        else if( Log::getLogLevel() >= Log::PriorityLevel::DEBUG ) {
            LOGGER_TRACE( cc::trace( "amixer configuration called: " ) + cc::note( option ) );
            LOGGER_TRACE( cc::trace( "   result is: " ) + cc::note( commandResult.first ) );
        }
    }
#endif

    return result;
}

void Speaker::tearDown() {
    if( m_cleanupTask ) {
        m_cleanupTask->stop();
        m_cleanupTask.reset();
    }

    killAllStartedProcess( ProcessKillPolicy::KillOnly );
}

void Speaker::cleanupFinishedPlayingInfo() {
    auto currentTime = std::chrono::steady_clock().now();
    std::vector< std::string > idsToRemove;

    std::unique_lock< std::mutex > scopeLock( m_timestampsMutex );
    for( const auto timestamp : m_finishedTimestamps ) {
        if( std::chrono::duration_cast< std::chrono::seconds >( currentTime - timestamp.second ) >=
            StoppedTaskLifetime ) {
            idsToRemove.push_back( timestamp.first );
        }
    }

    for( const auto& soundId : idsToRemove ) {
        m_finishedTimestamps.erase( soundId );
        m_storage.removeInfo( soundId );
    }

    if( !idsToRemove.empty() ) {
        LOGGER_TRACE( cc::trace( "Removed old playing sounds info: " ) + cc::j( JSON_ARRAY_( idsToRemove ) ) );
    }
}

std::string Speaker::play( std::string songFilePath,
                           std::int64_t time,
                           std::uint8_t volume,
                           std::string plugin,
                           std::string handlerScript,
                           Json userData ) {
    auto requestId = db::DbId::genOID();

    PlayingInfo playingInfo{ requestId,
                             std::move( songFilePath ),
                             time,
                             volume,
                             std::move( plugin ),
                             std::move( handlerScript ),
                             std::move( userData ) };

    m_storage.addInfo( playingInfo );
    if( playSound( playingInfo ) ) {
        return requestId;
    }

    return {};
}

bool Speaker::stop( const std::string& id ) {
    const auto processInfo = m_storage.getProcessInfo( id );

    if( processInfo.getPid() == 0 ) {
        LOGGER_WARN( cc::warn( "Speaker: Attempt to kill not started command: '" ) + cc::note( id ) );
        return false;
    }

    m_environment.kill( processInfo.getPid(), SIGTERM );
    return true;
}

bool Speaker::playSound( const PlayingInfo& sound, std::chrono::steady_clock::time_point startedTime ) {
    try {
        std::string command{ "aplay -t wav " };
        if( sound.getPlayingTime() > 0 ) {
            auto soundDuration = sound.getPlayingTime();
            if( startedTime != std::chrono::steady_clock::time_point() ) {
                const auto endTime = std::chrono::steady_clock().now();
                const auto executingTime = std::chrono::duration_cast< std::chrono::seconds >( endTime - startedTime );
                if( executingTime.count() < sound.getPlayingTime() ) {
                    soundDuration = sound.getPlayingTime() - executingTime.count();
                }
                else {
                    onPlayingFinished( sound.getId(), PlayingStatus::Finished );
                    return true;
                }
            }

            command += " --duration=" + std::to_string( soundDuration );
        }

        command += " " + sound.getSongFilePath();

        LOGGER_TRACE( cc::trace( "Speaker: Play sound: '" ) + cc::note( command ) + cc::p( sound.getSongFilePath() ) );

        const auto soundId = sound.getId();
        if( startedTime == std::chrono::steady_clock::time_point() ) {
            startedTime = std::chrono::steady_clock().now();
            LOGGER_TRACE( cc::trace( "Speaker: started time!" ) );
        }
        auto processPid = m_environment.fork(
            command,
            [this, soundId, startedTime](
                pid_t pid, Process::Forker::Status status, int exitCode, const std::string& error ) {
                processPlayingSoundResult( pid, soundId, startedTime, status, exitCode, error );
            } );

        if( processPid > 0 ) {
            m_storage.saveProcessInfo( { soundId, processPid, ( sound.getPlayingTime() == 0 ) } );
            m_storage.setInfoStatus( sound.getId(), PlayingStatus::Playing );
            return true;
        }

        onPlayingFinished( sound.getId(), PlayingStatus::Failed, ErrorInfo::Code::CannotStartPlaying );
    }
    catch( const std::exception& error ) {
        LOGGER_ERROR( cc::error( "Cannot start playing sound: " ) + cc::note( error.what() ) );
        onPlayingFinished( sound.getId(), PlayingStatus::Failed, ErrorInfo::Code::CannotStartPlaying );
    }

    return false;
}

void Speaker::processPlayingSoundResult( pid_t pid,
                                         const std::string& soundId,
                                         std::chrono::steady_clock::time_point startedTime,
                                         Process::Forker::Status status,
                                         int exitCode,
                                         const std::string& error ) {
    LOGGER_TRACE( cc::trace( "Speaker: Playing finished PID: " ) + cc::num10( pid ) + cc::trace( ", Status: " ) +
                  cc::num10( static_cast< int >( status ) ) + cc::trace( ", Exit code: " ) + cc::num10( exitCode ) +
                  cc::trace( ", Error: '" ) + cc::note( error ) + cc::trace( "'" ) );

    auto processInfo = m_storage.getProcessInfo( soundId );
    m_storage.removeProcessInfo( soundId );

    if( !m_storage.hasInfo( soundId ) ) {
        LOGGER_ERROR(
            cc::error( "Speaker: playing finished but corresponding sound information doesn't exist in DB: " ) +
            cc::note( soundId ) + cc::error( ", PID: " ) + cc::num10( pid ) + cc::error( ", Status: " ) +
            cc::num10( static_cast< int >( status ) ) + cc::error( ", Exit code: " ) + cc::num10( exitCode ) +
            cc::error( ", Error: '" ) + cc::note( error ) + cc::error( "'" ) );
        return;
    }

    if( processInfo.getId().empty() ) {
        LOGGER_WARN(
            cc::warn( "Speaker: playing finished but corresponding process information doesn't exist in DB: " ) +
            cc::note( soundId ) + cc::warn( ", PID: " ) + cc::num10( pid ) + cc::warn( ", Status: " ) +
            cc::num10( static_cast< int >( status ) ) + cc::warn( ", Exit code: " ) + cc::num10( exitCode ) +
            cc::warn( ", Error: '" ) + cc::note( error ) + cc::error( "'" ) );
    }

    switch( status ) {
    case Process::Forker::Status::Ok: {
        // single play
        if( processInfo.isSinglePlaying() ) {
            onPlayingFinished( soundId, PlayingStatus::Finished );
        }

        // infinity or time limited playing
        else {
            auto playingInfo = m_storage.getInfo( soundId );
            if( playingInfo.getId().empty() ) {
                LOGGER_WARN( cc::warn( "Speaker: cannot continue playing because playing information is missed: " ) +
                             cc::note( soundId ) );
            }
            else if( playingInfo.getStatus() == PlayingStatus::Playing ) {
                playSound( playingInfo, startedTime );
            }
            else {
                LOGGER_TRACE( cc::trace( "Speaker: finished already stopped playing: " ) +
                              cc::j( playingInfo.toJson() ) );
            }
        }
    } break;

    case Process::Forker::Status::TermSignal:
    case Process::Forker::Status::UnknownError:
        onPlayingFinished( soundId, PlayingStatus::Stopped );
        LOGGER_TRACE( cc::trace( "Speaker: playing process was stopped: " ) + cc::j( processInfo.toJson() ) );
        break;

    case Process::Forker::Status::ForkFailure:
        onPlayingFinished( soundId, PlayingStatus::Failed, ErrorInfo::Code::PlayingFinishedInNotStandardWay );
        LOGGER_WARN( cc::warn( "Speaker: fork of playing process finished with error: " ) +
                     cc::j( processInfo.toJson() ) );
        break;
    }
}

void Speaker::onPlayingFinished( const std::string& soundId, PlayingStatus status, ErrorInfo::Code errorCode ) {
    m_storage.setInfoStatus( soundId, status, errorCode );

    {
        std::unique_lock< std::mutex > scopeLock( m_timestampsMutex );
        m_finishedTimestamps[ soundId ] = std::chrono::steady_clock().now();
    }
}

void Speaker::killAllStartedProcess( ProcessKillPolicy policy ) {
    m_storage.processAllStartedSoundsInfo( [this, policy]( const PlayingProcessInfo& processInfo ) {
        LOGGER_INFO( cc::info( "Killing playing process: " ) + cc::j( processInfo.toJson() ) );
        m_environment.kill( processInfo.getPid(), SIGTERM );

        if( policy == ProcessKillPolicy::KillAndUpdateStatus ) {
            m_storage.setInfoStatus( processInfo.getId(), PlayingStatus::Stopped );
        }

        m_storage.removeProcessInfo( processInfo.getId() );
    } );
}

}  // namespace Speaker
}  // namespace HardwareManager
}  // namespace HubPlatform
