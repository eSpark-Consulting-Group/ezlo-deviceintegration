#include "zigbee/script_events/alert_received.h"
#include "zigbee/resources/zigbee_resources.h"
#include "zigbee/model/enums/commands/alert_notification_cotegory_source.h"
#include "zigbee/model/enums/commands/get_alerts_responce_source.h"

namespace HubPlatform {
namespace Zigbee {

AlertReceived::AlertReceived( uint16_t nodeId,
                              uint8_t endpointId,
                              const HubPlatform::Zigbee::AlertStructure& alertStructures,
                              const char* commandName,
                              AlertSource eventNotificationId )
: CommandReceivedBase( Resources::ScriptEvent::Name::AlertReceived,
                       nodeId,
                       endpointId,
                       Resources::Fields::Cluster::Names::ApplianceEventsAndAlerts,
                       static_cast< uint8_t >( eventNotificationId ),
                       commandName )
, m_alertStructures( alertStructures ) {}

Json AlertReceived::toJson() const {
    auto result = CommandReceivedBase::toJson();
    result[ Resources::ScriptEvent::Param::AlertId ] = m_alertStructures.alert_id;
    result[ Resources::ScriptEvent::Param::Category ] =
        AlertNotificationCategory::toString( m_alertStructures.category );
    result[ Resources::ScriptEvent::Param::ManufacturerSpecificBits ] = m_alertStructures.manufacturer_specific_bits;
    result[ Resources::ScriptEvent::Param::PresenceRecovery ] =
        AlertNotificationPresenceRecovery::toString( m_alertStructures.presence_recovery );

    return result;
}

}  // namespace Zigbee
}  // namespace HubPlatform
