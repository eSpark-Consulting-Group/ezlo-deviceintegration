#include "walk_test_events.h"
#include "cored/house_modes/resources/house_mode_resources.h"
#include "json_utils/json_wrapper.h"

namespace HubPlatform {
namespace CoreDHouseModes {
namespace ScriptEvents {

WalkTestEvent::WalkTestEvent( const std::string& eventName,
                              const std::string& operationId,
                              std::chrono::seconds timestamp,
                              std::chrono::seconds timeLeft )
: ScriptEvents::BaseEvent( eventName )
, m_operationId( std::move( operationId ) )
, m_timestamp( std::move( timestamp ) )
, m_timeLeft( std::move( timeLeft ) ) {}

Json WalkTestEvent::toJson() const {
    auto result = BaseEvent::toJson();
    result[ "type" ] = "walk";
    result[ "operation_id" ] = m_operationId;
    result[ "timestamp" ] = m_timestamp.count();
    result[ "time_left" ] = m_timeLeft.count();
    return result;
}

WalkTestEventStarted::WalkTestEventStarted( const std::string& operationId,
                                            std::chrono::seconds timestamp,
                                            std::chrono::seconds timeLeft )
: WalkTestEvent( Resources::WalkTestEvents::SystemTestStarted, operationId, timestamp, timeLeft ) {}

WalkTestEventExtended::WalkTestEventExtended( const std::string& operationId,
                                              std::chrono::seconds timestamp,
                                              std::chrono::seconds timeLeft )
: WalkTestEvent( Resources::WalkTestEvents::SystemTestExtended, operationId, timestamp, timeLeft ) {}

WalkTestEventFinished::WalkTestEventFinished( const std::string& operationId,
                                              std::chrono::seconds timestamp,
                                              std::chrono::seconds timeLeft )
: WalkTestEvent( Resources::WalkTestEvents::SystemTestFinished, operationId, timestamp, timeLeft ) {}

WalkTestEventTerminationProgress::WalkTestEventTerminationProgress( const std::string& operationId,
                                                                    std::chrono::seconds timestamp,
                                                                    std::chrono::seconds timeLeft )
: WalkTestEvent( Resources::WalkTestEvents::SystemTerminationProgress, operationId, timestamp, timeLeft ) {}

}  // namespace ScriptEvents
}  // namespace CoreDHouseModes
}  // namespace HubPlatform
