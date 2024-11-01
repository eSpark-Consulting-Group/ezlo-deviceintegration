#include "zwave/script_events/learn_show_key.h"
#include "zwave/resources/zwave_resources.h"

namespace HubPlatform {
namespace Zwave {

LearnShowKey::LearnShowKey( const std::vector< int >& key )
: Services::ScriptEvent( Resources::ScriptEvent::Name::LearnShowKey )
, m_key( key ) {}

Json LearnShowKey::toJson() const {
    using namespace Resources::ScriptEvent;

    auto result = Services::ScriptEvent::toJson();
    result[ Param::Key ] = m_key;
    return result;
}

}  // namespace Zwave
}  // namespace HubPlatform
