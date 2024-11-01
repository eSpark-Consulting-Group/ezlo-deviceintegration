#include "cameras/events/stream_error_event_base.h"

namespace HubPlatform {

StreamErrorEventBase::StreamErrorEventBase( std::string eventName, std::string errorDescription )
: Services::ScriptEvent( std::move( eventName ) )
, m_errorDescription( std::move( errorDescription ) ) {}

Json StreamErrorEventBase::toJson() const {
    auto result = Services::ScriptEvent::toJson();
    result[ "errorDescription" ] = m_errorDescription;
    return result;
}

}  // namespace HubPlatform
