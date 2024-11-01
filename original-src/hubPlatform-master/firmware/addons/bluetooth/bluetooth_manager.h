#pragma once

#include "bluetooth/ibluetooth_manager.h"
#include "system_utils/thread_unique_safe_wrapper.h"
#include "base_utils/native_timer.h"
#include "bluetooth/entities/agent_manager.h"

#include <gdbus.h>

#include <memory>
#include <atomic>
#include <vector>
#include <mutex>
#include <future>

namespace HubPlatform {

class Adapter;
class AgentManagerProxy;
class DeviceProxy;
class AdapterProxy;
class AdvertisingManagerProxy;
class GattManagerProxy;
class IBluetoothEnvironment;

class BluetoothManager : public IBluetoothManager {
public:
    explicit BluetoothManager( IBluetoothEnvironment& bluetoothEnvironment );
    ~BluetoothManager() final;

    void init() override;

    void startServiceAP( const std::string& localName,
                         const std::string& pinCode,
                         const std::string& serviceUUID,
                         const std::string& characterUUID ) override;
    void stopServiceAP() override;

    void registerAgentManager( AgentManager::AgentCapability capability );
    void unregisterAgentManager();

    void startAdvertising( const std::string& localName, const std::vector< std::string >& advertiseServiceUUIDs = {} );
    void stopAdvertising();

    void startDiscovery() override;
    void stopDiscovery() override;

    void maintainConnection( const std::string& deviceAddress ) override;

    void startNotify( const std::string& deviceAddress, const std::string& uuid ) override;
    void stopNotify( const std::string& deviceAddress, const std::string& uuid ) override;

    void removeDevice( const std::string& deviceAddress ) override;
    void connectToDevice( const std::string& deviceAddress ) override;
    void disconnectDevice( const std::string& deviceAddress ) override;
    void pair( const std::string& deviceAddress ) override;

    void writeCharacteristic( const std::string& deviceAddress,
                              const std::string& uuid,
                              const std::vector< std::uint8_t >& data ) override;
    void readCharacteristic( const std::string& deviceAddress, const std::string& uuid ) override;

private:
    static void updateConnnectionsHander( sigval args );
    static void connectWatch( DBusConnection* connection, void* userData );
    static void disconnectWatch( DBusConnection* connection, void* userData );
    static void signalWatch( DBusConnection* connection, DBusMessage* message, void* userData );
    static void readyWatch( GDBusClient* client, void* userData );
    static void proxyAddedHandler( GDBusProxy* proxy, void* userData );
    static void proxyRemovedHandler( GDBusProxy* proxy, void* userData );
    static void propertyChangedHandler( GDBusProxy* proxy, const char* name, DBusMessageIter* iter, void* userData );

    void setReady();

    void addProxy( GDBusProxy* proxy );
    void removeProxy( GDBusProxy* proxy );
    void onPropertyChanged( GDBusProxy* proxy, const char* name, DBusMessageIter* iter );

    void addAdvertisingManagerProxy( const AdvertisingManagerProxy& advertisingManagerProxy );
    void removeAdvertisingManagerProxy( const AdvertisingManagerProxy& advertisingManagerProxy );

    void addDeviceProxy( const DeviceProxy& deviceProxy );
    void removeDeviceProxy( const DeviceProxy& deviceProxy );

    void onAdapterPropertyChanged( const AdapterProxy& adapterProxy, const std::string& propertyName );
    void onAdvertisingManagerProxyPropertyChanged( const AdvertisingManagerProxy advertisingManagerProxy,
                                                   const std::string& propertyName );
    void onDevicePropertyChanged( const DeviceProxy& deviceProxy, const std::string& propertyName );

    template < typename T >
    void onAttributePropertyChanged( const T& attributeProxy, const std::string& propertyName );

    template < typename T >
    void addAttributeProxy( const T& attribute );
    template < typename T >
    void removeAttributeProxy( const T& attribute );

    void addAdapterProxy( const AdapterProxy& adapterProxy );
    void removeAdapterProxy( const AdapterProxy& adapterProxy );

    void addAgentManagerProxy( const AgentManagerProxy& agentManagerProxy );

    void addGattManagerProxy( const GattManagerProxy& gattManagerProxy );

    template < typename FuncType >
    void safeAdapterCall( FuncType&& func );

    template < typename FuncType >
    void safeAgentManagerCall( FuncType&& func );

    void updateConnections();

private:
    IBluetoothEnvironment& m_bluetoothEnvironment;

    std::unique_ptr< DBusConnection, decltype( &dbus_connection_unref ) > m_connection;
    std::unique_ptr< GDBusClient, decltype( &g_dbus_client_unref ) > m_client;
    std::unique_ptr< GMainLoop, decltype( &g_main_loop_unref ) > m_mainLoop;

    using AdapterPtr = std::unique_ptr< Adapter >;
    using AgentManagerPtr = std::unique_ptr< AgentManager >;
    threads::UniqueSafeWrapper< AdapterPtr > m_activeAdapter;
    threads::UniqueSafeWrapper< AgentManagerPtr > m_agentManager;

    std::promise< bool > m_ready;

    NativeTimer m_updateConnectionsTimer;
};

}  // namespace HubPlatform
