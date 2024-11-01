#include "hardware_manager/utils/battery.h"

#include "system_utils/system.h"

#include <tuple>

namespace HubPlatform {
namespace HardwareManager {
namespace Utils {

namespace {
int I2cBusNumber = 1;
int I2cChipAddress = 0x55;
}  // namespace

bool hasBatteryFuelGaugeMeter() {
#if defined( __HUB_TYPE_H2_SECURE__ )
    int result;
    std::tie( std::ignore, result ) = System::getCommandOutput(
        ( "i2cget -y " + std::to_string( I2cBusNumber ) + " " + std::to_string( I2cChipAddress ) ).c_str() );
    return result == 0;
#else
    return false;
#endif
}

}  // namespace Utils
}  // namespace HardwareManager
}  // namespace HubPlatform
