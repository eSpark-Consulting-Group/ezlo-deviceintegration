#pragma once

#include <cstdint>

namespace HubPlatform {
namespace HardwareManager {
namespace Bq27441g1 {

enum class ControlFunction : uint16_t {
    CONTROL_STATUS = 0x0000,
    DEVICE_TYPE = 0x0001,
    FW_VERSION = 0x0002,
    DM_CODE = 0x0004,
    PREV_MACWRITE = 0x0007,
    CHEM_ID = 0x0008,
    BAT_INSERT = 0x000C,
    BAT_REMOVE = 0x000D,
    SET_HIBERNATE = 0x0011,
    CLEAR_HIBERNATE = 0x0012,
    SET_CFGUPDATE = 0x0013,    // Only if UNSEALED
    SHUTDOWN_ENABLE = 0x001B,  // Only if UNSEALED
    SHUTDOWN = 0x001C,         // Only if UNSEALED
    SEALED = 0x0020,           // Only if UNSEALED
    TOGGLE_GPOUT = 0x0023,
    RESET = 0x0041,           // Only if UNSEALED
    SOFT_RESET = 0x0042,      // Only if UNSEALED
    EXIT_CFGUPDATE = 0x0043,  // Only if UNSEALED
    EXIT_RESIM = 0x0044       // Only if UNSEALED
};

}  // namespace Bq27441g1
}  // namespace HardwareManager
}  // namespace HubPlatform
