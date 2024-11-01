#include "zigbee/script_events/empty_alerts_list_response_received.h"
#include "zigbee/resources/zigbee_resources.h"
#include "zigbee/model/enums/commands/alert_notification_cotegory_source.h"
#include "zigbee/model/enums/commands/get_alerts_responce_source.h"

namespace HubPlatform {
namespace Zigbee {

EmptyAlertsListResponseReceived::EmptyAlertsListResponseReceived( uint16_t nodeId, uint8_t endpointId )
: CommandReceivedBase( Resources::ScriptEvent::Name::EmptyAlertsListReceived,
                       nodeId,
                       endpointId,
                       Resources::Fields::Cluster::Names::ApplianceEventsAndAlerts,
                       static_cast< uint8_t >( AlertSource::Response ),
                       Resources::Fields::Command::Names::ApplianceEventsAndAlerts::GetAlertsResponse ) {}
}  // namespace Zigbee
}  // namespace HubPlatform
