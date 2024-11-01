#pragma once

#include <cstdint>

namespace HubPlatform {
namespace HardwareManager {
namespace Bq27441g1 {

enum class StandardCommand : uint16_t {
    Control = 0x0001,
    Temperature = 0x0203,
    Voltage = 0x0405,
    Flags = 0x0607,
    NominalAvailableCapacity = 0x0809,
    FullAvailableCapacity = 0x0A0B,
    RemainingCapacity = 0x0C0D,
    FullChargeCapacity = 0x0E0F,
    AverageCurrent = 0x1011,
    StandbyCurrent = 0x1213,
    MaxLoadCurrent = 0x1415,
    AveragePower = 0x1819,
    StateOfCharge = 0x1C1D,
    InternalTemperature = 0x1E1F,
    StateOfHealth = 0x2021,
    RemainingCapacityUnfiltered = 0x2829,
    RemainingCapacityFiltered = 0x2A2B,
    FullChargeCapacityUnfiltered = 0x2C2D,
    FullChargeCapacityFlitered = 0x2E2F,
    StateOfChargeUnfiltered = 0x3031,
    OpConfig = 0x3A3B,        // RO
    DesignCapacity = 0x3C3D,  // RO

    CNTL = Control,
    TEMP = Temperature,
    VOLT = Voltage,
    FLAGS = Flags,
    RM = RemainingCapacity,
    FCC = FullChargeCapacity,
    SOC = StateOfCharge,
    SOH = StateOfHealth
};

}  // namespace Bq27441g1
}  // namespace HardwareManager
}  // namespace HubPlatform
