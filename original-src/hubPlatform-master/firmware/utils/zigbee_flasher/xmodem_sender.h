#pragma once

#include "base_utils/uart/uart.h"

#include <string>
#include <atomic>

namespace HubPlatform {
namespace ZigbeeFlasher {

class XmodemSender {
public:
    XmodemSender() = delete;
    explicit XmodemSender( Uart& uart );
    ~XmodemSender() = default;

public:
    void terminate();
    int sendFile( const std::string& filepath );

private:
    bool waitForTransmission();
    bool waitForAcknoledgement();
    bool sendPacket( uint8_t packetNumber, uint8_t* pData, size_t dataSize );
    bool sendEndOfTransmission();
    bool sendEndOfTransmissionBlock();
    uint16_t calculateCrc( uint8_t* pData, size_t dataSize );
    void printProgress( const char* message, size_t done, size_t max );

private:
    std::atomic_bool m_terminate;
    Uart& m_uart;
};

}  // namespace ZigbeeFlasher
}  // namespace HubPlatform
