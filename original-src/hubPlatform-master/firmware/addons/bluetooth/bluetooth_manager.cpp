#include "bluetooth/bluetooth_manager.h"

#include "bluetooth/bluez_interfaces.h"
#include "bluetooth/entities/adapter.h"
#include "bluetooth/entities/device.h"
#include "bluetooth/entities/agent_manager.h"
#include "bluetooth/proxies/advertising_manager_proxy.h"
#include "bluetooth/proxies/device_proxy.h"
#include "bluetooth/proxies/adapter_proxy.h"
#include "bluetooth/proxies/agent_manager_proxy.h"
#include "bluetooth/proxies/gatt_characteristic_proxy.h"
#include "bluetooth/proxies/gatt_descriptor_proxy.h"
#include "bluetooth/proxies/gatt_manager_proxy.h"
#include "bluetooth/log.h"
#include "bluetooth/exceptions/bluetooth_manager_initialization_failed.h"

#include "common_utils/cpp14.h"
#include "addon_interfaces/addon.h"
#include <tiny/tiny_consolecolors.h>

#include <iostream>
#include <cstring>
#include <algorithm>

static const std::chrono::seconds InitTimeout( 10 );
static const std::chrono::seconds AutoConnectTimeout( 1 );

namespace HubPlatform {

BluetoothManager::BluetoothManager( IBluetoothEnvironment& bluetoothEnvironment )
: m_bluetoothEnvironment( bluetoothEnvironment )
, m_connection( nullptr, dbus_connection_unref )
, m_client( nullptr, g_dbus_client_unref )
, m_mainLoop( nullptr, g_main_loop_unref )
, m_updateConnectionsTimer( std::bind( &BluetoothManager::updateConnections, this ) ) {}

BluetoothManager::~BluetoothManager() = default;

void BluetoothManager::init() {
    m_connection.reset( g_dbus_setup_bus( DBUS_BUS_SYSTEM, nullptr, nullptr ) );
    g_dbus_attach_object_manager( m_connection.get() );

    m_client.reset( g_dbus_client_new( m_connection.get(), "org.bluez", "/org/bluez" ) );
    g_dbus_client_set_connect_watch( m_client.get(), BluetoothManager::connectWatch, this );
    g_dbus_client_set_disconnect_watch( m_client.get(), BluetoothManager::disconnectWatch, this );
    g_dbus_client_set_signal_watch( m_client.get(), BluetoothManager::signalWatch, this );
    g_dbus_client_set_ready_watch( m_client.get(), BluetoothManager::readyWatch, this );
    g_dbus_client_set_proxy_handlers( m_client.get(),
                                      BluetoothManager::proxyAddedHandler,
                                      BluetoothManager::proxyRemovedHandler,
                                      BluetoothManager::propertyChangedHandler,
                                      this );

    std::thread( [this]() {
        m_mainLoop.reset( g_main_loop_new( nullptr, FALSE ) );
        g_main_loop_run( m_mainLoop.get() );
    } )
        .detach();

    auto readyFuture = m_ready.get_future();
    const auto status = readyFuture.wait_for( InitTimeout );
    switch( status ) {
    case std::future_status::deferred:
    case std::future_status::timeout:
        throw BluetoothManagerInitializationFailed();

    case std::future_status::ready:
        m_updateConnectionsTimer.start( AutoConnectTimeout );
        break;
    }
}

void BluetoothManager::registerAgentManager( AgentManager::AgentCapability capability ) {
    safeAgentManagerCall(
        [capability]( const AgentManagerPtr& agentManager ) { agentManager->registerAgent( capability ); } );
}

void BluetoothManager::unregisterAgentManager() {
    safeAgentManagerCall( []( const AgentManagerPtr& agentManager ) { agentManager->unregisterAgent(); } );
}

void BluetoothManager::startAdvertising( const std::string& localName,
                                         const std::vector< std::string >& advertiseServiceUUIDs ) {
    safeAdapterCall( [this, localName, advertiseServiceUUIDs]( const AdapterPtr& activeAdapter ) {
        activeAdapter->startAdvertising( localName, advertiseServiceUUIDs );
    } );
}

void BluetoothManager::stopAdvertising() {
    safeAdapterCall( []( const AdapterPtr& activeAdapter ) { activeAdapter->stopAdvertising(); } );
}

void BluetoothManager::startServiceAP( const std::string& localName,
                                       const std::string& pinCode,
                                       const std::string& serviceUUID,
                                       const std::string& characterUUID ) {
    registerAgentManager( AgentManager::AgentCapability::KeyboardOnly );
    startAdvertising( localName, { serviceUUID } );

    std::this_thread::sleep_for( std::chrono::seconds( 5 ) );

    safeAdapterCall( [&, serviceUUID, characterUUID]( const AdapterPtr& activeAdapter ) {
        activeAdapter->registerGattApplication( serviceUUID, characterUUID );
    } );
}

void BluetoothManager::stopServiceAP() {
    safeAdapterCall( []( const AdapterPtr& activeAdapter ) { activeAdapter->unregisterGattApplication(); } );
    stopAdvertising();
    unregisterAgentManager();
}

void BluetoothManager::updateConnnectionsHander( sigval args ) {
    if( args.sival_ptr ) {
        static_cast< BluetoothManager* >( args.sival_ptr )->updateConnections();
    }
}

void BluetoothManager::connectWatch( DBusConnection* /*connection*/, void* /*userData*/ ) {
    LOG_INFO( "connectWatch called" );
}

void BluetoothManager::disconnectWatch( DBusConnection* /*connection*/, void* /*userData*/ ) {
    LOG_INFO( "disconnectWatch called" );
}

void BluetoothManager::signalWatch( DBusConnection* /*connection*/, DBusMessage* /*message*/, void* /*userData*/ ) {
    LOG_INFO( "signalWatch called" );
}

void BluetoothManager::readyWatch( GDBusClient* /*client*/, void* userData ) {
    if( userData ) {
        static_cast< BluetoothManager* >( userData )->setReady();
        LOG_INFO( "Bluetooth manager is ready" );
    }
}

void BluetoothManager::proxyAddedHandler( GDBusProxy* proxy, void* userData ) {
    if( userData ) {
        static_cast< BluetoothManager* >( userData )->addProxy( proxy );
    }
}

void BluetoothManager::addProxy( GDBusProxy* proxy ) {
    const auto interface = Proxy( proxy ).getInterface();
    if( interface == BluezInterfaces::Adapter ) {
        addAdapterProxy( AdapterProxy( proxy ) );
    }
    else if( interface == BluezInterfaces::Device ) {
        addDeviceProxy( DeviceProxy( proxy ) );
    }
    else if( interface == BluezInterfaces::GattService ) {
        addAttributeProxy( GattServiceProxy( proxy ) );
    }
    else if( interface == BluezInterfaces::GattCharacteristic ) {
        addAttributeProxy( GattCharacteristicProxy( proxy ) );
    }
    else if( interface == BluezInterfaces::GattDescriptor ) {
        addAttributeProxy( GattDescriptorProxy( proxy ) );
    }
    else if( interface == BluezInterfaces::AgentManager ) {
        addAgentManagerProxy( AgentManagerProxy( proxy ) );
    }
    else if( interface == BluezInterfaces::LEAdvertisingManager ) {
        addAdvertisingManagerProxy( AdvertisingManagerProxy( proxy ) );
    }
    else if( interface == BluezInterfaces::GattManager ) {
        addGattManagerProxy( GattManagerProxy( proxy ) );
    }
    else {
        LOG_WARN( "Interface '%s' proxy adding isn't handled", interface.c_str() );
    }
}

void BluetoothManager::proxyRemovedHandler( GDBusProxy* proxy, void* userData ) {
    if( userData ) {
        static_cast< BluetoothManager* >( userData )->removeProxy( proxy );
    }
}

void BluetoothManager::removeProxy( GDBusProxy* proxy ) {
    const auto interface = Proxy( proxy ).getInterface();
    if( interface == BluezInterfaces::Device ) {
        removeDeviceProxy( DeviceProxy( proxy ) );
    }
    else if( interface == BluezInterfaces::Adapter ) {
        removeAdapterProxy( AdapterProxy( proxy ) );
    }
    else if( interface == BluezInterfaces::GattService ) {
        removeAttributeProxy( GattServiceProxy( proxy ) );
    }
    else if( interface == BluezInterfaces::GattCharacteristic ) {
        removeAttributeProxy( GattCharacteristicProxy( proxy ) );
    }
    else if( interface == BluezInterfaces::GattDescriptor ) {
        removeAttributeProxy( GattDescriptorProxy( proxy ) );
    }
    else {
        LOG_WARN( "Interface '%s' proxy removing isn't handled", interface.c_str() );
    }
}

void BluetoothManager::removeDeviceProxy( const DeviceProxy& deviceProxy ) {
    safeAdapterCall(
        [&deviceProxy]( const AdapterPtr& activeAdapter ) { activeAdapter->removeDevice( deviceProxy ); } );
}

void BluetoothManager::removeAdapterProxy( const AdapterProxy& adapterProxy ) {
    m_activeAdapter.process( [&adapterProxy]( AdapterPtr& activeAdapter ) {
        if( activeAdapter->getAddress() == adapterProxy.getAddress() ) {
            activeAdapter.reset();
            LOG_INFO( "Bluetooth adapter removed" );
        }
    } );
}

void BluetoothManager::addAgentManagerProxy( const AgentManagerProxy& agentManagerProxy ) {
    m_agentManager.process( [this, &agentManagerProxy]( AgentManagerPtr& agentManager ) {
        if( !agentManager ) {
            agentManager.reset( new AgentManager( m_connection.get(), agentManagerProxy ) );
            LOG_DEBUG( "Agent manager proxy registered" );
        }
        else {
            LOG_WARN( "Multiple agent manager proxies are not supported" );
        }
    } );
}

void BluetoothManager::addGattManagerProxy( const GattManagerProxy& gattManagerProxy ) {
    safeAdapterCall( [this, &gattManagerProxy]( const AdapterPtr& activeAdapter ) {
        if( gattManagerProxy.getProxyPath() == activeAdapter->getProxyPath() ) {
            activeAdapter->setupGattManager( gattManagerProxy );
            LOG_DEBUG( "Gatt manager proxy registered" );
        }
        else {
            LOG_ERROR( "GattManager proxy object isn't owned by a current active adapter" );
        }
    } );
}

void BluetoothManager::propertyChangedHandler( GDBusProxy* proxy,
                                               const char* name,
                                               DBusMessageIter* iter,
                                               void* userData ) {
    if( userData ) {
        static_cast< BluetoothManager* >( userData )->onPropertyChanged( proxy, name, iter );
    }
}

void BluetoothManager::onPropertyChanged( GDBusProxy* proxy, const char* name, DBusMessageIter* iter ) {
    const auto interface = Proxy( proxy ).getInterface();
    if( interface == BluezInterfaces::Adapter ) {
        onAdapterPropertyChanged( AdapterProxy( proxy ), name );
    }
    else if( interface == BluezInterfaces::Device ) {
        onDevicePropertyChanged( DeviceProxy( proxy ), name );
    }
    else if( interface == BluezInterfaces::GattCharacteristic ) {
        onAttributePropertyChanged( GattCharacteristicProxy( proxy ), name );
    }
    else if( interface == BluezInterfaces::GattDescriptor ) {
        onAttributePropertyChanged( GattDescriptorProxy( proxy ), name );
    }
    else if( interface == BluezInterfaces::GattService ) {
        onAttributePropertyChanged( GattServiceProxy( proxy ), name );
    }
    else if( interface == BluezInterfaces::LEAdvertisingManager ) {
        onAdvertisingManagerProxyPropertyChanged( AdvertisingManagerProxy( proxy ), name );
    }
    else {
        LOG_WARN( "Interface '%s' proxy property changing isn't handled", interface.c_str() );
    }
}

void BluetoothManager::onAdapterPropertyChanged( const AdapterProxy& adapterProxy, const std::string& propertyName ) {
    safeAdapterCall( [&adapterProxy, &propertyName]( const AdapterPtr& activeAdapter ) {
        if( activeAdapter->getProxyPath() == adapterProxy.getProxyPath() ) {
            activeAdapter->onPropertyChanged( propertyName );
        }
    } );
}

void BluetoothManager::onAdvertisingManagerProxyPropertyChanged( const AdvertisingManagerProxy advertisingManagerProxy,
                                                                 const std::string& propertyName ) {
    LOG_DEBUG( "advertise property changed: %s", propertyName.c_str() );
}

void BluetoothManager::onDevicePropertyChanged( const DeviceProxy& deviceProxy, const std::string& propertyName ) {
    safeAdapterCall( [&deviceProxy, &propertyName]( const AdapterPtr& activeAdapter ) {
        activeAdapter->onDevicePropertyChanged( deviceProxy, propertyName );
    } );
}

void BluetoothManager::startDiscovery() {
    safeAdapterCall( []( const AdapterPtr& activeAdapter ) { activeAdapter->startDiscovery(); } );
}

void BluetoothManager::stopDiscovery() {
    safeAdapterCall( []( const AdapterPtr& activeAdapter ) { activeAdapter->stopDiscovery(); } );
}

void BluetoothManager::removeDevice( const std::string& deviceAddress ) {
    safeAdapterCall(
        [&deviceAddress]( const AdapterPtr& activeAdapter ) { activeAdapter->removeDevice( deviceAddress ); } );
}

void BluetoothManager::connectToDevice( const std::string& deviceAddress ) {
    safeAdapterCall(
        [&deviceAddress]( const AdapterPtr& activeAdapter ) { activeAdapter->connectToDevice( deviceAddress ); } );
}

void BluetoothManager::disconnectDevice( const std::string& deviceAddress ) {
    safeAdapterCall(
        [&deviceAddress]( const AdapterPtr& activeAdapter ) { activeAdapter->disconnectDevice( deviceAddress ); } );
}

void BluetoothManager::pair( const std::string& deviceAddress ) {
    safeAdapterCall( [&deviceAddress]( const AdapterPtr& activeAdapter ) { activeAdapter->pair( deviceAddress ); } );
}

void BluetoothManager::writeCharacteristic( const std::string& deviceAddress,
                                            const std::string& uuid,
                                            const std::vector< std::uint8_t >& data ) {
    safeAdapterCall(
        [&]( const AdapterPtr& activeAdapter ) { activeAdapter->writeCharacteristic( deviceAddress, uuid, data ); } );
}

void BluetoothManager::readCharacteristic( const std::string& deviceAddress, const std::string& uuid ) {
    safeAdapterCall(
        [&]( const AdapterPtr& activeAdapter ) { activeAdapter->readCharacteristic( deviceAddress, uuid ); } );
}

void BluetoothManager::addAdapterProxy( const AdapterProxy& adapterProxy ) {
    m_activeAdapter.process( [this, &adapterProxy]( AdapterPtr& activeAdapter ) {
        if( !activeAdapter ) {
            activeAdapter.reset( new Adapter( m_connection.get(), m_bluetoothEnvironment, adapterProxy ) );
            LOG_INFO( "Bluetooth adapter added" );
        }
        else {
            LOG_WARN( "Multiple bluetooth addapters are not supported" );
        }
    } );
}

void BluetoothManager::addDeviceProxy( const DeviceProxy& deviceProxy ) {
    safeAdapterCall( [this, &deviceProxy]( const AdapterPtr& activeAdapter ) {
        if( deviceProxy.getAdapterPath() == activeAdapter->getProxyPath() ) {
            activeAdapter->addDevice( deviceProxy );
        }
        else {
            LOG_ERROR( "Can't find device parent bluetooth adapter" );
        }
    } );
}

template < typename T >
void BluetoothManager::addAttributeProxy( const T& attribute ) {
    safeAdapterCall(
        [&attribute]( const AdapterPtr& activeAdapter ) { activeAdapter->addDeviceAttribute( attribute ); } );
}

template < typename T >
void BluetoothManager::removeAttributeProxy( const T& attribute ) {
    safeAdapterCall(
        [&attribute]( const AdapterPtr& activeAdapter ) { activeAdapter->removeDeviceAttribute( attribute ); } );
}

void BluetoothManager::addAdvertisingManagerProxy( const AdvertisingManagerProxy& advertisingManagerProxy ) {
    safeAdapterCall( [this, &advertisingManagerProxy]( const AdapterPtr& activeAdapter ) {
        if( advertisingManagerProxy.getProxyPath() == activeAdapter->getProxyPath() ) {
            activeAdapter->setupAdvertisingManagerProxy( advertisingManagerProxy );
            LOG_DEBUG( "Advertising manager proxy registered" );
        }
        else {
            LOG_ERROR( "LEAdvertisingManager proxy object isn't owned by a current active adapter" );
        }
    } );
}

template < typename T >
void BluetoothManager::onAttributePropertyChanged( const T& attributeProxy, const std::string& propertyName ) {
    safeAdapterCall( [&attributeProxy, &propertyName]( const AdapterPtr& activeAdapter ) {
        activeAdapter->onAttributePropertyChanged( attributeProxy, propertyName );
    } );
}

void BluetoothManager::setReady() {
    m_ready.set_value( true );
}

template < typename FuncType >
void BluetoothManager::safeAdapterCall( FuncType&& func ) {
    m_activeAdapter.process( [&func]( const AdapterPtr& activeAdapter ) {
        if( !activeAdapter ) {
            LOG_ERROR( "No active bluetooth adapter found for call" );
        }
        else {
            func( activeAdapter );
        }
    } );
}

template < typename FuncType >
void BluetoothManager::safeAgentManagerCall( FuncType&& func ) {
    m_agentManager.process( [&func]( const AgentManagerPtr& agentManager ) {
        if( !agentManager ) {
            LOG_ERROR( "Agent manager isn't registered yet" );
        }
        else {
            func( agentManager );
        }
    } );
}

void BluetoothManager::startNotify( const std::string& deviceAddress, const std::string& uuid ) {
    safeAdapterCall( [&deviceAddress, &uuid]( const AdapterPtr& activeAdapter ) {
        activeAdapter->startNotify( deviceAddress, uuid );
    } );
}

void BluetoothManager::stopNotify( const std::string& deviceAddress, const std::string& uuid ) {
    safeAdapterCall( [&deviceAddress, &uuid]( const AdapterPtr& activeAdapter ) {
        activeAdapter->stopNotify( deviceAddress, uuid );
    } );
}

void BluetoothManager::maintainConnection( const std::string& deviceAddress ) {
    safeAdapterCall(
        [&deviceAddress]( const AdapterPtr& activeAdapter ) { activeAdapter->maintainConnection( deviceAddress ); } );
}

void BluetoothManager::updateConnections() {
    safeAdapterCall( []( const AdapterPtr& activeAdapter ) { activeAdapter->updateConnections(); } );
}

}  // namespace HubPlatform
