#include "zwave/script_events/anti_theft_state_updated.h"
#include "zwave/resources/zwave_resources.h"

namespace HubPlatform {
namespace Zwave {

AntiTheftStateUpdated::AntiTheftStateUpdated( uint8_t nodeId,
                                              uint8_t channelId,
                                              uint8_t classId,
                                              const znet::AntiTheftInfoOpt& infoOpt )
: ValueEvent( Resources::ScriptEvent::Name::AntiTheftStateUpdated, nodeId, classId, channelId )
, m_infoOpt( infoOpt ) {}

Json AntiTheftStateUpdated::toJson() const {
    auto result = ValueEvent::toJson();

    if( m_infoOpt ) {
        auto& lockInfo = result[ Resources::ScriptEvent::Param::LockInfo ] = Json::object();

        lockInfo[ Resources::ScriptEvent::Param::IsRestricted ] = m_infoOpt->m_isRestricted;
        lockInfo[ Resources::ScriptEvent::Param::Hint ] = m_infoOpt->getHexHint();
        lockInfo[ Resources::ScriptEvent::Param::ManufacturerId ] = m_infoOpt->m_manufacturerId;
        if( m_infoOpt->m_zwaveAllianceLockingId ) {
            lockInfo[ Resources::ScriptEvent::Param::ZWaveAllianceLockingId ] = *m_infoOpt->m_zwaveAllianceLockingId;
        }
    }

    return result;
}

}  // namespace Zwave

}  // namespace HubPlatform
