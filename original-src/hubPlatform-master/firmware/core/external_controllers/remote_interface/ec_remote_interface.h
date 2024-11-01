#pragma once

#include "external_controllers/remote_interface/ec_abstract_interface.h"

#include "json_utils/json_fwd.h"
#include "jsonrpc/message.h"

#include <vector>
#include <string>

namespace HubPlatform {

namespace Services {
class Service;
}

namespace ExternalControllers {

class RemoteInterface final : public AbstractInterface {
public:
    explicit RemoteInterface( Services::Service& hostService );

    // Subscription
    void subscribeOnItemChanges( const ControllerID& controllerId, const ItemsIDs& itemsIds ) override;
    void unsubscribeFromItemChanges( const ControllerID& controllerId, const ItemsIDs& itemsIds ) override;

    void subscribeOnDevicesChanges( const ControllerID& controllerId, const DevicesIDs& devicesIds ) override;
    void unsubscribeFromDevicesChanges( const ControllerID& controllerId, const DevicesIDs& devicesIds ) override;

    // Items
    JsonObject getItem( const ControllerID& controllerId, const ItemID& itemId ) const override;
    RawItems getItems( const ControllerID& controllerId, const ItemsIDs& itemsIds ) const override;
    /* data = {
     *          "_id": "5c4df91d28afdb52a6c19534",
     *          "value" : <item_value>
     * } */
    void setItemValue( const ControllerID& controllerId,
                       const JsonObject& data,
                       SendResponseCallback sendResponseCallback = []( const JsonObject& ) {} ) override;

    RawItems getItemsBy( const ControllerID& controllerId, const JsonObject& condition ) const override;
    RawDevices getDevicesBy( const ControllerID& controllerId, const JsonObject& condition ) const override;
    // Devices
    JsonObject getSingleDevice( const ControllerID& controllerId, const DeviceID& deviceId ) const override;
    JsonObject setDeviceArmed( const ControllerID& controllerId, const DeviceID& deviceId, bool armed ) override;

    // NOTE: await following broadcasts from external controllers in "external_device_watchers_group":
    //  - hub.item.updated
    //  - hub.item.dictionary.updated
    //  - hub.item.removed
    // NOTE: these broadcasts will be similar to standard ones but will contain additional field "controllerId"

private:
    Services::Service& m_hostService;
};

}  // namespace ExternalControllers
}  // namespace HubPlatform
