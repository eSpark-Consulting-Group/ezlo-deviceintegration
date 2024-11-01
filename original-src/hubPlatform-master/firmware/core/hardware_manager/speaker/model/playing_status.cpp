
#include "hardware_manager/speaker/model/playing_status.h"
#include "hardware_manager/speaker/model/resources.h"

#include <cstring>

namespace HubPlatform {
namespace HardwareManager {
namespace Speaker {

const char* toString( PlayingStatus status ) {
    switch( status ) {
    case PlayingStatus::Pending:
        return Status::pending;

    case PlayingStatus::Playing:
        return Status::playing;

    case PlayingStatus::Failed:
        return Status::failed;

    case PlayingStatus::Finished:
        return Status::finished;

    case PlayingStatus::Stopped:
        return Status::stopped;

    default:
        break;
    }

    return Status::pending;
}

PlayingStatus toPlayingStatus( const char* status ) {
    for( int idx = static_cast< int >( PlayingStatus::Begin ); idx < static_cast< int >( PlayingStatus::End ); ++idx ) {
        if( strcmp( toString( static_cast< PlayingStatus >( idx ) ), status ) == 0 ) {
            return static_cast< PlayingStatus >( idx );
        }
    }

    return PlayingStatus::Pending;
}

}  // namespace Speaker
}  // namespace HardwareManager
}  // namespace HubPlatform