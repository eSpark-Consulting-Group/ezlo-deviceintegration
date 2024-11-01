
#include "hardware_manager/speaker/model/playing_process_info.h"

#include "json_utils/json_utils.h"
#include "hardware_manager/speaker/model/resources.h"

namespace HubPlatform {
namespace HardwareManager {
namespace Speaker {

PlayingProcessInfo::PlayingProcessInfo( std::string id, int pid, bool isSinglePlaying )
: m_id( std::move( id ) )
, m_pid( pid )
, m_isSinglePlaying( isSinglePlaying ) {}

const std::string& PlayingProcessInfo::getId() const {
    return m_id;
}

int PlayingProcessInfo::getPid() const {
    return m_pid;
}

bool PlayingProcessInfo::isSinglePlaying() const {
    return m_isSinglePlaying;
}

bool PlayingProcessInfo::fromJson( const Json& data ) {
    return json_get( data, Fields::id, m_id ) && json_get( data, Fields::pid, m_pid ) &&
           json_get( data, Fields::singlePlaying, m_isSinglePlaying );
}

Json PlayingProcessInfo::toJson() const {
    return { { Fields::id, m_id }, { Fields::pid, m_pid }, { Fields::singlePlaying, m_isSinglePlaying } };
}

}  // namespace Speaker
}  // namespace HardwareManager
}  // namespace HubPlatform