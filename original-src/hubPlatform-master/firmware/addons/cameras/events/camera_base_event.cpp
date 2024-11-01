#include "cameras/events/camera_base_event.h"

namespace HubPlatform {

CameraBaseEvent::CameraBaseEvent( const std::string& eventName, Json&& json )
: Services::ScriptEvent( eventName )
, m_json( std::move( json ) ) {}

Json CameraBaseEvent::toJson() const {
    JsonObject result = ScriptEvent::toJson();
    for( auto j = m_json.begin(); j != m_json.end(); ++j ) {
        result[ j.key() ] = j.value();
    }
    return result;
}

}  // namespace HubPlatform
