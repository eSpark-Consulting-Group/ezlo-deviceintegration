#include "scenes_manager/scenes_processor/controller_interface/external_controller.h"
#include "common_utils/cpp14.h"

namespace HubPlatform {
namespace ScenesManager {

using namespace SceneEngine;

class ItemEntity : public IEntity {
public:
    ItemEntity( ExternalController& externalController, const Id& id )
    : IEntity( externalController, id ) {}

    virtual Entity getEntity() override {
        return getRemoteInterface().getItem( getControllerId(), m_id );
    }
    virtual void subscribe() override {
        LOGGER_DEBUG( "subscribeOnItemChanges controllerId: " + getControllerId() + ", itemId: " + m_id );
        getRemoteInterface().subscribeOnItemChanges( getControllerId(), { m_id } );
    }
    virtual void unsubscribe() override {
        LOGGER_DEBUG( "unsubscribeFromItemChanges controllerId: " + getControllerId() + ", itemId: " + m_id );
        getRemoteInterface().unsubscribeFromItemChanges( getControllerId(), { m_id } );
    }
    virtual Json getDatabaseQuery() override {
        return JSON( "controllerId" << getControllerId() << "itemId" << m_id );
    }
    virtual CurrentStates& getCurrentStates() override {
        return getCurrentItemStates();
    }
};

class DeviceEntity : public IEntity {
public:
    DeviceEntity( ExternalController& externalController, const Id& id )
    : IEntity( externalController, id ) {}

    virtual Entity getEntity() override {
        return getRemoteInterface().getSingleDevice( getControllerId(), m_id );
    }
    virtual void subscribe() override {
        LOGGER_DEBUG( "subscribeOnDevicesChanges controllerId: " + getControllerId() + ", deviceId: " + m_id );
        getRemoteInterface().subscribeOnDevicesChanges( getControllerId(), { m_id } );
    }
    virtual void unsubscribe() override {
        LOGGER_DEBUG( "unsubscribeFromDevicesChanges controllerId: " + getControllerId() + ", deviceId: " + m_id );
        getRemoteInterface().unsubscribeFromDevicesChanges( getControllerId(), { m_id } );
    }
    virtual Json getDatabaseQuery() override {
        return JSON( "controllerId" << getControllerId() << "deviceId" << m_id );
    }
    virtual CurrentStates& getCurrentStates() override {
        return getCurrentDeviceStates();
    }
};

class SubscribedState : public ISubscriptionState {
public:
    SubscribedState( ExternalController& externalController, EntityPtr&& entityPtr )
    : ISubscriptionState( externalController, std::move( entityPtr ) ) {}

    virtual void subscribe( const Id& id, CallbackSelector callback ) override {
        getHostService().addTask( [this, callback]() {
            if( callback.noParamsCallback ) {
                callback.noParamsCallback();
            }
            else if( callback.withParamsCallback ) {
                callback.withParamsCallback( m_entityPtr->getEntity() );
            }
        } );
        m_entityPtr->incrementCounter();
    }

    virtual void entityAdded( const IEntity::Entity& /*entity*/ ) override{};
};

class WaitSubscriptionState : public ISubscriptionState {
public:
    WaitSubscriptionState( ExternalController& externalController, EntityPtr&& entityPtr )
    : ISubscriptionState( externalController, std::move( entityPtr ) ) {}

    virtual void subscribe( const Id& id, CallbackSelector callback ) override {
        m_entityPtr->pushCallback( callback );
        m_entityPtr->incrementCounter();
    }

    virtual void entityAdded( const IEntity::Entity& entity ) override {
        getDbClient().update( HaScenesControllerSubscriptionDBName,
                              m_entityPtr->getDatabaseQuery(),
                              JSON( "$set" << JSON( "state"
                                                    << "subscribed" ) ) );

        auto& callbacks = m_entityPtr->getCallbacks();
        for( const auto& callback : callbacks ) {
            if( callback.noParamsCallback ) {
                callback.noParamsCallback();
            }
            else if( callback.withParamsCallback ) {
                callback.withParamsCallback( entity );
            }
        }
        callbacks.clear();
        auto& currentState = m_entityPtr->getCurrentState();
        currentState = std::make_unique< SubscribedState >( m_externalController, std::move( m_entityPtr ) );
    };

    virtual void unsubscribe( const Id& id ) override {
        m_entityPtr->getCallbacks().clear();
        ISubscriptionState::unsubscribe( id );
    }
};

class UnsubcribedState : public ISubscriptionState {
public:
    UnsubcribedState( ExternalController& externalController, EntityPtr&& entityPtr )
    : ISubscriptionState( externalController, std::move( entityPtr ) ) {}

