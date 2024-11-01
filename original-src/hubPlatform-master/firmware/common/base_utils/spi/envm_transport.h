#pragma once
#include "base_utils/hubtypes.h"

#include <iostream>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include <vector>
#include <sstream>
#include <iomanip>

#define DEBUG_TRANSPORT false
#if( DEBUG_TRANSPORT )
#define debug_out( message )                        \
    do {                                            \
        if( DEBUG_TRANSPORT )                       \
            std::cout << ( #message ) << std::endl; \
    } while( 0 );
#else
#define debug_out( message ) /*#message*/
#endif

class EnvmTransportInterface {
public:
    EnvmTransportInterface() = default;
    virtual ~EnvmTransportInterface() = default;

public:
    int getDescriptor() const {
        return m_descriptor;
    };

public:
    virtual int32_t init( uint32_t speed, const bool keepCSActivated = true ) = 0;
    virtual void deinit() = 0;
    virtual int sendRequest( const std::vector< uint8_t >& command,
                             int32_t resultSize,
                             std::vector< uint8_t >& outResult ) = 0;
    virtual bool isValid() const = 0;

protected:
    int m_descriptor = -1;
};

class EnvmMockTransport : public EnvmTransportInterface {
public:
    int32_t init( uint32_t spiSpeed, const bool keepCSActivated = true ) override;
    void deinit() override;
    int sendRequest( const std::vector< uint8_t >& command,
                     int32_t resultSize,
                     std::vector< uint8_t >& outResult ) override;
    bool isValid() const override;
};
