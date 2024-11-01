#include "base_utils/zwave/efr32zg14_interface.h"
#include "base_utils/spi/sercomm.h"

#include <fstream>
#include <unistd.h>
#include <string.h>
#include <unordered_map>

namespace HubPlatform {

using namespace Environment::Zwave;

Efr32Zg14::Efr32Zg14()
: m_serialApi( Environment::getZwaveDevice(), true ) {}

Efr32Zg14::~Efr32Zg14() {}

bool Efr32Zg14::setProgrammingOn() {
    return true;
}

bool Efr32Zg14::setProgrammingOff() {
    hwResetDeassert();
    usleep( 100000 );

    hwResetAssert();
    usleep( 100000 );

    hwResetDeassert();
    return true;
}

int32_t Efr32Zg14::init( uint32_t speed, const bool keepCSActivated ) {
    return 0;
}

void Efr32Zg14::deinit() {}

int Efr32Zg14::getDescriptor() const {
    return -1;
}

int Efr32Zg14::sendRequest( const std::vector< uint8_t >& command,
                            int32_t resultSize,
                            std::vector< uint8_t >& outResult ) {
    outResult.clear();
    return outResult.size();
}

bool Efr32Zg14::isValid() const {
    return true;
}

const std::string& Efr32Zg14::getName() const {
    static const std::string name = "EFR32ZG14";
    return name;
}

bool Efr32Zg14::isSupported() {
    return m_serialApi.waitSerialApiStarted();
}

bool Efr32Zg14::readID( ChipMemoryVector& outResult ) {
    outResult.clear();
    std::cout << __PRETTY_FUNCTION__ << " is not supported" << std::endl;
    return true;
}

bool Efr32Zg14::readFlashCrc( ChipMemoryVector& outResult ) {
    outResult.clear();
    std::cout << __PRETTY_FUNCTION__ << " is not supported" << std::endl;
    return true;
}

bool Efr32Zg14::checkFlashCrc() {
    std::cout << __PRETTY_FUNCTION__ << " is not supported" << std::endl;
    return true;
}

size_t Efr32Zg14::readFlash( ChipMemoryVector& outResult ) {
    outResult.clear();
    std::cout << __PRETTY_FUNCTION__ << " is not supported" << std::endl;
    return outResult.size();
}

size_t Efr32Zg14::writeFlash( const ChipMemoryVector& data ) {
    std::cout << __PRETTY_FUNCTION__ << " is not supported" << std::endl;
    return data.size();
}

size_t Efr32Zg14::readNvr( ChipMemoryVector& outResult ) {
    outResult.clear();
    std::cout << __PRETTY_FUNCTION__ << " is not supported" << std::endl;
    return outResult.size();
}

size_t Efr32Zg14::writeNvr( const ChipMemoryVector& data ) {
    std::cout << __PRETTY_FUNCTION__ << " is not supported" << std::endl;
    return data.size();
}

uint16_t Efr32Zg14::calcNvrCrc( const ChipMemoryVector& data ) {
    // CRC16 value calculated from address 0x10 to address 0x7D ! ! !
    return ccittCrc16( &data[ 7 ], ( 0x7D - 0x10 + 1 ) );
}

const size_t Efr32Zg14::getNvrSize() const {
    std::cout << __PRETTY_FUNCTION__ << " is not supported" << std::endl;
    return 0;
}

void Efr32Zg14::hwResetCycle( size_t afterDelayUs ) {
    hwResetDeassert();
    usleep( resetDeAssertPeriodUs );

    hwResetAssert();
    usleep( resetAssertPeriodUs );

    hwResetDeassert();
    if( afterDelayUs ) {
        usleep( afterDelayUs );
    }
}

void Efr32Zg14::hwResetAssert() {
#if( defined( __HUB_TYPE_G150__ ) )
    auto fdGpioDevice = Sercomm::openGpioDevice();
    Sercomm::gpioDirectionOut( Sercomm::ZwaveRstPinNumber );
    Sercomm::gpioWriteValue( Sercomm::ZwaveRstPinNumber, 0, fdGpioDevice );
    close( fdGpioDevice );
#else
    std::ofstream resetFile( resetDeviceFilename, std::ofstream::binary );
    resetFile << resetAssertCmd;
    resetFile.flush();
#endif
}

void Efr32Zg14::hwResetDeassert() {
#if( defined( __HUB_TYPE_G150__ ) )
    auto fdGpioDevice = Sercomm::openGpioDevice();
    Sercomm::gpioDirectionOut( Sercomm::ZwaveRstPinNumber );
    Sercomm::gpioWriteValue( Sercomm::ZwaveRstPinNumber, 1, fdGpioDevice );
    close( fdGpioDevice );
#else
    std::ofstream resetFile( resetDeviceFilename, std::ofstream::binary );
    resetFile << resetDeassertCmd;
    resetFile.flush();
#endif
}

bool Efr32Zg14::bulkErase() {
    std::cout << __PRETTY_FUNCTION__ << " is not supported" << std::endl;
    return true;
}

std::string Efr32Zg14::nvrGetRegion( const std::vector< uint8_t >& nvrData ) {
    std::cout << __PRETTY_FUNCTION__ << " is not supported" << std::endl;
    return std::string();
}

void Efr32Zg14::nvrSetRegion( const std::string& newRegion, ChipMemoryVector& nvrData ) {
    std::cout << __PRETTY_FUNCTION__ << " is not supported" << std::endl;
}

std::string Efr32Zg14::nvrGetS2DskKey( const std::vector< uint8_t >& nvrData, std::vector< uint8_t >& outKey ) {
    std::string dskResult;
    m_serialApi.getDsk( dskResult );
    return dskResult;
}
void Efr32Zg14::nvrSetS2DskKey( const std::vector< uint8_t >& newKey, std::vector< uint8_t >& nvrData ) {
    std::cout << __PRETTY_FUNCTION__ << " is not supported" << std::endl;
}

uint32_t Efr32Zg14::nvrGetSawC( const ChipMemoryVector& nvrData ) {
    std::cout << __PRETTY_FUNCTION__ << " is not supported" << std::endl;
    return 0;
}

void Efr32Zg14::nvrSetSawC( uint32_t nvrSawCValue, ChipMemoryVector& nvrData ) {
    std::cout << __PRETTY_FUNCTION__ << " is not supported" << std::endl;
}

/*=========================== ccittCrc16 ============================
 * CRC-CCITT (0x1D0F) calculation / check
 *
 * In: byte string excluding 16bit check field
 * Out: CRC-16 value
 * or
 * In: byte string including 16bit check field
 * Out: zero when OK
 *
 * and
 * In: The crc input should be set to the initialization
 * value = 0x1D0F.
 * It can also be used to carry over crc value between separate
 * calculations of multiple parts of a frame, e.g. header and body.
 *
 *--------------------------------------------------------------------------*/
uint16_t Efr32Zg14::ccittCrc16( const uint8_t* pData, uint16_t dataLength ) {
    static constexpr uint16_t crcInitValue = 0x1D0F;
    static constexpr uint16_t crcPolynomSeed = 0x1021;

    uint16_t crc = crcInitValue;
    uint8_t tempData;
    uint8_t bitMask;
    uint8_t newBit;
    while( dataLength-- ) {
        tempData = *pData++;
        for( bitMask = 0x80; bitMask != 0; bitMask >>= 1 ) {
            newBit = ( ( tempData & bitMask ) != 0 ) ^ ( ( crc & 0x8000 ) != 0 );
            crc <<= 1;
            if( newBit ) {
                crc ^= crcPolynomSeed;
            }
        }
    }
    return crc;
}

}  // namespace HubPlatform