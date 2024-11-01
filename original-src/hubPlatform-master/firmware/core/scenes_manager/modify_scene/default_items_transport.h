#pragma once
#include "scenes_manager/modify_scene/items_transport.h"
#include "cored/remote_interface/cored_interface.h"

namespace HubPlatform {
namespace ScenesManager {

class DefaultItemsTransport : public ItemsTransport {
public:
    using RemoteInterface = CoreDRemoteInterface::CoreDInterface;
    using RawItems = std::vector< Json >;

    explicit DefaultItemsTransport( RemoteInterface& remote );
    RawItems getItemsBy( const JsonObject& condition ) override;

private:
    RemoteInterface& m_core;
};

}  // namespace ScenesManager
}  // namespace HubPlatform