#include "bluetooth/exceptions/bluetooth_manager_initialization_failed.h"

namespace HubPlatform {

const char* BluetoothManagerInitializationFailed::what() const noexcept {
    return "Bluetooth manager initialization failed";
}

}  // namespace HubPlatform
