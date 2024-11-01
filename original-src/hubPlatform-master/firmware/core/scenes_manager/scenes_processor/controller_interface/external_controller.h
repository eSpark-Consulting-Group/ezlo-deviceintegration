#pragma once

#include "scenes_manager/scenes_engine/controller_interface/controller_interface.h"
#include "service_interfaces/service.h"
#include "external_controllers/remote_interface/ec_abstract_interface.h"

namespace HubPlatform {

namespace UT {
class ExternalControllerTest;
}

namespace ScenesManager {

class ISubscriptionState;

class ExternalController : public SceneEngine::ControllerInterface,
                           public std::enable_shared_from_this< ExternalController > {
public:
    using ItemId = std::string;
    using DeviceId = std::string;
    using SubscriptionStatePtr = std::shared_ptr< ISubscriptionState >;
    using CurrentStates = std::map< ItemId, SubscriptionStatePtr >;

    ExternalController( const ControllerId& controllerId,
                        ExternalControllers::AbstractInterface& remoteInterface,
                        Services::Service& hostService );

    virtual void subscribeOnItemChanges( const ItemId& itemId ) override;
    virtual void subscribeOnItemChanges( const ItemId& itemId, SubscriptionCallbackSelector ) override;
    virtual void unsubscribeFromItemChanges( const ItemId& itemId ) override;

    virtual void subscribeOnDevicesChanges( const DeviceId& deviceId ) override;
    virtual void subscribeOnDevicesChanges( const DeviceId& deviceId, SubscriptionCallbackSelector ) override;
    virtual void unsubscribeFromDevicesChanges( const DeviceId& deviceId ) override;

    virtual BoolResult isDeviceArmed( const DeviceId& deviceId ) override;
    virtual Device getDevice( const DeviceId& deviceId ) override;
    virtual Response setDeviceArmed( const DeviceId& deviceId, bool armed ) override;

    virtual void setItemValue( const ItemData& data, SendResponseCallback sendResponseCallback ) override;
    virtual RawItems getItemsBy( const JsonObject& condition ) override;
    virtual RawDevices getDevicesBy( const JsonObject& condition ) override;
    virtual Items getItems( const ItemIds& itemIds ) override;
    virtual Item getItem( const ItemId& itemId ) override;

    virtual Response setHouseMode( const HouseModeId& houseModeId ) override;

private:
    void itemAdded( const SceneEngine::ControllerInterface::Item& item );
    void deviceAdded( const SceneEngine::ControllerInterface::Device& device );

private:
    ControllerId m_controllerId;
    ExternalControllers::AbstractInterface& m_remoteInterface;
    Services::Service& m_hostService;
    CurrentStates m_currentItemStates;
    CurrentStates m_currentDeviceStates;

    friend class ControllerFactory;
    friend class ISubscriptionState;
    friend class IEntity;
    friend class UT::ExternalControllerTest;
};

class IEntity {
public:
    using Id = std::string;
    using Entity = Json;
    using CallbackSelector = SceneEngine::ControllerInterface::SubscriptionCallbackSelector;
    using ControllerId = SceneEngine::ControllerInterface::ControllerId;
    using CallbackSelectors = std::vector< CallbackSelector >;
    using CurrentStates = ExternalController::CurrentStates;
    using SubscriptionStatePtr = ExternalController::SubscriptionStatePtr;

    IEntity( ExternalController& externalController, const Id& id );

    virtual Entity getEntity() = 0;
    virtual void subscribe() = 0;
    virtual void unsubscribe() = 0;
    virtual Json getDatabaseQuery() = 0;
    virtual CurrentStates& getCurrentStates() = 0;

    void incrementCounter();
    void decrementCounter();
    int& getCounter();
    const Id& getId();
    void pushCallback( CallbackSelector callback );
    CallbackSelectors& getCallbacks();
    SubscriptionStatePtr& getCurrentState();

protected:
    ExternalControllers::AbstractInterface& getRemoteInterface();
    ControllerId& getControllerId();
    IDbClient& getDbClient();
    CurrentStates& getCurrentItemStates();
    CurrentStates& getCurrentDeviceStates();

protected:
    Id m_id;
    int m_counter;
    CallbackSelectors m_callbacks;

protected:
    ExternalController& m_externalController;
};

class ISubscriptionState {
public:
    using CallbackSelector = SceneEngine::ControllerInterface::SubscriptionCallbackSelector;
    using ControllerId = SceneEngine::ControllerInterface::ControllerId;
    using Id = std::string;
    using EntityPtr = std::unique_ptr< IEntity >;

    ISubscriptionState( ExternalController& externalController, EntityPtr&& entityPtr );

    virtual void subscribe( const Id& id, CallbackSelector callback ) = 0;
    virtual void unsubscribe( const Id& id );
    virtual void entityAdded( const IEntity::Entity& entity ) = 0;

protected:
    IDbClient& getDbClient();
    Services::Service& getHostService();

protected:
    ExternalController& m_externalController;
    EntityPtr m_entityPtr;
};

}  // namespace ScenesManager
}  // namespace HubPlatform
