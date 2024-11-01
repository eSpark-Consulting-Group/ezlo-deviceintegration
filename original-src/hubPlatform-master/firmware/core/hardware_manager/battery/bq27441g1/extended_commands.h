#pragma once

#include <cstdint>
#include <cstring>

namespace HubPlatform {
namespace HardwareManager {
namespace Bq27441g1 {

enum class ExtendedCommand : uint8_t {
    //    OpConfig = 0x3A3B, // This command is similar to standard commands, so it was moved there
    //    DesignCapacity = 0x3C3D, // This command is similar to standard commands, so it was moved there
    DataClass = 0x3E,          // N/A when SEALSED, RW when UNSEALED
    DataBlock = 0x3F,          // N/A when SEALSED, RW when UNSEALED
    BlockData = 0x40,          // RO when SEALED, RW when UNSEALED
    BlockDataCheckSum = 0x60,  // N/A when SEALSED, RW when UNSEALED
    BlockDataControl = 0x61,   // N/A when SEALSED, RW when UNSEALED
    Reserved = 0x62            // RO
};

}  // namespace Bq27441g1
}  // namespace HardwareManager
}  // namespace HubPlatform
