#include "base_utils/zwave/zwave_serial_api.h"
#include "base_utils/zwave/zwave_environment.h"

#include "base_utils/timeutils.h"
#include "base_utils/utils.h"

#include <thread>
#include <sstream>
#include <iomanip>
#include <arpa/inet.h>

namespace HubPlatform {
namespace {

static constexpr uint8_t SerialApiSOF = 0x01;
static constexpr uint8_t SerialApiACK = 0x06;
static constexpr uint8_t SerialApiNACK = 0x15;
static constexpr uint8_t SerialApiRequest = 0x00;
static constexpr uint8_t SerialApiResponse = 0x01;
static constexpr uint8_t SerialApiFcStartedCode = 0x0A;
static constexpr uint8_t SerialApiFcGetHomeId = 0x20;
static constexpr uint8_t SerialApiFcGetNvr = 0x28;
static constexpr uint8_t SerialApiFcSendNif = 0x12;
static constexpr uint8_t SerialApiFcSetDefault = 0x42;

}  // namespace

const volatile sig_atomic_t ZwaveSerialApi::DefaultExitSignal = 0;

ZwaveSerialApi::ZwaveSerialApi( const std::string& serialPortDevice )
: m_uart( serialPortDevice, true )
, m_exitSignal( DefaultExitSignal ) {}

ZwaveSerialApi::ZwaveSerialApi( const std::string& serialPortDevice, bool nonBlocking )
: m_uart( serialPortDevice, nonBlocking )
, m_exitSignal( DefaultExitSignal ) {}

ZwaveSerialApi::ZwaveSerialApi( const std::string& serialPortDevice,
                                bool nonBlocking,
                                const volatile sig_atomic_t& exitSignal )
: m_uart( serialPortDevice, nonBlocking )
, m_exitSignal( exitSignal ) {}

std::vector< uint8_t > ZwaveSerialApi::readFrame( const time_t timeoutSeconds, const uint8_t frameFuncClass ) {
    auto startTime = TimeUtils::getMonotonicTime();
    int32_t frameSofIndex = -1;
    size_t index = 0;
    std::vector< uint8_t > result;

    while( !m_exitSignal && TimeUtils::getDiffMonotonicTime( startTime ) < timeoutSeconds ) {
        auto dataCount = m_uart.read( (void*) &m_buffer[ index ], 1 );
        if( dataCount > 0 ) {
            index += dataCount;
        }

        // search for start of frame
        if( frameSofIndex < 0 ) {
            frameSofIndex = searchSof( m_buffer, index );
        }
        else {
            // we have start of frame - wait for full frame length
            if( index < frameSofIndex + m_buffer[ frameSofIndex + 1 ] + 1 ) {
                continue;
            }

            if( checkCrc( &m_buffer[ frameSofIndex ] ) ) {
                const uint8_t* frameBegin = &m_buffer[ frameSofIndex ];
                const size_t frameLength = frameBegin[ 1 ] + 2;
                if( frameFuncClass == 0xFF || m_buffer[ frameSofIndex + 3 ] == frameFuncClass ) {
                    result.resize( frameLength );
                    std::copy( frameBegin, frameBegin + frameLength - 1, result.begin() );
                    break;
                }
            }
        }
        std::this_thread::sleep_for( std::chrono::milliseconds( 2 ) );
    }

    return result;
}

int32_t ZwaveSerialApi::searchSof( const uint8_t* pBuffer, size_t length ) {
    int32_t result = -1;
    size_t searchIndex = 0;

    while( searchIndex < length ) {
        if( pBuffer[ searchIndex ] == SerialApiSOF && ( searchIndex + 1 ) < length ) {
            result = searchIndex;
            break;
        }
        ++searchIndex;
    }
    return result;
}

bool ZwaveSerialApi::checkCrc( const uint8_t* pFrame ) {
    uint8_t crcValue = calculateCrc( pFrame );
    size_t frameLength = *( pFrame + 1 ) + 2;
    uint8_t frameCrc = pFrame[ frameLength - 1 ];
    const bool result = ( crcValue == frameCrc );
    return result;
}

uint8_t ZwaveSerialApi::calculateCrc( const uint8_t* pFrame ) {
    uint8_t crcValue = 0xff;
    size_t frameLength = *( pFrame + 1 ) + 2;
    for( size_t index = 1; index < frameLength - 1; ++index ) {
        crcValue ^= pFrame[ index ];
    }
    return crcValue;
}

bool ZwaveSerialApi::waitSerialApiStarted( const time_t timeoutSeconds ) {
    int triesCount = 0;

    bool result = false;

    while( triesCount < 3 ) {
        ++triesCount;
        memset( m_buffer, 0, sizeof( m_buffer ) );

        if( m_uart.isInitialized() ) {
            m_uart.flush();
            Environment::Zwave::resetCycle();
        }

        auto frame = readFrame( timeoutSeconds, SerialApiFcStartedCode );
        if( !frame.empty() ) {
            sendAck();
            result = true;
            break;
        }
    }

    return result;
}

bool ZwaveSerialApi::getDsk( std::string& outResult ) {
    bool result = false;
    outResult = "";
    constexpr uint8_t DskOffsetBytes = 0x23;
    constexpr uint8_t PukSizeBytes = 32;
    constexpr uint8_t DskSizeBytes = PukSizeBytes / 2;
    constexpr uint8_t const getDskRequest[ 7 ] = {
        SerialApiSOF, 0x05, SerialApiRequest, SerialApiFcGetNvr, DskOffsetBytes, PukSizeBytes, 0xD1
    };

    memset( m_buffer, 0, sizeof( m_buffer ) );
    m_uart.flush();
    m_uart.write( getDskRequest, sizeof( getDskRequest ) );

    auto frame = readFrame( 2, SerialApiFcGetNvr );
    if( !frame.empty() ) {
        sendAck();
        uint16_t* pDskStart = (uint16_t*) &frame[ 4 ];
        std::stringstream ss;
        // DSK key is first 16 bytes from public key
        for( uint8_t index = 0; index < DskSizeBytes; index += 2 ) {
            ss << std::setw( 5 ) << std::setfill( '0' ) << htons( *pDskStart ) << "-";
            ++pDskStart;
        }
        outResult = ss.str();
        outResult.pop_back();
        result = true;
    }

    return result;
}

bool ZwaveSerialApi::getHomeId( std::string& outResult ) {
    bool result = false;
    outResult = "";
    constexpr uint8_t const getHomeIdRequest[ 5 ] = {
        SerialApiSOF, 0x03, SerialApiRequest, SerialApiFcGetHomeId, 0xDC
    };

    memset( m_buffer, 0, sizeof( m_buffer ) );
    m_uart.flush();
    m_uart.write( getHomeIdRequest, sizeof( getHomeIdRequest ) );

    auto frame = readFrame( 2, SerialApiFcGetHomeId );
    if( !frame.empty() ) {
        sendAck();
        outResult = hexToString( &frame[ 4 ], 4, true );
        result = true;
    }

    return result;
}

void ZwaveSerialApi::sendAck() {
    m_uart.write( &SerialApiACK, 1 );
}

bool ZwaveSerialApi::sendSetDefault() {
    size_t retriesNumber = 0;
    constexpr uint8_t const sendSetDefaultRequest[ 6 ] = { SerialApiSOF,          0x04, SerialApiRequest,
                                                           SerialApiFcSetDefault, 0x01, 0xb8 };
    while( !m_exitSignal && retriesNumber < 3 ) {
        ++retriesNumber;
        m_uart.flush();
        m_uart.write( sendSetDefaultRequest, sizeof( sendSetDefaultRequest ) );
        if( waitSetDefaultResponse() ) {
            return true;
        }
        std::this_thread::sleep_for( std::chrono::milliseconds( 250 ) );
    }
    return false;
}

bool ZwaveSerialApi::sendNif() {
    size_t retriesNumber = 0;
    size_t sentNifsCount = 0;
    constexpr uint8_t const sendNifRequest1[ 8 ] = {
        SerialApiSOF, 0x06, SerialApiRequest, SerialApiFcSendNif, 0xFF, 0x10, 0x01, 0x05
    };
    std::cout << "Start sending NIF" << std::endl;
    while( !m_exitSignal && retriesNumber < 5 ) {
        ++retriesNumber;
        m_uart.flush();
        m_uart.write( sendNifRequest1, sizeof( sendNifRequest1 ) );
        if( waitAck() ) {
            if( waitSendNifResponse() && waitSendNifRequest() ) {
                waitAck();
                ++sentNifsCount;
                std::cout << "NIF-" << sentNifsCount << " sent !" << std::endl << std::flush;
            }
        }
        std::this_thread::sleep_for( std::chrono::milliseconds( 300 ) );
    }
    return ( retriesNumber == sentNifsCount );
}

bool ZwaveSerialApi::waitSetDefaultResponse() {
    memset( m_buffer, 0, sizeof( m_buffer ) );

    auto frame = readFrame( 6, SerialApiFcSetDefault );
    if( !frame.empty() ) {
        sendAck();
        return true;
    }
    return false;
}

bool ZwaveSerialApi::waitAck() {
    auto startTime = TimeUtils::getMonotonicTime();
    uint8_t buffer;
    while( !m_exitSignal && TimeUtils::getDiffMonotonicTime( startTime ) < 2 ) {
        auto readCount = m_uart.read( &buffer, sizeof( buffer ) );
        if( readCount > 0 && buffer == SerialApiACK ) {
            return true;
        }
        std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
    }
    return false;
}

bool ZwaveSerialApi::waitSendNifResponse() {
    memset( m_buffer, 0, sizeof( m_buffer ) );

    auto frame = readFrame( 2, SerialApiFcSendNif );
    if( !frame.empty() ) {
        sendAck();
        return true;
    }
    return false;
}

bool ZwaveSerialApi::waitSendNifRequest() {
    memset( m_buffer, 0, sizeof( m_buffer ) );

    auto frame = readFrame( 2, SerialApiFcSendNif );
    if( !frame.empty() ) {
        sendAck();
        return true;
    }
    return false;
}

}  // namespace HubPlatform
