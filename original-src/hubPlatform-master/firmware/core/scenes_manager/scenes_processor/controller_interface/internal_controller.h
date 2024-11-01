#pragma once

#include "scenes_manager/scenes_engine/controller_interface/controller_interface.h"
#include "cored/remote_interface/cored_interface.h"
#include "service_interfaces/service.h"

namespace HubPlatform {
namespace ScenesManager {

class InternalController : public SceneEngine::ControllerInterface,
                           public std::enable_shared_from_this< InternalController > {
public:
    explicit InternalController( Services::Service& hostService );

    void subscribeOnItemChanges( const ItemId& itemId ) override;
    void subscribeOnItemChanges( const ItemId& itemId, SubscriptionCallbackSelector ) override;
    void unsubscribeFromItemChanges( const ItemId& itemId ) override;

    void subscribeOnDevicesChanges( const DeviceId& deviceId ) override;
    void subscribeOnDevicesChanges( const DeviceId& deviceId, SubscriptionCallbackSelector ) override;
    void unsubscribeFromDevicesChanges( const DeviceId& deviceId ) override;

    BoolResult isDeviceArmed( const DeviceId& deviceId ) override;
    Device getDevice( const DeviceId& deviceId ) override;
    Response setDeviceArmed( const DeviceId& deviceId, bool armed ) override;

    void setItemValue( const ItemData& data, SendResponseCallback sendResponseCallback ) override;
    RawItems getItemsBy( const JsonObject& condition ) override;
    RawDevices getDevicesBy( const JsonObject& condition ) override;
    Items getItems( const ItemIds& itemIds ) override;
    Item getItem( const ItemId& itemId ) override;

    Response setHouseMode( const HouseModeId& houseModeId ) override;

private:
    CoreDRemoteInterface::CoreDInterfaceUPtr m_remoteInterface;
    Services::Service& m_hostService;
};

}  // namespace ScenesManager
}  // namespace HubPlatform
