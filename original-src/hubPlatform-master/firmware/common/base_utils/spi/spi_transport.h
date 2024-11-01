#pragma once

#include "base_utils/spi/envm_transport.h"
#include "base_utils/spi/actual_transport.h"
#include "base_utils/hubtypes.h"

#include <iostream>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include <vector>
#include <sstream>
#include <iomanip>

class SpiTransport : public EnvmTransportInterface {
public:
    int32_t init( uint32_t spiSpeed, const bool keepCSActivated = true ) override;
    void deinit() override;
    int sendRequest( const std::vector< uint8_t >& command,
                     int32_t resultSize,
                     std::vector< uint8_t >& outResult ) override;
    bool isValid() const override;

private:
    static constexpr int m_bufferSize = 2048;

    bool m_isValid = false;
    unsigned char m_txBuffer[ m_bufferSize ];
    unsigned char m_rxBuffer[ m_bufferSize ];
    struct spi_ioc_transfer m_xfer[ 2 ];
};
