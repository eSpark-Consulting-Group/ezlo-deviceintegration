#include "bluetooth/bluetooth_client.h"

#include "jsonrpc/jsonrpc.h"
#include "bluetooth/bluetooth_addon.h"
#include "bluetooth/bluetooth_addon_db.h"

namespace HubPlatform {

BluetoothClient::BluetoothClient( Addons::Addon& hostAddon )
: ScriptClient( Addons::Addon::createQualifiedName( BluetoothAddon::Name ), hostAddon ) {}

void BluetoothClient::startDiscovery() {
    sendMethodCall( prepareMethod( BluetoothAddon::StartDiscoveryMethod ) );
}

void BluetoothClient::stopDiscovery() {
    sendMethodCall( prepareMethod( BluetoothAddon::StopDiscoveryMethod ) );
}

void BluetoothClient::removeDevice( const std::string& deviceAddress ) {
    auto method = prepareMethod( BluetoothAddon::RemoveDeviceMethod );
    method[ "params" ] = JSON( "deviceAddress" << deviceAddress );
    sendMethodCall( std::move( method ) );
}

void BluetoothClient::pair( const std::string& deviceAddress ) {
    auto method = prepareMethod( BluetoothAddon::PairMethod );
    method[ "params" ] = JSON( "deviceAddress" << deviceAddress );
    sendMethodCall( std::move( method ) );
}

void BluetoothClient::connectToDevice( const std::string& deviceAddress ) {
    auto method = prepareMethod( BluetoothAddon::ConnectToDeviceMethod );
    method[ "params" ] = JSON( "deviceAddress" << deviceAddress );
    sendMethodCall( std::move( method ) );
}

void BluetoothClient::disconnectDevice( const std::string& deviceAddress ) {
    auto method = prepareMethod( BluetoothAddon::DisconnectDeviceMethod );
    method[ "params" ] = JSON( "deviceAddress" << deviceAddress );
    sendMethodCall( std::move( method ) );
}

void BluetoothClient::writeCharacteristic( const std::string& deviceAddress,
                                           const std::string& uuid,
                                           const std::vector< std::uint8_t >& data ) {
    auto method = prepareMethod( BluetoothAddon::WriteCharacteristicMethod );
    method[ "params" ] = JSON( "deviceAddress" << deviceAddress << "uuid" << uuid << "data" << data );
    sendMethodCall( std::move( method ) );
}

void BluetoothClient::readCharacteristic( const std::string& deviceAddress, const std::string& uuid ) {
    auto method = prepareMethod( BluetoothAddon::ReadCharacteristicMethod );
    method[ "params" ] = JSON( "deviceAddress" << deviceAddress << "uuid" << uuid );
    sendMethodCall( std::move( method ) );
}

Json BluetoothClient::getDeviceAsJson( const std::string& deviceAddress ) {
    using namespace BluetoothAddonDB;

    const auto& db = getHost().getDBClient();
    return db.findOne( BluetoothDeviceTable, JSON( DeviceAddressField << deviceAddress ), nullptr );
}

void BluetoothClient::startNotify( const std::string& deviceAddress, const std::string& uuid ) {
    auto method = prepareMethod( BluetoothAddon::StartNotifyMethod );
    method[ "params" ] = JSON( "deviceAddress" << deviceAddress << "uuid" << uuid );
    sendMethodCall( std::move( method ) );
}

void BluetoothClient::stopNotify( const std::string& deviceAddress, const std::string& uuid ) {
    auto method = prepareMethod( BluetoothAddon::StopNotifyMethod );
    method[ "params" ] = JSON( "deviceAddress" << deviceAddress << "uuid" << uuid );
    sendMethodCall( std::move( method ) );
}

void BluetoothClient::maintainConnection( const std::string& deviceAddress ) {
    auto method = prepareMethod( BluetoothAddon::MaintainConnectionMethod );
    method[ "params" ] = JSON( "deviceAddress" << deviceAddress );
    sendMethodCall( std::move( method ) );
}

}  // namespace HubPlatform
