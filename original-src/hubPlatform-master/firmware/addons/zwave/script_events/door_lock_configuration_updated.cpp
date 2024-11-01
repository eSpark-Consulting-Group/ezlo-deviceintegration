#include "door_lock_configuration_updated.h"
#include "zwave/resources/zwave_resources.h"

#include <znet/znet_utils.hpp>

namespace HubPlatform {
namespace Zwave {

DoorLockConfigurationUpdated::DoorLockConfigurationUpdated( const uint8_t nodeId,
                                                            const uint8_t channelId,
                                                            const Configuration& configuration,
                                                            const ExtendedConfigurationOpt& extendedConfigurationOpt )
: ValueEvent( Resources::ScriptEvent::Name::DoorLockConfigurationUpdated,
              nodeId,
              ZNET_COMMAND_CLASS_DOOR_LOCK,
              channelId )
, m_configuration( configuration )
, m_extendedConfigurationOpt( extendedConfigurationOpt ) {}

Json DoorLockConfigurationUpdated::toJson() const {
    using namespace Resources::ScriptEvent;

    auto result = ValueEvent::toJson();

    auto& value = result[ Param::Value ] = Json::object();

    if( m_configuration.are_timed_operations_enabled ) {
        value[ Param::LockTimeout ] = m_configuration.lock_timeout;
    }

    auto& jsonInsideHandleIds = value[ Param::EnabledInsideHandleIds ] = Json::array();
    znet::utils::forEachBit( &m_configuration.enabled_inside_handle_ids, 1, [&jsonInsideHandleIds]( uint8_t index ) {
        jsonInsideHandleIds.push_back( index + 1 );
    } );

    auto& jsonOutsideHandleIds = value[ Param::EnabledOutsideHandleIds ] = Json::array();
    znet::utils::forEachBit( &m_configuration.enabled_outside_handle_ids, 1, [&jsonOutsideHandleIds]( uint8_t index ) {
        jsonOutsideHandleIds.push_back( index + 1 );
    } );

    if( m_extendedConfigurationOpt.second ) {
        value[ Param::HoldAndReleaseTimeout ] = m_extendedConfigurationOpt.first.hold_and_release_timeout;
        value[ Param::AutoRelockTimeout ] = m_extendedConfigurationOpt.first.auto_relock_timeout;
        value[ Param::TwistAssistEnabled ] = m_extendedConfigurationOpt.first.twist_assist_enabled;
        value[ Param::BlockToBlockEnabled ] = m_extendedConfigurationOpt.first.block_to_block_enabled;
    }

    return result;
}

}  // namespace Zwave
}  // namespace HubPlatform