    virtual void subscribe( const Id& id, CallbackSelector callback ) override {
        auto query = JSON( m_entityPtr->getDatabaseQuery() << "state"
                                                           << "wait" );
        m_entityPtr->subscribe();
        getDbClient().insert( HaScenesControllerSubscriptionDBName, query );
        m_entityPtr->pushCallback( callback );
        m_entityPtr->incrementCounter();
        auto& currentState = m_entityPtr->getCurrentState();
        currentState = std::make_unique< WaitSubscriptionState >( m_externalController, std::move( m_entityPtr ) );
    }

    virtual void entityAdded( const IEntity::Entity& /*entity*/ ) override{};
};

ExternalController::ExternalController( const ControllerId& controllerId,
                                        ExternalControllers::AbstractInterface& remoteInterface,
                                        Services::Service& hostService )
: m_controllerId( controllerId )
, m_remoteInterface( remoteInterface )
, m_hostService( hostService ) {
    auto query = JSON( "controllerId" << controllerId << JSON( "itemId" << JSON( "$exists" << true ) ) );
    auto subscriptions = m_hostService.getDBClient().findAll( HaScenesControllerSubscriptionDBName, query, nullptr );
    for( const auto& subscrition : subscriptions ) {
        auto itemId = json_get_str( subscrition, "itemId" );
        auto state = json_get_str( subscrition, "state" );
        if( state == "wait" ) {
            m_currentItemStates.emplace(
                itemId,
                std::make_unique< WaitSubscriptionState >( *this, std::make_unique< ItemEntity >( *this, itemId ) ) );
        }
        else if( state == "subscribed" ) {
            m_currentItemStates.emplace(
                itemId, std::make_unique< SubscribedState >( *this, std::make_unique< ItemEntity >( *this, itemId ) ) );
        }
    }
    query = JSON( "controllerId" << controllerId << JSON( "deviceId" << JSON( "$exists" << true ) ) );
    subscriptions = m_hostService.getDBClient().findAll( HaScenesControllerSubscriptionDBName, query, nullptr );
    for( const auto& subscrition : subscriptions ) {
        auto deviceId = json_get_str( subscrition, "deviceId" );
        auto state = json_get_str( subscrition, "state" );
        if( state == "wait" ) {
            m_currentDeviceStates.emplace( deviceId,
                                           std::make_unique< WaitSubscriptionState >(
                                               *this, std::make_unique< DeviceEntity >( *this, deviceId ) ) );
        }
        else if( state == "subscribed" ) {
            m_currentDeviceStates.emplace(
                deviceId,
                std::make_unique< SubscribedState >( *this, std::make_unique< DeviceEntity >( *this, deviceId ) ) );
        }
    }
}

void ExternalController::subscribeOnItemChanges( const ExternalController::ItemId& itemId ) {
    subscribeOnItemChanges( itemId, []() {} );
}

void ExternalController::subscribeOnItemChanges( const ControllerInterface::ItemId& itemId,
                                                 SubscriptionCallbackSelector callback ) {
    auto result = m_currentItemStates.emplace(
        itemId, std::make_unique< UnsubcribedState >( *this, std::make_unique< ItemEntity >( *this, itemId ) ) );
    ISubscriptionState& state = *result.first->second;
    state.subscribe( itemId, callback );
}

void ExternalController::unsubscribeFromItemChanges( const ControllerInterface::ItemId& itemId ) {
    auto currentStateIt = m_currentItemStates.find( itemId );
    if( currentStateIt == m_currentItemStates.end() ) {
        LOGGER_ERROR( cc::error( "state not found for item: " ) + cc::note( itemId ) );
        return;
    }
    currentStateIt->second->unsubscribe( itemId );
}

void ExternalController::subscribeOnDevicesChanges( const ControllerInterface::DeviceId& deviceId ) {
    subscribeOnDevicesChanges( deviceId, []() {} );
}

void ExternalController::subscribeOnDevicesChanges( const ExternalController::DeviceId& deviceId,
                                                    ControllerInterface::SubscriptionCallbackSelector callback ) {
    auto result = m_currentDeviceStates.emplace(
        deviceId, std::make_unique< UnsubcribedState >( *this, std::make_unique< DeviceEntity >( *this, deviceId ) ) );
    ISubscriptionState& state = *result.first->second;
    state.subscribe( deviceId, callback );
}

void ExternalController::unsubscribeFromDevicesChanges( const ControllerInterface::DeviceId& deviceId ) {
    auto currentStateIt = m_currentDeviceStates.find( deviceId );
    if( currentStateIt == m_currentDeviceStates.end() ) {
        LOGGER_ERROR( cc::error( "state not found for item: " ) + cc::note( deviceId ) );
        return;
    }
    currentStateIt->second->unsubscribe( deviceId );
}

ExternalController::BoolResult ExternalController::isDeviceArmed( const ControllerInterface::DeviceId& deviceId ) {
    JsonObject device = m_remoteInterface.getSingleDevice( m_controllerId, deviceId );
    if( device.empty() ) {
        LOGGER_TRACE( cc::trace( "empty device" ) );
        return { false, false };
    }
    bool armed;
    if( !json_get( device, "armed", armed ) ) {
        LOGGER_TRACE( cc::trace( "empty armed" ) );
        return { false, false };
    }
    return { armed, true };
}

ControllerInterface::Device ExternalController::getDevice( const ExternalController::DeviceId& deviceId ) {
    return m_remoteInterface.getSingleDevice( m_controllerId, deviceId );
}

ControllerInterface::Response ExternalController::setDeviceArmed( const ControllerInterface::DeviceId& deviceId,
                                                                  bool armed ) {
    return m_remoteInterface.setDeviceArmed( m_controllerId, deviceId, armed );
}

void ExternalController::setItemValue( const ControllerInterface::ItemData& data,
                                       ControllerInterface::SendResponseCallback sendResponseCallback ) {
    m_remoteInterface.setItemValue( m_controllerId, data, sendResponseCallback );
}

ControllerInterface::RawItems ExternalController::getItemsBy( const JsonObject& condition ) {
    return m_remoteInterface.getItemsBy( m_controllerId, condition );
}

ControllerInterface::RawDevices ExternalController::getDevicesBy( const JsonObject& condition ) {
    return m_remoteInterface.getDevicesBy( m_controllerId, condition );
}

ControllerInterface::Items ExternalController::getItems( const ControllerInterface::ItemIds& itemIds ) {
    return m_remoteInterface.getItems( m_controllerId, itemIds );
}

ControllerInterface::Item ExternalController::getItem( const ControllerInterface::ItemId& itemId ) {
    return m_remoteInterface.getItem( m_controllerId, itemId );
}

ControllerInterface::Response ExternalController::setHouseMode(
    const ControllerInterface::HouseModeId& /*houseModeId*/ ) {
    return JSON_EMPTY_OBJ;
}

void ExternalController::itemAdded( const ControllerInterface::Item& item ) {
    LOGGER_DEBUG( cc::j( item ) );
    auto itemId = json_get_str( item, "_id" );
    auto currentStateIt = m_currentItemStates.find( itemId );
    if( currentStateIt == m_currentItemStates.end() ) {
        return;
    }
    currentStateIt->second->entityAdded( item );
}

void ExternalController::deviceAdded( const ControllerInterface::Device& device ) {
    LOGGER_DEBUG( cc::j( device ) );
    auto deviceId = json_get_str( device, "_id" );
    auto currentStateIt = m_currentDeviceStates.find( deviceId );
    if( currentStateIt == m_currentDeviceStates.end() ) {
        return;
    }
    currentStateIt->second->entityAdded( device );
}

ISubscriptionState::ISubscriptionState( ExternalController& externalController, EntityPtr&& entityPtr )
: m_externalController( externalController )
, m_entityPtr( std::move( entityPtr ) ) {}

void ISubscriptionState::unsubscribe( const ISubscriptionState::Id& id ) {
    int& counter = m_entityPtr->getCounter();
    --counter;
    if( counter <= 0 ) {
        getDbClient().remove( HaScenesControllerSubscriptionDBName, m_entityPtr->getDatabaseQuery() );
        m_entityPtr->unsubscribe();
        m_entityPtr->getCurrentStates().erase( id );
    }
}

IDbClient& ISubscriptionState::getDbClient() {
    return m_externalController.m_hostService.getDBClient();
}

Services::Service& ISubscriptionState::getHostService() {
    return m_externalController.m_hostService;
}

IEntity::IEntity( ExternalController& externalController, const IEntity::Id& id )
: m_id( id )
, m_counter( 0 )
, m_externalController( externalController ) {}

void IEntity::incrementCounter() {
    ++m_counter;
}

void IEntity::decrementCounter() {
    --m_counter;
}

int& IEntity::getCounter() {
    return m_counter;
}

const IEntity::Id& IEntity::getId() {
    return m_id;
}

void IEntity::pushCallback( IEntity::CallbackSelector callback ) {
    m_callbacks.push_back( callback );
}

IEntity::CallbackSelectors& IEntity::getCallbacks() {
    return m_callbacks;
}

IEntity::SubscriptionStatePtr& IEntity::getCurrentState() {
    return getCurrentStates()[ m_id ];
}

ExternalControllers::AbstractInterface& IEntity::getRemoteInterface() {
    return m_externalController.m_remoteInterface;
}

IEntity::ControllerId& IEntity::getControllerId() {
    return m_externalController.m_controllerId;
}

IDbClient& IEntity::getDbClient() {
    return m_externalController.m_hostService.getDBClient();
}

IEntity::CurrentStates& IEntity::getCurrentItemStates() {
    return m_externalController.m_currentItemStates;
}

IEntity::CurrentStates& IEntity::getCurrentDeviceStates() {
    return m_externalController.m_currentDeviceStates;
}

}  // namespace ScenesManager
}  // namespace HubPlatform
