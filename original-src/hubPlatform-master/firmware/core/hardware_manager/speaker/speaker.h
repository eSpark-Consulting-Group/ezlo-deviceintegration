#pragma once

#include "hardware_manager/speaker/playing_info_storage.h"

#include "service_interfaces/service.h"
#include "system_utils/process/forker.h"
#include "system_utils/thread_unique_safe_wrapper.h"
#include "hardware_manager/speaker/lua_messenger.h"

#include <map>
#include <stdexcept>

namespace HubPlatform {
namespace HardwareManager {
namespace Speaker {

class IEnvironment;

class Speaker {
    enum class ProcessKillPolicy { KillOnly, KillAndUpdateStatus };

public:
    Speaker( IEnvironment& environment );

public:
    std::string play( std::string songFilePath,
                      std::int64_t time,
                      std::uint8_t volume,
                      std::string plugin,
                      std::string handlerScript,
                      Json userData );

    bool stop( const std::string& id );

    bool setUp();

    void tearDown();

private:
    bool playSound( const PlayingInfo& sound, std::chrono::steady_clock::time_point startedTime = {} );

    void processPlayingSoundResult( pid_t,
                                    const std::string& soundId,
                                    std::chrono::steady_clock::time_point startedTime,
                                    Process::Forker::Status status,
                                    int exitCode,
                                    const std::string& error );

    void killAllStartedProcess( ProcessKillPolicy policy );

    void onPlayingFinished( const std::string& soundId,
                            PlayingStatus status,
                            ErrorInfo::Code errorCode = ErrorInfo::Code::NoError );

    void cleanupFinishedPlayingInfo();

private:
    IEnvironment& m_environment;

    LuaMessenger m_luaMessenger;
    PlayingInfoStorage m_storage;

    Services::PostponedTaskPtr m_cleanupTask;

    using FinishedTimeCollection = std::map< std::string, std::chrono::steady_clock::time_point >;
    FinishedTimeCollection m_finishedTimestamps;
    std::mutex m_timestampsMutex;
};

}  // namespace Speaker
}  // namespace HardwareManager
}  // namespace HubPlatform
