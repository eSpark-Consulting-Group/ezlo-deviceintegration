#pragma once

#include <stdexcept>

#include "hardware_manager/speaker/model/playing_status.h"

#include "json_utils/json_fwd.h"

#include <string>
#include <cctype>

namespace HubPlatform {
namespace HardwareManager {
namespace Speaker {

class PlayingProcessInfo {
public:
    PlayingProcessInfo( std::string id, int pid, bool m_isSinglePlaying );

    PlayingProcessInfo() = default;

public:
    const std::string& getId() const;
    int getPid() const;
    bool isSinglePlaying() const;

public:
    Json toJson() const;
    bool fromJson( const Json& data );

private:
    std::string m_id;
    int m_pid{ 0 };
    bool m_isSinglePlaying{ false };
};

}  // namespace Speaker
}  // namespace HardwareManager
}  // namespace HubPlatform