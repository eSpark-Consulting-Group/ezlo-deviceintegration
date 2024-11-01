#pragma once

#include "base_utils/uart/uart.h"

#include <vector>
#include <csignal>

namespace HubPlatform {

class ZwaveSerialApi final {
public:
    ZwaveSerialApi() = delete;
    explicit ZwaveSerialApi( const std::string& serialPortDevice );
    explicit ZwaveSerialApi( const std::string& serialPortDevice, bool nonBlocking );
    explicit ZwaveSerialApi( const std::string& serialPortDevice,
                             bool nonBlocking,
                             const volatile sig_atomic_t& exitSignal );
    ~ZwaveSerialApi() = default;

public:
    bool waitSerialApiStarted( const time_t timeoutSeconds = 10 );
    bool getDsk( std::string& outResult );
    bool getHomeId( std::string& outResult );
    bool sendSetDefault();
    bool sendNif();

private:
    std::vector< uint8_t > readFrame( const time_t timeoutSeconds, const uint8_t frameFuncClass );
    int32_t searchSof( const uint8_t* pBuffer, size_t length );
    bool checkCrc( const uint8_t* pFrame );
    uint8_t calculateCrc( const uint8_t* pFrame );

private:
    void sendAck();
    bool waitSetDefaultResponse();
    bool waitAck();
    bool waitSendNifResponse();
    bool waitSendNifRequest();

public:
    static const volatile sig_atomic_t DefaultExitSignal;

private:
    uint8_t m_buffer[ 1024 ];
    Uart m_uart;
    const volatile sig_atomic_t& m_exitSignal;
};

}  // namespace HubPlatform
