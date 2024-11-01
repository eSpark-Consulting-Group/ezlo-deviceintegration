#include "zwave/resources/zwave_device_info.h"
#include "common_utils/firmware_info_helper.h"
#include "common_utils/hub_info_helper.h"
#include "tiny/include/tiny/tiny_utils.h"
#include "log_utils/ezlo_log.h"

namespace HubPlatform {
namespace Zwave {
namespace Resources {
namespace DeviceInfo {

namespace {

const uint8_t HardwareVersionUS = 0x01;
const uint8_t HardwareVersionEU = 0x02;
const uint8_t HardwareVersionDefault = 0x00;

const uint8_t ZWaveChipFirmwareVersion = 0x07;
const uint8_t ZWaveChipFirmwareSubVersion = 0x0F;

const uint16_t ManufactirerId = 0x025B;
const uint16_t ProductTypeE450 = 0x04;
const uint16_t ProductTypeE550 = 0x05;
const uint16_t ProductTypeG150 = 0x06;
const uint16_t ProductTypeBeeWave = 0x07;

const uint16_t ProductTypeDefault = 0x00;
const uint16_t ProductIdUS = 0x01;
const uint16_t ProductIdEU = 0x02;
const uint16_t ProductIdDefault = 0x00;

const uint16_t InstallerIconType = 0x0500;
const uint16_t UserIconType = 0x0500;

const uint16_t ChipFirmwareIdUS = 0x0701;
const uint16_t ChipFirmwareIdEU = 0x0702;
const uint16_t ChipFirmwareIdDefault = 0x0700;

}  // namespace

FirmwareVersion getZwaveChipFirmwareVersion() {
    return { ZWaveChipFirmwareVersion, ZWaveChipFirmwareSubVersion };
}

FirmwareVersion getHubFirmwareVersion() {
    const auto& version = FirmwareInfoHelper::getFirmwareVersion();

    if( version.getMajor() > 0xFF || version.getMinor() > 0xFF ) {
        LOGGER_ERROR( "Firmware version " + std::to_string( version.getMajor() ) + "," +
                      std::to_string( version.getMinor() ) + " is out of zwave firmware versions limit" );
    }

    return { static_cast< uint8_t >( version.getMajor() ), static_cast< uint8_t >( version.getMinor() ) };
}

uint8_t getHardwareVersion() {
    const auto locale = getZwaveChipLocale();
    return locale == ChipLocaleEU ? HardwareVersionEU :
                                    locale == ChipLocaleUS ? HardwareVersionUS : HardwareVersionDefault;
}

uint16_t getManufacturerId() {
    return ManufactirerId;
}

uint16_t getProductType() {
    const auto hubType = HubInfoHelper::getHubType();
    return hubType == "h2.1" ?
               ProductTypeE450 :
               hubType == "h2_secure.1" ?
               ProductTypeE550 :
               hubType == "g150" ? ProductTypeG150 : hubType == "beewave.1" ? ProductTypeBeeWave : ProductTypeDefault;
}

uint16_t getProductId() {
    const auto locale = getZwaveChipLocale();
    return locale == ChipLocaleEU ? ProductIdEU : locale == ChipLocaleUS ? ProductIdUS : ProductIdDefault;
}

uint16_t getInstallerIconType() {
    return InstallerIconType;
}

uint16_t getUserIconType() {
    return UserIconType;
}

uint16_t getZwaveChipFirmwareId() {
    const auto locale = getZwaveChipLocale();
    return locale == ChipLocaleEU ? ChipFirmwareIdEU :
                                    locale == ChipLocaleUS ? ChipFirmwareIdUS : ChipFirmwareIdDefault;
}

std::string getZwaveChipLocale() {
    return tiny::tools::to_upper( FirmwareInfoHelper::getFirmwareRegion() );
}

uint16_t getHubFirmwareId() {
    return getProductType();
}

uint16_t getZWaveAllianceLockingId() {
    // TODO: set custom id
    return getManufacturerId();
}

}  // namespace DeviceInfo
}  // namespace Resources
}  // namespace Zwave
}  // namespace HubPlatform
