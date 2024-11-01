#include "scenes_manager/modify_scene/default_items_transport.h"

namespace HubPlatform {
namespace ScenesManager {

DefaultItemsTransport::DefaultItemsTransport( DefaultItemsTransport::RemoteInterface& remote )
: m_core( remote ) {}

DefaultItemsTransport::RawItems DefaultItemsTransport::getItemsBy( const JsonObject& condition ) {
    return m_core.getItemsBy( condition );
}

}  // namespace ScenesManager
}  // namespace HubPlatform
