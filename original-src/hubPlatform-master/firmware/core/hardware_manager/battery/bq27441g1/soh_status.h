#pragma once

#include <cstdint>

namespace HubPlatform {
namespace HardwareManager {
namespace Bq27441g1 {

enum class SOHStatus : uint8_t {
    SOHNotValid = 0x00,
    InstantSOHValueReady = 0x01,
    InitialSOHValueReady = 0x02,
    SOHValueReady = 0x03,
    Reserved = 0x04
};

}  // namespace Bq27441g1
}  // namespace HardwareManager
}  // namespace HubPlatform
