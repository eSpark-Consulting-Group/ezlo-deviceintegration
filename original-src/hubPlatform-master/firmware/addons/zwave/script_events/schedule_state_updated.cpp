#include "zwave/script_events/schedule_state_updated.h"
#include "zwave/resources/zwave_resources.h"
#include <znet/converters/schedule_converter.hpp>

namespace HubPlatform {
namespace Zwave {

ScheduleStateUpdated::ScheduleStateUpdated( znet_node_id_t nodeId,
                                            znet_node_channel_id_t channelId,
                                            const znet_cmd_schedule_state_report_t& value,
                                            const znet::ScheduleIdBlockOpt& idBlockOpt )
: ValueEvent( Resources::ScriptEvent::Name::ScheduleStateUpdated, nodeId, ZNET_COMMAND_CLASS_SCHEDULE, channelId )
, m_idBlockOpt( idBlockOpt )
, m_numberSupportedScheduleIds( value.number_supported_schedule_ids )
, m_reportsToFollow( value.reports_to_follow )
, m_overrideSupport( value.override_support ) {
    for( uint8_t i = 0; i < m_numberSupportedScheduleIds; ++i ) {
        m_activityStatuses.push_back( znet::schedule_active_status_converter::activeStatusToString(
            static_cast< znet::ScheduleActiveStatus >( value.active_id[ i ] ) ) );
    }
}

Json ScheduleStateUpdated::toJson() const {
    auto result = ValueEvent::toJson();

    using namespace Resources::ScriptEvent::Param::Schedule;

    if( m_idBlockOpt ) {
        result[ ScheduleIdBlock ] = *m_idBlockOpt;
    }

    auto& fieldValue = result[ Resources::ScriptEvent::Param::Value ] = Json::object();

    fieldValue[ NumberSupportedScheduleIds ] = m_numberSupportedScheduleIds;
    fieldValue[ ReportsToFollow ] = m_reportsToFollow;
    fieldValue[ OverrideSupport ] = m_overrideSupport;

    auto& activityStatuses = fieldValue[ ScheduleStasuses ] = Json::array();
    for( const auto& activeStatus : m_activityStatuses ) {
        activityStatuses.push_back( activeStatus );
    }

    return result;
}

}  // namespace Zwave
}  // namespace HubPlatform
