#pragma once

#include "json_utils/json_fwd.h"
#include "jsonrpc/message.h"

#include <vector>
#include <string>

namespace HubPlatform {
namespace ExternalControllers {

class AbstractInterface {
public:
    using ControllerID = std::string;
    using DeviceID = std::string;
    using ItemID = std::string;
    using ItemsIDs = std::vector< ItemID >;
    using DevicesIDs = std::vector< DeviceID >;
    using RawItems = std::vector< JsonObject >;
    using RawDevices = std::vector< JsonObject >;
    using SendResponseCallback = std::function< void( const JsonObject& response ) >;

public:
    // Subscription
    virtual void subscribeOnItemChanges( const ControllerID& controllerId, const ItemsIDs& itemsIds ) = 0;
    virtual void unsubscribeFromItemChanges( const ControllerID& controllerId, const ItemsIDs& itemsIds ) = 0;

    virtual void subscribeOnDevicesChanges( const ControllerID& controllerId, const DevicesIDs& devicesIds ) = 0;
    virtual void unsubscribeFromDevicesChanges( const ControllerID& controllerId, const DevicesIDs& devicesIds ) = 0;

    // Items
    virtual JsonObject getItem( const ControllerID& controllerId, const ItemID& itemId ) const = 0;
    virtual RawItems getItems( const ControllerID& controllerId, const ItemsIDs& itemsIds ) const = 0;
    virtual void setItemValue( const ControllerID& controllerId,
                               const JsonObject& data,
                               SendResponseCallback sendResponseCallback = []( const JsonObject& ) {} ) = 0;

    virtual RawItems getItemsBy( const ControllerID& controllerId, const JsonObject& condition ) const = 0;
    virtual RawDevices getDevicesBy( const ControllerID& controllerId, const JsonObject& condition ) const = 0;
    // Devices
    virtual JsonObject getSingleDevice( const ControllerID& controllerId, const DeviceID& deviceId ) const = 0;
    virtual JsonObject setDeviceArmed( const ControllerID& controllerId, const DeviceID& deviceId, bool armed ) = 0;
};

}  // namespace ExternalControllers
}  // namespace HubPlatform
