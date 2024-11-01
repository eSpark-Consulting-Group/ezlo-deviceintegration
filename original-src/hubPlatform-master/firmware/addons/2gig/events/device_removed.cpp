#include "2gig/events/device_removed.h"
#include "2gig/resources/resources.h"

namespace HubPlatform {
namespace TwoGig {

DeviceRemoved::DeviceRemoved( const std::vector< uint32_t >& txIds )
: Services::ScriptEvent( Resources::Events::DeviceRemoved )
, m_txIds( txIds ) {}

Json DeviceRemoved::toJson() const {
    auto result = Services::ScriptEvent::toJson();

    result[ Resources::TxIds ] = m_txIds;

    return result;
}

}  // namespace TwoGig
}  // namespace HubPlatform
