#include "scenes_manager/scenes_processor/controller_interface/internal_controller.h"
#include "cored/model/resources/cored_resources.h"
#include "log_utils/ezlo_log.h"
#include "cored/remote_interface/cored_interface_factory.h"

namespace HubPlatform {
namespace ScenesManager {

using namespace SceneEngine;

InternalController::InternalController( Services::Service& hostService )
: m_remoteInterface( CoreDRemoteInterface::makeInstance( hostService ) )
, m_hostService( hostService ) {}

void InternalController::subscribeOnItemChanges( const ControllerInterface::ItemId& /*itemId*/ ) {}

void InternalController::subscribeOnItemChanges( const ControllerInterface::ItemId& itemId,
                                                 SubscriptionCallbackSelector callback ) {
    if( callback.noParamsCallback ) {
        callback.noParamsCallback();
    }
    else if( callback.withParamsCallback ) {
        auto internalControllerPtr = shared_from_this();
        m_hostService.addTask( [internalControllerPtr, itemId, callback]() {
            try {
                callback.withParamsCallback( internalControllerPtr->getItem( itemId ) );
            }
            catch( const std::exception& e ) {
                LOGGER_ERROR_RAW( "Unable to subscribe on item changes: " + std::string( e.what() ) );
            }
        } );
    }
}

void InternalController::unsubscribeFromItemChanges( const ControllerInterface::ItemId& /*itemId*/ ) {}

void InternalController::subscribeOnDevicesChanges( const ControllerInterface::DeviceId& /*deviceId*/ ) {}

void InternalController::subscribeOnDevicesChanges( const ControllerInterface::DeviceId& deviceId,
                                                    ControllerInterface::SubscriptionCallbackSelector callback ) {
    if( callback.noParamsCallback ) {
        callback.noParamsCallback();
    }
    else if( callback.withParamsCallback ) {
        auto internalControllerPtr = shared_from_this();
        m_hostService.addTask( [internalControllerPtr, deviceId, callback]() {
            try {
                callback.withParamsCallback( internalControllerPtr->getDevice( deviceId ) );
            }
            catch( const std::exception& e ) {
                LOGGER_ERROR_RAW( "Unable to subscribe on devices changes: " + std::string( e.what() ) );
            }
        } );
    }
}

void InternalController::unsubscribeFromDevicesChanges( const ControllerInterface::DeviceId& /*deviceId*/ ) {}

InternalController::BoolResult InternalController::isDeviceArmed( const ControllerInterface::DeviceId& deviceId ) {
    JsonObject device = getDevice( deviceId );
    if( device.empty() ) {
        LOGGER_TRACE( "empty device from core" );
        return { false, false };
    }
    bool armed = false;
    if( !json_get( device, "armed", armed ) ) {
        LOGGER_TRACE( "empty armed from core" );
        return { false, false };
    }
    return { armed, true };
}

ControllerInterface::Device InternalController::getDevice( const ControllerInterface::DeviceId& deviceId ) {
    return m_remoteInterface->getSingleDevice( deviceId );
}

ControllerInterface::Response InternalController::setDeviceArmed( const ControllerInterface::DeviceId& deviceId,
                                                                  bool armed ) {
    return m_remoteInterface->setDeviceArmed( deviceId, armed );
}

void InternalController::setItemValue( const ControllerInterface::ItemData& data,
                                       ControllerInterface::SendResponseCallback sendResponseCallback ) {
    m_remoteInterface->setItemValue( data, sendResponseCallback );
}

ControllerInterface::Items InternalController::getItems( const ControllerInterface::ItemIds& itemIds ) {
    using namespace HubPlatform::CoreD::Model::Resources;
    JsonObject query = JSON( "$or" << JSON_EMPTY_ARR );
    JsonArray& queryItems = query[ "$or" ];
    for( const auto& id : itemIds ) {
        queryItems.push_back( JSON( ItemFields::IdFieldName << id ) );
    }
    return m_remoteInterface->getItemsBy( queryItems );
}

ControllerInterface::RawItems InternalController::getItemsBy( const JsonObject& condition ) {
    return m_remoteInterface->getItemsBy( condition );
}

ControllerInterface::RawDevices InternalController::getDevicesBy( const JsonObject& condition ) {
    return m_remoteInterface->getDevicesBy( condition );
}

ControllerInterface::Item InternalController::getItem( const ControllerInterface::ItemId& itemId ) {
    return m_remoteInterface->getItem( itemId );
}

ControllerInterface::Response InternalController::setHouseMode( const ControllerInterface::HouseModeId& houseModeId ) {
    return m_remoteInterface->setHouseMode( houseModeId );
}

}  // namespace ScenesManager
}  // namespace HubPlatform
