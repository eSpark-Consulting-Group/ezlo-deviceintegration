#include "bluetooth/bluetooth_environment.h"

#include "bluetooth/script_events/discovery_stopped.h"
#include "bluetooth/script_events/discovery_started.h"
#include "bluetooth/script_events/device_discovered.h"
#include "bluetooth/script_events/device_removed.h"
#include "bluetooth/script_events/device_connected.h"
#include "bluetooth/script_events/device_disconnected.h"
#include "bluetooth/script_events/device_paired.h"
#include "bluetooth/script_events/device_unpaired.h"
#include "bluetooth/script_events/characteristic_updated.h"

#include "addon_interfaces/addon.h"
#include "bluetooth/bluetooth_addon_db.h"

#include <tiny/tiny_consolecolors.h>
#include "bluetooth/log.h"

namespace HubPlatform {

BluetoothEnvironment::BluetoothEnvironment( HubPlatform::Addons::Addon& addon )
: m_addon( addon ) {}

void BluetoothEnvironment::onDiscoveryStopped() {
    m_addon.emit( DiscoveryStopped() );
}

void BluetoothEnvironment::onDiscoveryStarted() {
    m_addon.emit( DiscoveryStarted() );
}

void BluetoothEnvironment::onDeviceDiscovered( const std::string& deviceAddress ) {
    m_addon.emit( DeviceDiscovered( deviceAddress ) );
}

void BluetoothEnvironment::onDeviceRemoved( const std::string& deviceAddress ) {
    removeDeviceFromDB( deviceAddress );

    m_addon.emit( DeviceRemoved( deviceAddress ) );
}

void BluetoothEnvironment::onDeviceConnected( const std::string& hostAddress,
                                              const std::string& deviceAddress,
                                              const std::vector< std::string >& characteristicUUIDs ) {
    addDeviceToDB( deviceAddress, characteristicUUIDs );

    m_addon.sendBroadcast( BT_INCOMMING_CONNECTION_ESTABLISHED,
                           JSON( "hostMAC" << hostAddress << "peerMAC" << deviceAddress ),
                           GroupsNames::NETWORK );
    // TODO: send an fd representing a bt/ble characteristic pipe to an ha-networkd/ha-uid services

    m_addon.emit( DeviceConnected( deviceAddress ) );
}

void BluetoothEnvironment::onDeviceDisconnected( const std::string& deviceAddress ) {
    removeDeviceFromDB( deviceAddress );

    m_addon.emit( DeviceDisconnected( deviceAddress ) );
}

void BluetoothEnvironment::onDevicePaired( const std::string& deviceAddress,
                                           const std::vector< std::string >& characteristicUUIDs ) {
    addDeviceToDB( deviceAddress, characteristicUUIDs );

    m_addon.emit( DevicePaired( deviceAddress ) );
}

void BluetoothEnvironment::onDeviceUnpaired( const std::string& deviceAddress ) {
    removeDeviceFromDB( deviceAddress );

    m_addon.emit( DeviceUnpaired( deviceAddress ) );
}

void BluetoothEnvironment::onCharacteristicUpdated( const std::string& deviceAddress,
                                                    const std::string& characteristicUUID,
                                                    const std::vector< std::uint8_t >& value ) {
    m_addon.emit( CharacteristicUpdated( deviceAddress, characteristicUUID, value ) );
}

void BluetoothEnvironment::removeDeviceFromDB( const std::string& deviceAddress ) {
    using namespace BluetoothAddonDB;

    auto& db = m_addon.getDBClient();
    db.remove( BluetoothDeviceTable, JSON( DeviceAddressField << JSON( "$eq" << deviceAddress ) ) );
}

void BluetoothEnvironment::addDeviceToDB( const std::string& deviceAddress,
                                          const std::vector< std::string >& characteristicUUIDs ) {
    using namespace BluetoothAddonDB;

    auto& db = m_addon.getDBClient();
    db.insert( BluetoothDeviceTable,
               JSON( DeviceAddressField << deviceAddress << DeviceCharacteristicsField << characteristicUUIDs ) );
}

}  // namespace HubPlatform
