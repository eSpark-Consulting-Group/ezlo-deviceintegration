#include "bluetooth/bluetooth_addon.h"
#include "bluetooth/version/bluetooth_addon_version.h"

#include "common_utils/cpp14.h"
#include "json_utils/json_utils.h"
#include "bluetooth/bluetooth_manager.h"
#include "bluetooth/bluetooth_environment.h"

namespace HubPlatform {

const char* const BluetoothAddon::Name = "bluetooth";
const char* const BluetoothAddon::StartDiscoveryMethod = "startDiscovery";
const char* const BluetoothAddon::StopDiscoveryMethod = "stopDiscovery";
const char* const BluetoothAddon::RemoveDeviceMethod = "removeDevice";
const char* const BluetoothAddon::ConnectToDeviceMethod = "connectToDevice";
const char* const BluetoothAddon::DisconnectDeviceMethod = "disconnectDevice";
const char* const BluetoothAddon::WriteCharacteristicMethod = "writeCharacteristic";
const char* const BluetoothAddon::ReadCharacteristicMethod = "readCharacteristic";
const char* const BluetoothAddon::PairMethod = "pair";
const char* const BluetoothAddon::StartNotifyMethod = "startNotify";
const char* const BluetoothAddon::StopNotifyMethod = "stopNotify";
const char* const BluetoothAddon::MaintainConnectionMethod = "maintainConnection";

BluetoothAddon::BluetoothAddon()
: Addons::Addon( Name ) {}

BluetoothAddon::~BluetoothAddon() = default;

IBluetoothManager& BluetoothAddon::getManager() {
    return *m_bluetoothManager;
}

CommonUtils::AddonVersion BluetoothAddon::getVersion() const {
    return Addons::Bluetooth::getVersion();
}

void BluetoothAddon::handleLegacyMethodCall( const JsonObject& json, const std::string& sender ) {
    const auto method = json_get_str( json[ "method" ] );
    if( method == StartDiscoveryMethod ) {
        m_bluetoothManager->startDiscovery();
    }
    else if( method == StopDiscoveryMethod ) {
        m_bluetoothManager->stopDiscovery();
    }
    else if( method == StartNotifyMethod ) {
        const auto& params = json[ "params" ];
        m_bluetoothManager->startNotify( params[ "deviceAddress" ], params[ "uuid" ] );
    }
    else if( method == StopNotifyMethod ) {
        const auto& params = json[ "params" ];
        m_bluetoothManager->stopNotify( params[ "deviceAddress" ], params[ "uuid" ] );
    }
    else if( method == RemoveDeviceMethod ) {
        m_bluetoothManager->removeDevice( json_get_str( json[ "params" ][ "deviceAddress" ] ) );
    }
    else if( method == ConnectToDeviceMethod ) {
        m_bluetoothManager->connectToDevice( json_get_str( json[ "params" ][ "deviceAddress" ] ) );
    }
    else if( method == DisconnectDeviceMethod ) {
        m_bluetoothManager->disconnectDevice( json_get_str( json[ "params" ][ "deviceAddress" ] ) );
    }
    else if( method == WriteCharacteristicMethod ) {
        const auto& params = json[ "params" ];
        m_bluetoothManager->writeCharacteristic(
            params[ "deviceAddress" ], params[ "uuid" ], json_get_vec< std::uint8_t >( params, "data" ) );
    }
    else if( method == ReadCharacteristicMethod ) {
        const auto& params = json[ "params" ];
        m_bluetoothManager->readCharacteristic( params[ "deviceAddress" ], params[ "uuid" ] );
    }
    else if( method == PairMethod ) {
        m_bluetoothManager->pair( json_get_str( json[ "params" ][ "deviceAddress" ] ) );
    }
    else if( method == MaintainConnectionMethod ) {
        m_bluetoothManager->maintainConnection( json_get_str( json[ "params" ][ "deviceAddress" ] ) );
    }
    else {
        Addon::handleLegacyMethodCall( json, sender );
    }
}

void BluetoothAddon::handleMethodCall( const JsonObject& request,
                                       MessageSubClasses requestType,
                                       const std::string& sender ) {
    switch( requestType ) {
    case BT_START_SERVICE_AP: {
        auto name = json_get_str( request, "name" );
        auto pinCode = json_get_str( request, "pinCode" );
        auto serviceUUID = json_get_str( request, "serviceUUID" );
        auto characterUUID = json_get_str( request, "characterUUID" );

        Log::info( cc::info( "Got request to start service AP, name: " ) + cc::note( name ) );

        m_bluetoothManager->startServiceAP( name, pinCode, serviceUUID, characterUUID );
        break;
    }
    case BT_STOP_SERVICE_AP: {
        Log::info( cc::info( "Got request to stop service AP" ) );

        m_bluetoothManager->stopServiceAP();
        break;
    }
    }
}

void BluetoothAddon::setUp() {
    Addon::setUp();

    if( !m_bluetoothEnvironment ) {
        m_bluetoothEnvironment = std::make_unique< BluetoothEnvironment >( *this );
    }

    if( !m_bluetoothManager ) {
        m_bluetoothManager = std::make_unique< BluetoothManager >( *m_bluetoothEnvironment );
    }

    m_bluetoothManager->init();
}

}  // namespace HubPlatform
