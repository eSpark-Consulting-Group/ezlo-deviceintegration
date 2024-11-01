#pragma once

#include "base_utils/spi/envm_transport.h"

#include <iostream>
#include <fstream>

class SpiSoftSercomm : public EnvmTransportInterface {
public:
    int32_t init( uint32_t spiSpeed, const bool keepCSActivated = true ) override;
    void deinit() override;
    int sendRequest( const std::vector< uint8_t >& command,
                     int32_t resultSize,
                     std::vector< uint8_t >& outResult ) override;
    bool isValid() const override;

private:
    uint8_t sendMsbFirst( const uint8_t dataByte );
    uint8_t sendBit( const bool dataBit );

private:
    void initPins();
    void deInitPins();
    void setPinInput( int pinNumber );
    void setPinOutput( int pinNumber );
    void setPin( int pinNumber );
    void clearPin( int pinNumber );
    bool readPin( int pinNumber );

private:
    static constexpr int m_bufferSize = 2048;

    unsigned char m_txBuffer[ m_bufferSize ];
    unsigned char m_rxBuffer[ m_bufferSize ];
};
