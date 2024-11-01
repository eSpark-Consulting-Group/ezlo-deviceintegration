#include "cored/script_events/feature_removed.h"
#include "cored/script_events/resources/script_event_resources.h"

namespace HubPlatform {
namespace CoreD {
namespace ScriptEvents {

FeatureRemoved::FeatureRemoved( const Json& json, std::string name )
: BaseEvent( Name::FeatureRemoved, json )
, m_name( std::move( name ) ) {}

Json FeatureRemoved::toJson() const {
    auto result = BaseEvent::toJson();

    result[ "name" ] = m_name;

    return result;
}

}  // namespace ScriptEvents
}  // namespace CoreD
}  // namespace HubPlatform
