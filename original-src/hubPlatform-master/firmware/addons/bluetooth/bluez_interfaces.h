#pragma once

namespace HubPlatform {
namespace BluezInterfaces {

using InterfaceName = const char* const;

extern InterfaceName Adapter;
extern InterfaceName Device;
extern InterfaceName AgentManager;
extern InterfaceName GattService;
extern InterfaceName GattCharacteristic;
extern InterfaceName GattDescriptor;
extern InterfaceName GattManager;
extern InterfaceName LEAdvertisingManager;

}  // namespace BluezInterfaces
}  // namespace HubPlatform
