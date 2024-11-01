#pragma once

#include "utils/serial_port/serial_port.h"

#include <memory>

namespace TwoGig {

struct ModuleInterface {
    ModuleInterface( const std::string& devicePath, const std::string& resetPinGpio );

    std::unique_ptr< SerialPort > createSerialPort();

    void enable();
    void disable();
    void reenable();

private:
    std::string m_devicePath;
    std::string m_resetPinGpio;
};

}  // namespace TwoGig
