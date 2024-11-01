#include "cameras/rtsp_server/live555/event_trigger.h"
#include "cameras/rtsp_server/rtsp_server_exception.h"

namespace HubPlatform {
namespace Cameras {
namespace Live555 {

EventTrigger::EventTrigger()
: m_env( nullptr )
, m_triggerId( 0 ) {}

EventTrigger::EventTrigger( const UsageEnvironment* env, TaskFunc taskFunc )
: m_env( env )
, m_triggerId( env->taskScheduler().createEventTrigger( taskFunc ) ) {
    if( m_triggerId == 0 ) {
        throw RtspServerException( "No event trigger can be created" );
    }
}

EventTrigger::EventTrigger( EventTrigger&& other )
: m_env( other.m_env )
, m_triggerId( other.m_triggerId ) {
    m_triggerId = 0;
}

EventTrigger::~EventTrigger() {
    release();
}

EventTrigger& EventTrigger::operator=( EventTrigger&& other ) {
    m_env = other.m_env;
    m_triggerId = other.m_triggerId;
    other.m_triggerId = 0;
    return *this;
}

const EventTriggerId& EventTrigger::getId() const {
    return m_triggerId;
}

void EventTrigger::release() {
    if( m_triggerId ) {
        m_env->taskScheduler().deleteEventTrigger( m_triggerId );
    }
    m_triggerId = 0;
}

}  // namespace Live555
}  // namespace Cameras
}  // namespace HubPlatform
