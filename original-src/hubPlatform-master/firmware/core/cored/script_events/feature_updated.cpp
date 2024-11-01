#include "cored/script_events/feature_updated.h"
#include "cored/script_events/resources/script_event_resources.h"

namespace HubPlatform {
namespace CoreD {
namespace ScriptEvents {

FeatureUpdated::FeatureUpdated( const Json& json, std::string name, std::string status )
: BaseEvent( Name::FeatureUpdated, json )
, m_name( std::move( name ) )
, m_status( std::move( status ) ) {}

Json FeatureUpdated::toJson() const {
    auto result = BaseEvent::toJson();

    result[ "name" ] = m_name;
    result[ "status" ] = m_status;

    return result;
}

}  // namespace ScriptEvents
}  // namespace CoreD
}  // namespace HubPlatform
