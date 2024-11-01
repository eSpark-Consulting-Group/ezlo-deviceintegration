#pragma once

#include <cstdint>
#include <utility>
#include <string>

namespace HubPlatform {
namespace Zwave {
namespace Resources {
namespace DeviceInfo {

constexpr char ChipLocaleUS[] = "US";
constexpr char ChipLocaleEU[] = "EU";

using FirmwareVersion = std::pair< uint8_t, uint8_t >;

FirmwareVersion getZwaveChipFirmwareVersion();

FirmwareVersion getHubFirmwareVersion();

uint8_t getHardwareVersion();

uint16_t getManufacturerId();

uint16_t getProductType();

uint16_t getProductId();

uint16_t getInstallerIconType();

uint16_t getUserIconType();

uint16_t getZwaveChipFirmwareId();

std::string getZwaveChipLocale();

uint16_t getHubFirmwareId();

uint16_t getZWaveAllianceLockingId();

}  // namespace DeviceInfo
}  // namespace Resources
}  // namespace Zwave
}  // namespace HubPlatform
