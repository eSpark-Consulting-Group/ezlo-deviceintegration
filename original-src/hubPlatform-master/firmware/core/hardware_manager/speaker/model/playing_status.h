#pragma once

namespace HubPlatform {
namespace HardwareManager {
namespace Speaker {

enum class PlayingStatus {
    Begin,

    Pending = Begin,
    Playing,
    Failed,
    Finished,
    Stopped,

    End
};

const char* toString( PlayingStatus status );
PlayingStatus toPlayingStatus( const char* status );

}  // namespace Speaker
}  // namespace HardwareManager
}  // namespace HubPlatform