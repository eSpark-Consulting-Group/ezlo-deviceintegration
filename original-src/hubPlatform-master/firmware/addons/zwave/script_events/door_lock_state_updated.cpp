#include "zwave/script_events/door_lock_state_updated.h"

#include "zwave/resources/zwave_resources.h"

#include <znet/znet_defs.h>
#include <znet/znet_utils.hpp>
#include <znet/converters/door_lock.hpp>

namespace HubPlatform {
namespace Zwave {

DoorLockStateUpdated::DoorLockStateUpdated( const uint8_t nodeId,
                                            const uint8_t channelId,
                                            const znet::DoorLock::Mode mode,
                                            const uint8_t insideHandlesMode,
                                            const uint8_t outsideHandlesMode,
                                            const znet::DoorCondition condition,
                                            const SecondsOpt lockTimeout,
                                            const znet::DoorLock::ModeOpt targetMode,
                                            const SecondsOpt targetModeTimeout )
: ValueEvent( Resources::ScriptEvent::Name::DoorLockStateUpdated, nodeId, ZNET_COMMAND_CLASS_DOOR_LOCK, channelId )
, m_mode( mode )
, m_insideHandlesMode( insideHandlesMode )
, m_outsideHandlesMode( outsideHandlesMode )
, m_condition( condition )
, m_lockTimeout( lockTimeout )
, m_targetMode( targetMode )
, m_targetModeTimeout( targetModeTimeout ) {}

Json DoorLockStateUpdated::toJson() const {
    using namespace Resources::ScriptEvent;

    auto result = ValueEvent::toJson();
    addOptionalStringField( result, Param::DoorLockMode, znet::door_lock_converter::modeToString, m_mode );
    auto& jsonDoorCondition = result[ Param::DoorCondition ] = Json::object();
    addOptionalStringField(
        jsonDoorCondition, Param::LatchState, znet::door_lock_converter::latchStateToString, m_condition.latchState );
    addOptionalStringField(
        jsonDoorCondition, Param::BoltState, znet::door_lock_converter::boltStateToString, m_condition.boltState );
    addOptionalStringField(
        jsonDoorCondition, Param::DoorState, znet::door_lock_converter::doorStateToString, m_condition.doorState );

    if( m_lockTimeout ) {
        result[ Param::LockTimeout ] = m_lockTimeout->count();
    }

    if( m_targetMode ) {
        result[ Param::TargetDoorLockMode ] = znet::door_lock_converter::modeToString( *m_targetMode );
    }

    if( m_targetModeTimeout ) {
        result[ Param::TargetDoorLockModeTimeout ] = m_targetModeTimeout->count();
    }

    auto& jsonInsideHandleIds = result[ Param::EnabledInsideHandleIds ] = Json::array();
    znet::utils::forEachBit( &m_insideHandlesMode, 1, [&jsonInsideHandleIds]( uint8_t index ) {
        jsonInsideHandleIds.push_back( index + 1 );
    } );

    auto& jsonOutsideHandleIds = result[ Param::EnabledOutsideHandleIds ] = Json::array();
    znet::utils::forEachBit( &m_outsideHandlesMode, 1, [&jsonOutsideHandleIds]( uint8_t index ) {
        jsonOutsideHandleIds.push_back( index + 1 );
    } );

    return result;
}

}  // namespace Zwave
}  // namespace HubPlatform
