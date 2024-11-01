#pragma once

#include "utils/serial_port/serial_port.h"
#include "utils/serial_port/serial_port_configuration.h"

#include <cstdint>
#include <string>

#include <termios.h>

namespace TwoGig {

class SerialPortImpl : public SerialPort {
public:
    SerialPortImpl( const std::string& devicePath, const ConfigurationSettings& settings );

    bool open() override;
    void close() override;

    bool readByte( uint8_t& buffer ) const override;

private:
    bool configure() const;

    bool saveCurrentConfiguration();
    bool loadPreviousConfiguration() const;

    void reset();

private:
    std::string m_devicePath;
    ConfigurationSettings m_settings;

    int m_fileDescriptor;

    termios m_previousConfig;
};

}  // namespace TwoGig
