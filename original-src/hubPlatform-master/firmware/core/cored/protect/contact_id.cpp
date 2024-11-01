#include "contact_id.h"
#include "resources.h"
#include "json_utils/json_wrapper.h"
#include "json_utils/json_utils.h"

namespace HubPlatform {
namespace CoreD {
namespace Protect {
using namespace Resources;
Json ContactIdInfo::toJson() const {
    JsonObject result = JSON_EMPTY_OBJ;
    result[ "_id" ] = m_id;
    result[ CMSSignalAlias::Text ] = m_text;
    result[ CMSSignalAlias::Off ] = m_off;
    result[ CMSSignalAlias::On ] = m_on;
    result[ CMSSignalAlias::Persistent ] = m_persistent;
    return result;
}

bool ContactIdInfo::fromJson( const JsonObject& data ) {
    m_id = json_get_str( data, CMSSignalAlias::Code );
    m_on = json_get_str( data, CMSSignalAlias::On );
    m_off = json_get_str( data, CMSSignalAlias::Off );
    m_text = json_get_str( data, CMSSignalAlias::Text );
    m_persistent = json_get_bool( data, CMSSignalAlias::Persistent, false );

    return !m_id.empty() && !m_on.empty() && !m_text.empty();
}
}  // namespace Protect
}  // namespace CoreD
}  // namespace HubPlatform