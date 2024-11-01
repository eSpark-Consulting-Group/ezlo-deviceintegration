#pragma once

#include <stdexcept>

#include "hardware_manager/speaker/model/playing_status.h"

#include "json_utils/json_fwd.h"

#include <string>
#include <cctype>

namespace HubPlatform {
namespace HardwareManager {
namespace Speaker {

class PlayingInfo {
public:
    PlayingInfo( std::string id,
                 std::string songFilePath,
                 std::int64_t time,
                 std::uint8_t volume,

                 std::string plugin,
                 std::string handlerScript,
                 Json userData );

    PlayingInfo() = default;

public:
    const std::string& getId() const;
    const std::string& getSongFilePath() const;
    std::int64_t getPlayingTime() const;
    std::uint8_t getVolume() const;

    PlayingStatus getStatus() const;

    const std::string& getPluginName() const;
    const std::string& getHandlerScriptPath() const;
    const Json& getUserData() const;

public:
    void setPlayingTime( std::int64_t playingTime );

public:
    Json toJson() const;
    bool fromJson( const Json& data );

private:
    std::string m_id;
    std::string m_songFilePath;
    std::int64_t m_time{ 0 };
    std::uint8_t m_volume{ 0 };

    std::string m_plugin;
    std::string m_handlerScript;
    Json m_userData;

    PlayingStatus m_status{ PlayingStatus::Pending };
};

}  // namespace Speaker
}  // namespace HardwareManager
}  // namespace HubPlatform