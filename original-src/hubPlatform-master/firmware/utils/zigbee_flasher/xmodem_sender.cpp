#include "zigbee_flasher/xmodem_sender.h"

#include "base_utils/timeutils.h"

#include <iostream>
#include <fstream>
#include <cstring>
#include <errno.h>

/*
 * Implementation relies on:
 * http://web.mit.edu/6.115/www/amulet/xmodem.htm
 */

namespace HubPlatform {
namespace ZigbeeFlasher {
namespace {

static constexpr uint8_t StartOfHeader = 0x01;
static constexpr uint8_t EndOfTransmission = 0x04;
static constexpr uint8_t Acknowledge = 0x06;
static constexpr uint8_t NotAcknowledge = 0x15;
static constexpr uint8_t EndOfTransmissionBlock = 0x17;
static constexpr uint8_t Cancel = 0x18;
static constexpr uint8_t AsciiC = 0x43;

/*
 * CRC implementation is borrowed from Silabs bootloader sources:
 * ${SDK_PATH}/gecko_sdk_suite/v2.7/platform/bootloader/plugin/security/btl_crc16.c
 */
uint16_t btl_crc16( const uint8_t newByte, uint16_t prevResult ) {
    prevResult = ( prevResult >> 8 ) | ( prevResult << 8 );
    prevResult ^= newByte;
    prevResult ^= ( prevResult & 0xff ) >> 4;
    prevResult ^= ( prevResult << 8 ) << 4;

    prevResult ^= ( ( uint8_t )( ( uint8_t )( ( uint8_t )( prevResult & 0xff ) ) << 5 ) ) |
                  ( ( uint16_t )( ( uint8_t )( ( uint8_t )( prevResult & 0xff ) ) >> 3 ) << 8 );

    return prevResult;
}

uint16_t btl_crc16Stream( const uint8_t* buffer, size_t length, uint16_t prevResult ) {
    for( size_t position = 0; position < length; position++ ) {
        prevResult = btl_crc16( buffer[ position ], prevResult );
    }
    return prevResult;
}

}  // namespace

XmodemSender::XmodemSender( Uart& uart )
: m_terminate( false )
, m_uart( uart ) {}

void XmodemSender::terminate() {
    m_terminate = true;
}

int XmodemSender::sendFile( const std::string& filepath ) {
    waitForTransmission();
    std::ifstream fwFile( filepath, std::ios::binary | std::ios::ate );
    const size_t fwFileSize = fwFile.tellg();
    fwFile.seekg( std::ios::beg );

    std::cout << "Firmware file size: " << fwFileSize << " bytes" << std::endl;
    // packet number must 1 for initiating transaction
    size_t packetNumber = 1;
    size_t fileOffset = 0;

    uint8_t dataBuffer[ 128 ];
    while( !m_terminate && fileOffset < fwFileSize ) {
        std::memset( (void*) dataBuffer, 0xFF, sizeof( dataBuffer ) );
        const size_t bytesLeft = fwFileSize - fwFile.tellg();
        const size_t bytesToRead = ( bytesLeft >= sizeof( dataBuffer ) ) ? sizeof( dataBuffer ) : bytesLeft;
        fwFile.read( (char*) dataBuffer, bytesToRead );

        if( sendPacket( packetNumber, dataBuffer, sizeof( dataBuffer ) ) ) {
            ++packetNumber;
            fileOffset += bytesToRead;
            printProgress( "Firmware transfer", fileOffset, fwFileSize );
        }
        else {
            std::cerr << "Send packet failed after maximum retries..." << std::endl;
            return EXIT_FAILURE;
        }
    }
    std::cout << std::endl;
    std::cout << "File transfer completed: " << packetNumber << " packets sent, " << fileOffset << " bytes transferred"
              << std::endl;
    sendEndOfTransmission();
    return 0;
}

bool XmodemSender::sendPacket( uint8_t packetNumber, uint8_t* pData, size_t dataSize ) {
    // StartOfHeader + PacketNumber + ~PacketNumber + Data + CRC = 1 + 1 + 1 + 128 + 2 = 133 octets
    uint8_t packetBuffer[ 133 ] = { 0 };
    packetBuffer[ 0 ] = StartOfHeader;
    packetBuffer[ 1 ] = packetNumber;
    packetBuffer[ 2 ] = ~packetNumber;
    memcpy( &packetBuffer[ 3 ], pData, dataSize );
    // uint16_t crcValue = btl_crc16Stream( &packetBuffer[ 3 ], 128, 0 );
    uint16_t crcValue = calculateCrc( &packetBuffer[ 3 ], 128 );
    packetBuffer[ 131 ] = ( uint8_t )( crcValue >> 8 );
    packetBuffer[ 132 ] = (uint8_t) crcValue;

    bool result = false;
    size_t retriesNumber = 0;
    while( retriesNumber++ < 3 ) {
        m_uart.flush();
        auto writeResult = m_uart.write( packetBuffer, sizeof( packetBuffer ) );
        if( writeResult != sizeof( packetBuffer ) ) {
            std::cerr << "Send data packet failed: UART write failed, " << strerror( errno ) << std::endl;
            continue;
        }
        auto ackResult = waitForAcknoledgement();
        if( ackResult ) {
            result = true;
            break;
        }
    }
    return result;
}

bool XmodemSender::sendEndOfTransmission() {
    bool result = false;
    size_t retriesNumber = 0;
    while( retriesNumber++ < 3 ) {
        m_uart.flush();
        auto writeResult = m_uart.write( &EndOfTransmission, sizeof( EndOfTransmission ) );
        if( writeResult != sizeof( EndOfTransmission ) ) {
            std::cerr << "Send EOT failed: UART write failed, " << strerror( errno ) << std::endl;
            continue;
        }
        auto ackResult = waitForAcknoledgement();
        if( ackResult ) {
            std::cout << "Send EOT acknowledged" << std::endl;
            result = true;
            break;
        }
    }
    return result;
}

bool XmodemSender::sendEndOfTransmissionBlock() {
    bool result = false;
    size_t retriesNumber = 0;
    while( retriesNumber++ < 3 ) {
        m_uart.flush();
        auto writeResult = m_uart.write( &EndOfTransmission, sizeof( EndOfTransmission ) );
        if( writeResult != sizeof( EndOfTransmission ) ) {
            std::cerr << "Send EOT failed: UART write failed, " << strerror( errno ) << std::endl;
            continue;
        }
        auto ackResult = waitForAcknoledgement();
        if( ackResult ) {
            std::cout << "Send ETB acknowledged" << std::endl;
            result = true;
            break;
        }
    }
    return result;
}

bool XmodemSender::waitForAcknoledgement() {
    auto startTime = TimeUtils::getMonotonicTime();

    // 2 seconds timeout
    uint8_t readData;
    while( TimeUtils::getDiffMonotonicTime( startTime ) < 2 ) {
        auto readCount = m_uart.read( &readData, 1 );
        if( readCount > 0 ) {
            if( readData == Acknowledge ) {
                return true;
            }
            else if( readData == NotAcknowledge ) {
                std::cout << "NotAcknowledge received" << std::endl;
                return false;
            }
            else {
                std::cout << "Received: " << std::to_string( readData ) << std::endl;
            }
        }
    }
    return false;
}

bool XmodemSender::waitForTransmission() {
    while( !m_terminate ) {
        uint8_t readSymbol = 0;
        auto dataCount = m_uart.read( (void*) &readSymbol, 1 );
        if( dataCount > 0 ) {
            if( readSymbol == AsciiC ) {
                return true;
            }
            else {
                std::cout << readSymbol;
            }
        }
    }
    return false;
}

void XmodemSender::printProgress( const char* message, size_t done, size_t max ) {
    static char progressMessage[ 128 ];
    memset( (void*) progressMessage, 0, sizeof( progressMessage ) );
    progressMessage[ 0 ] = '\r';
    strcpy( progressMessage + 1, message );
    float percentValue = ( (float) done * 100.0f / max );
    ( percentValue > 100.0f ? percentValue = 100.0f : percentValue = percentValue );
    snprintf( progressMessage + strlen( message ) + 1, sizeof( progressMessage ), ": %3.1f %% done", percentValue );
    std::cout << progressMessage;
    std::cout.flush();
}

uint16_t XmodemSender::calculateCrc( uint8_t* pData, size_t dataSize ) {
    const uint16_t crcSeed = 0x1021;
    uint16_t crc = 0;
    uint8_t i;

    while( dataSize-- > 0 ) {
        crc = crc ^ (uint16_t) *pData << 8;
        ++pData;
        i = 8;
        do {
            if( crc & 0x8000 ) {
                crc = crc << 1 ^ crcSeed;
            }
            else {
                crc = crc << 1;
            }
        } while( --i );
    }
    return ( crc );
}

}  // namespace ZigbeeFlasher
}  // namespace HubPlatform
