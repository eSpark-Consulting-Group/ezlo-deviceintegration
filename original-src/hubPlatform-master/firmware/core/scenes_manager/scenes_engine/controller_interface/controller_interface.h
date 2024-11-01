#pragma once

#include "json_utils/json_utils.h"

namespace SceneEngine {

class ControllerInterface {
public:
    using ItemId = std::string;
    using DeviceId = std::string;
    using HouseModeId = std::string;
    using ControllerId = std::string;
    using ItemIds = std::vector< ItemId >;
    using DeviceIds = std::vector< DeviceId >;
    using Device = JsonObject;
    using Item = JsonObject;
    using Items = std::vector< Item >;
    using RawItems = std::vector< JsonObject >;
    using RawDevices = std::vector< JsonObject >;
    using ItemData = JsonObject;
    using Response = JsonObject;
    using IsSuccessful = bool;
    using BoolResult = std::pair< bool, IsSuccessful >;
    using SendResponseCallback = std::function< void( const JsonObject& response ) >;
    using SubscriptionNoParamsCallback = std::function< void() >;
    using SubscriptionWithParamsCallback = std::function< void( const Json& ) >;

    struct SubscriptionCallbackSelector {
        template <
            class Func,
            typename std::enable_if< std::is_assignable< SubscriptionNoParamsCallback, Func >::value, int >::type = 0 >
        SubscriptionCallbackSelector( Func callback )
        : noParamsCallback( callback ) {}

        template < class Func,
                   typename std::enable_if< std::is_assignable< SubscriptionWithParamsCallback, Func >::value,
                                            int >::type = 0 >
        SubscriptionCallbackSelector( Func callback )
        : withParamsCallback( callback ) {}

        SubscriptionNoParamsCallback noParamsCallback;
        SubscriptionWithParamsCallback withParamsCallback;
    };

    virtual void subscribeOnItemChanges( const ItemId& itemId ) = 0;
    virtual void subscribeOnItemChanges( const ItemId& itemId, SubscriptionCallbackSelector ) = 0;
    virtual void unsubscribeFromItemChanges( const ItemId& itemId ) = 0;

    virtual void subscribeOnDevicesChanges( const DeviceId& deviceId ) = 0;
    virtual void subscribeOnDevicesChanges( const DeviceId& deviceId, SubscriptionCallbackSelector ) = 0;
    virtual void unsubscribeFromDevicesChanges( const DeviceId& deviceId ) = 0;

    virtual BoolResult isDeviceArmed( const DeviceId& deviceId ) = 0;
    virtual Device getDevice( const DeviceId& deviceId ) = 0;
    virtual Response setDeviceArmed( const DeviceId& deviceId, bool armed ) = 0;

    virtual void setItemValue( const ItemData& data,
                               SendResponseCallback sendResponseCallback = []( const JsonObject& ) {} ) = 0;

    virtual RawItems getItemsBy( const JsonObject& condition ) = 0;
    virtual RawDevices getDevicesBy( const JsonObject& condition ) = 0;
    virtual Items getItems( const ItemIds& itemIds ) = 0;
    virtual Item getItem( const ItemId& itemId ) = 0;

    virtual Response setHouseMode( const HouseModeId& houseModeId ) = 0;
};

}  // namespace SceneEngine
