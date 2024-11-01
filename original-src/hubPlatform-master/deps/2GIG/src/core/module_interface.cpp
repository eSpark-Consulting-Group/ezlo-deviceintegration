#include "core/module_interface.h"

#include "utils/serial_port/serial_port_impl.h"
#include "utils/serial_port/serial_port_configuration.h"

#include <fstream>
#include <thread>

namespace TwoGig {

ModuleInterface::ModuleInterface( const std::string& devicePath, const std::string& resetPinGpio )
: m_devicePath( devicePath )
, m_resetPinGpio( resetPinGpio ) {}

std::unique_ptr< SerialPort > ModuleInterface::createSerialPort() {
    constexpr ConfigurationSettings settings{ .baudRate = BaudRate::Baud_19200,
                                              .parity = Parity::Odd,
                                              .characterSize = CharacterSize::CS_8,
                                              .stopBitsCount = StopBitsCount::One,
                                              .hardwareFlowControlEnabled = false };

    return std::unique_ptr< SerialPort >( new SerialPortImpl( m_devicePath, settings ) );
}

void ModuleInterface::enable() {
    std::ofstream resetFile( m_resetPinGpio, std::ofstream::binary );

    resetFile << "1" << std::flush;
}

void ModuleInterface::disable() {
    std::ofstream resetFile( m_resetPinGpio, std::ofstream::binary );

    resetFile << "0" << std::flush;
}

void ModuleInterface::reenable() {
    std::ofstream resetFile( m_resetPinGpio, std::ofstream::binary );

    resetFile << "0" << std::flush;
    std::this_thread::sleep_for( std::chrono::milliseconds( 50 ) );
    resetFile << "1" << std::flush;
}

}  // namespace TwoGig
