#pragma once

#include "hardware_manager/speaker/model/playing_info.h"
#include "hardware_manager/speaker/model/playing_process_info.h"
#include "hardware_manager/speaker/model/speaker_errors.h"

#include "json_utils/json_fwd.h"

#include <functional>

class IDbClient;

namespace HubPlatform {
namespace HardwareManager {
namespace Speaker {

class IMessenger;

class PlayingInfoStorage {
public:
    using OnPlayingInfo = std::function< void( const PlayingInfo& processInfo ) >;
    using OnPlayingProcessInfo = std::function< void( const PlayingProcessInfo& processInfo ) >;

public:
    PlayingInfoStorage( IMessenger& luaMessenger, IDbClient& database );

public:
    PlayingInfo getInfo( const std::string& soundId );
    void addInfo( const PlayingInfo& info );
    void removeInfo( const std::string& soundId );
    bool hasInfo( const std::string& soundId );
    void processAllPlayingInfo( OnPlayingInfo callback );

    void setInfoStatus( const std::string& soundId,
                        PlayingStatus status,
                        ErrorInfo::Code errorCode = ErrorInfo::Code::NoError );

    void saveProcessInfo( const PlayingProcessInfo& processInfo );
    PlayingProcessInfo getProcessInfo( const std::string& soundId );
    void removeProcessInfo( const std::string& soundId );
    void processAllStartedSoundsInfo( OnPlayingProcessInfo callback );

private:
    IMessenger& m_luaMessenger;

    IDbClient& m_database;
};

}  // namespace Speaker
}  // namespace HardwareManager
}  // namespace HubPlatform
