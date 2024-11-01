#include "cored/script_events/feature_added.h"

#include <utility>
#include "cored/script_events/resources/script_event_resources.h"

namespace HubPlatform {
namespace CoreD {
namespace ScriptEvents {

FeatureAdded::FeatureAdded( const Json& json, std::string name, std::string status )
: BaseEvent( Name::FeatureAdded, json )
, m_name( std::move( name ) )
, m_status( std::move( status ) ) {}

Json FeatureAdded::toJson() const {
    auto result = BaseEvent::toJson();

    result[ "name" ] = m_name;
    result[ "status" ] = m_status;

    return result;
}

}  // namespace ScriptEvents
}  // namespace CoreD
}  // namespace HubPlatform
