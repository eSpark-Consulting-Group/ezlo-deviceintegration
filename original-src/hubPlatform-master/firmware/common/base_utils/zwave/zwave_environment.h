#pragma once

#include "system_utils/environment.h"

#include <unordered_map>

namespace HubPlatform {
namespace Environment {
namespace Zwave {

static const std::string ZwaveBinariesPath = HubPlatform::Environment::getFirmwareRootFolder().append( "/zw-flash" );
static const std::string ZwaveFlashOutBinaryPath = ZwaveBinariesPath + ( "/flash.bin.out" );
static const std::string ZwaveNvrFactoryBinaryPath = ZwaveBinariesPath + ( "/nvr.bin.factory" );
static const std::string ZwaveNvrOutBinaryPath = ZwaveBinariesPath + ( "/nvr.bin.out" );
static const std::string ZwaveEnvmOutBinaryPath = ZwaveBinariesPath + ( "/envm.bin.out" );

static constexpr char const resetAssertCmd[] = "0";
static constexpr char const resetDeassertCmd[] = "1";

static constexpr size_t const resetAssertPeriodUs = 100000;
static constexpr size_t const resetDeAssertPeriodUs = 100000;

extern uint32_t const frequencyDeviationKHz;
extern const std::unordered_map< uint32_t, const std::string > frequencyToRegionCodeMap;
extern const std::unordered_map< std::string, uint32_t > regionToFrequencyMap;

// absolute address offset can be found in SDS12467-18 Series Z-Wave Chip NVR Flash Page Contents
// magic offset ( comes from eZLO origins in Odessa ) = 9 ( because addresses from 0x00 to 0x08 are used for lock bits )
// address offset = absolute address offset - magic offset
// NVR revision 0.1
static constexpr size_t nvrAddressFirst = 0x09;
static constexpr uint8_t revNvrDataOffset = ( 0x10 - nvrAddressFirst );  // 0x07
static constexpr uint8_t ccalNvrDataOffset = ( 0x11 - nvrAddressFirst );
static constexpr uint8_t pinsNvrDataOffset = ( 0x12 - nvrAddressFirst );
static constexpr uint8_t nvmcsNvrDataOffset = ( 0x13 - nvrAddressFirst );
static constexpr uint8_t sawc1NvrDataOffset = ( 0x14 - nvrAddressFirst );
static constexpr uint8_t sawc2NvrDataOffset = ( 0x15 - nvrAddressFirst );
static constexpr uint8_t sawc3NvrDataOffset = ( 0x16 - nvrAddressFirst );
static constexpr uint8_t sawbNvrDataOffset = ( 0x17 - nvrAddressFirst );
static constexpr uint8_t nvmTypeNvrDataOffset = ( 0x18 - nvrAddressFirst );
static constexpr uint8_t nvmSizeNvrDataOffset = ( 0x19 - nvrAddressFirst );
static constexpr uint8_t nvmPageSizeNvrDataOffset = ( 0x1B - nvrAddressFirst );
static constexpr uint8_t uuidNvrDataOffset = ( 0x1D - nvrAddressFirst );
static constexpr uint8_t uuidNvrDataSize = 0x10;
static constexpr uint8_t vidNvrDataOffset = ( 0x2D - nvrAddressFirst );
static constexpr uint8_t pidNvrDataOffset = ( 0x2F - nvrAddressFirst );
static constexpr uint8_t txcal1NvrDataOffset = ( 0x31 - nvrAddressFirst );
static constexpr uint8_t txcal2NvrDataOffset = ( 0x32 - nvrAddressFirst );

// NVR revision 0.2 = NVR revision 0.1 data + public key + private key
static constexpr uint8_t pukNvrDataOffset = ( 0x33 - nvrAddressFirst );
static constexpr uint8_t dskNvrDataOffset = pukNvrDataOffset;
static constexpr uint8_t pukNvrDataSize = 0x20;
static constexpr uint8_t dskNvrDataSize = pukNvrDataSize / 2;
static constexpr uint8_t prkNvrDataOffset = ( 0x53 - nvrAddressFirst );
static constexpr uint8_t prkNvrDataSize = 0x20;

// NVR common
static constexpr uint8_t crc16HighNvrDataOffset = ( 0x7E - nvrAddressFirst );
static constexpr uint8_t crc16LowNvrDataOffset = ( 0x7F - nvrAddressFirst );
static constexpr uint8_t hwVersionNvrDataOffset = ( 0x7F - nvrAddressFirst );
static constexpr size_t nvrAddressLast = 0xff;

// NVR revisions
static constexpr uint8_t nvrRevision_v01 = 0x01;
static constexpr uint8_t nvrRevision_v02 = 0x02;

void resetAssert();
void resetDeAssert();
void resetCycle( size_t usDelayAfter = 5000 );

}  // namespace Zwave
}  // namespace Environment
}  // namespace HubPlatform
