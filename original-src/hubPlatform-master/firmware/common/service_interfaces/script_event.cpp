#include "service_interfaces/script_event.h"

#include "json_utils/json_wrapper.h"

namespace HubPlatform {
namespace Services {

ScriptEvent::ScriptEvent( std::string eventName, const std::string& moduleName )
: m_eventName( std::move( eventName ) )
, m_moduleName( moduleName ) {}

ScriptEvent::~ScriptEvent() = default;

Json ScriptEvent::toJson() const {
    return JSON( "event" << m_eventName );
}

const std::string& ScriptEvent::getName() const {
    return m_eventName;
}

const std::string& ScriptEvent::getModuleName() const {
    return m_moduleName;
}

bool ScriptEvent::operator==( const ScriptEvent& rhs ) const {
    return toJson() == rhs.toJson();
}

bool ScriptEvent::operator!=( const ScriptEvent& rhs ) const {
    return !this->operator==( rhs );
}

}  // namespace Services
}  // namespace HubPlatform
