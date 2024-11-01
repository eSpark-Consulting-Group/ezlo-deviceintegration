#include "base_utils/spi/spi_soft_sercomm.h"

#include "base_utils/spi/sercomm.h"
#include <base_utils/utils.h>

#include <string>
#include <unistd.h>
#include <string.h>

int32_t SpiSoftSercomm::init( uint32_t /* spiSpeed */, const bool /*keepCSActivated*/ ) {
    debug_out( "SpiSoftSercomm::init()" );
    m_descriptor = -1;

    initPins();

    return 0;
}

void SpiSoftSercomm::deinit() {
    debug_out( "SpiSoftSercomm::deinit()" );
    close( m_descriptor );
    m_descriptor = -1;

    deInitPins();
}

int SpiSoftSercomm::sendRequest( const std::vector< uint8_t >& command,
                                 int32_t resultSize,
                                 std::vector< uint8_t >& outResult ) {
    std::vector< uint8_t > inputBuffer;
    initPins();

    m_descriptor = Sercomm::openGpioDevice( m_descriptor );
    if( m_descriptor < 0 ) {
        return -1;
    }

    clearPin( Sercomm::CsPinNumber );

    for( size_t index = 0; index < command.size() + resultSize; index++ ) {
        auto item = ( index < command.size() ? command[ index ] : 0xFF );
        uint8_t readResult = sendMsbFirst( item );
        inputBuffer.push_back( readResult );
    }
    setPin( Sercomm::CsPinNumber );

    outResult.clear();

    if( resultSize == 0 ) {
        outResult.resize( command.size() );
        inputBuffer.resize( command.size() );
        std::copy( inputBuffer.begin(), inputBuffer.end(), outResult.begin() );
    }
    else {
        outResult.resize( resultSize );
        std::copy( inputBuffer.begin() + command.size(),
                   inputBuffer.begin() + command.size() + resultSize,
                   outResult.begin() );
    }

    debug_out( "sendRequest: " << hexToString( &command[ 0 ], command.size() ) );
    debug_out( "resultSize: " << resultSize );
    debug_out( "outResult: " << hexToString( &outResult[ 0 ], outResult.size() ) );

    return resultSize;
}

bool SpiSoftSercomm::isValid() const {
    return true;
}

uint8_t SpiSoftSercomm::sendMsbFirst( const uint8_t dataByte ) {
    uint8_t byteResult = 0;
    for( uint8_t mask = 0x80; mask > 0; ) {
        auto bitRead = sendBit( ( ( dataByte & mask ) != 0 ) );
        byteResult = byteResult << 1;
        byteResult |= bitRead;
        mask = mask >> 1;
    }

    debug_out( "send byte: " << hexToString( &dataByte, 1 ) );
    debug_out( "read byte: " << hexToString( &byteResult, 1 ) );

    return byteResult;
}

uint8_t SpiSoftSercomm::sendBit( const bool dataBit ) {
    clearPin( Sercomm::SckPinNumber );
    ( dataBit ? setPin( Sercomm::MosiPinNumber ) : clearPin( Sercomm::MosiPinNumber ) );
    usleep( 1 );
    setPin( Sercomm::SckPinNumber );
    usleep( 1 );
    const bool result = ( readPin( Sercomm::MisoPinNumber ) ? 1 : 0 );
    clearPin( Sercomm::SckPinNumber );

    return result;
}

void SpiSoftSercomm::initPins() {
    m_descriptor = Sercomm::openGpioDevice( m_descriptor );
    if( m_descriptor < 0 ) {
        return;
    }

    setPinOutput( Sercomm::SckPinNumber );
    setPinOutput( Sercomm::MosiPinNumber );
    setPinOutput( Sercomm::CsPinNumber );
    setPinInput( Sercomm::MisoPinNumber );
}

void SpiSoftSercomm::deInitPins() {
    m_descriptor = Sercomm::openGpioDevice( m_descriptor );
    if( m_descriptor < 0 ) {
        return;
    }

    setPinInput( Sercomm::SckPinNumber );
    setPinInput( Sercomm::MosiPinNumber );
    setPinInput( Sercomm::CsPinNumber );
    setPinInput( Sercomm::MisoPinNumber );
}

void SpiSoftSercomm::setPinInput( int pinNumber ) {
    Sercomm::gpioDirectionIn( pinNumber, m_descriptor );
}

void SpiSoftSercomm::setPinOutput( int pinNumber ) {
    Sercomm::gpioDirectionOut( pinNumber, m_descriptor );
}

void SpiSoftSercomm::setPin( int pinNumber ) {
    Sercomm::gpioWriteValue( pinNumber, 1, m_descriptor );
}

void SpiSoftSercomm::clearPin( int pinNumber ) {
    Sercomm::gpioWriteValue( pinNumber, 0, m_descriptor );
}

bool SpiSoftSercomm::readPin( int pinNumber ) {
    return ( Sercomm::gpioReadValue( pinNumber, m_descriptor ) > 0 );
}
