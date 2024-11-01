#include "base_utils/zwave/zm5101_interface.h"
#include "base_utils/zwave/zwave_environment.h"
#include "base_utils/spi/sercomm.h"

#include <fstream>
#include <unistd.h>
#include <string.h>
#include <unordered_map>

using namespace HubPlatform::Environment::Zwave;

namespace {
static constexpr int const CmdEraseDelayUs = 10000;  // 10 milliseconds = 10000 microseconds

void stopAt3FFs( std::vector< uint8_t > sectorData, size_t* stop, int fromPosition ) {
    const int untrackedFFsCount = 2;
    for( int i = fromPosition; i > untrackedFFsCount; i-- ) {
        if( ( sectorData[ i ] == 0xFF ) && ( sectorData[ i - 1 ] == 0xFF ) && ( sectorData[ i - 2 ] == 0xFF ) ) {
            *stop = i - 2;
        }
        else {
            break;
        }
    }
}

}  // namespace

Zm5101::Zm5101()
: m_cmdEnableInterface( { 0xAC, 0x53, 0xAA, 0x55 } )
, m_cmdCheckState( { 0x7F, 0xFE, 0x00 } )
, m_cmdReadFlash( { 0x10, 0x00, 0xFF } )
, m_cmdReadSram( { 0x06, 0x00, 0x00 } )
, m_cmdReadContinue( { 0xA0, 0x00, 0x00, 0x00 } )
, m_cmdWriteFlash( { 0x20, 0x00, 0xFF, 0xFF } )
, m_cmdWriteSram( { 0x04, 0x00, 0x00, 0x00 } )
, m_cmdWriteContinue( { 0x80, 0x00, 0x00, 0x00 } )
, m_cmdEraseSector( { 0x0B, 0x00, 0x00, 0x00 } )
, m_cmdEraseChip( { 0x0A, 0x00, 0x00, 0x00 } )
, m_cmdResetChip( { 0xFF, 0xFF, 0xFF, 0xFF } )
, m_cmdReadSignature( { 0x30, 0x00, 0xFF } )
, m_cmdEooSEnable( { 0xC0, 0x00, 0x00, 0x00 } )
, m_cmdEooSDisable( { 0xD0, 0xFF, 0xFF, 0xFF } )
, m_cmdSetLockBits( { 0xF0, 0x00, 0xFF, 0x00 } )
, m_cmdReadLockBits( { 0xF1, 0x00, 0xFF } )
, m_cmdSetNvr( { 0xFE, 0x00, 0x00, 0x00 } )
, m_cmdReadNvr( { 0xF2, 0x00, 0x00 } )
, m_cmdRunCrcCheck( { 0xC3, 0x00, 0x00, 0x00 } ) {}

Zm5101::~Zm5101() {}

bool Zm5101::setProgrammingOn() {
    ChipMemoryVector buffer;
    bool result = false;
    uint32_t retries = 0;

    while( retries < 10 ) {
        hwResetAssert();

        usleep( m_programmingOnTResetLowUs );

        init( m_maxFrequency );
        if( getDescriptor() < 0 ) {
            std::cerr << __PRETTY_FUNCTION__ << " failed: invalid descriptor" << std::endl;
            hwResetDeassert();
        }
        else {
            sendRequest( m_cmdEnableInterface.getCommand(), m_cmdEnableInterface.getResultSize(), buffer );
            if( buffer[ 2 ] == m_cmdEnableInterface.getCommand()[ 1 ] &&
                buffer[ 3 ] == m_cmdEnableInterface.getCommand()[ 2 ] ) {
                result = true;
                break;
            }
            else {
                hwResetDeassert();
            }
        }

        usleep( m_programmingOnTResetLowUs );
        ++retries;
    }
    if( !result ) {
        std::cerr << "Cannot set programming mode for ZW chip!" << std::endl;
    }
    return result;
}

bool Zm5101::setProgrammingOff() {
    deinit();
    hwResetDeassert();
    usleep( m_programmingOnTResetLowUs );

    hwResetAssert();
    usleep( m_programmingOnTResetLowUs );

    hwResetDeassert();
    return true;
}

int32_t Zm5101::init( uint32_t speed, const bool keepCSActivated ) {
    return m_transport.init( speed, keepCSActivated );
}

void Zm5101::deinit() {
    m_transport.deinit();
}

int Zm5101::getDescriptor() const {
    return m_transport.getDescriptor();
}

int Zm5101::sendRequest( const std::vector< uint8_t >& command,
                         int32_t resultSize,
                         std::vector< uint8_t >& outResult ) {
    return m_transport.sendRequest( command, resultSize, outResult );
}

bool Zm5101::isValid() const {
    return isValid();
}

const std::string& Zm5101::getName() const {
    static const std::string name = "ZM5101";
    return name;
}

bool Zm5101::isSupported() {
    auto result = setProgrammingOn();
    setProgrammingOff();
    return result;
}

bool Zm5101::readID( ChipMemoryVector& outResult ) {
    auto result = setProgrammingOn();

    if( !result ) {
        std::cerr << __PRETTY_FUNCTION__ << ": set programming mode FAILED" << std::endl;
        setProgrammingOff();
        return false;
    }

    outResult.clear();
    ChipMemoryVector buffer;
    ChipMemoryVector readCmd = m_cmdReadSignature.getCommand();
    for( size_t i = 0; i < m_signatureLength; ++i ) {
        readCmd[ 1 ] = i;
        sendRequest( readCmd, m_cmdReadSignature.getResultSize(), buffer );
        outResult.push_back( buffer[ 0 ] );
    }

    setProgrammingOff();
    return true;
}

bool Zm5101::readFlashCrc( ChipMemoryVector& outResult ) {
    auto result = setProgrammingOn();

    if( !result ) {
        std::cerr << __PRETTY_FUNCTION__ << ": set programming mode FAILED" << std::endl;
        setProgrammingOff();
        return false;
    }

    size_t totalBytesRead;
    ChipMemoryVector buffer;
    ChipMemoryVector lastSectorData;

    outResult.clear();
    if( !waitChipContinue() ) {
        return false;
    }
    outResult.resize( m_crcLength );

    // read last sector
    auto readCmd = m_cmdReadFlash.getCommand();
    readCmd[ 1 ] = m_flashTotalSectors - 1;
    totalBytesRead = sendRequest( readCmd, m_cmdReadFlash.getResultSize(), buffer );
    lastSectorData.push_back( buffer[ 0 ] );

    while( true ) {
        if( !waitChipContinue() ) {
            break;
        }
        sendRequest( m_cmdReadContinue.getCommand(), m_cmdReadContinue.getResultSize(), buffer );

        lastSectorData.push_back( buffer[ 1 ] );
        if( ++totalBytesRead >= m_flashSectorLength )
            break;
        lastSectorData.push_back( buffer[ 2 ] );
        if( ++totalBytesRead >= m_flashSectorLength )
            break;
        lastSectorData.push_back( buffer[ 3 ] );
        if( ++totalBytesRead >= m_flashSectorLength )
            break;
    }

    std::copy( lastSectorData.end() - 4, lastSectorData.end(), outResult.begin() );

    setProgrammingOff();
    return true;
}

bool Zm5101::checkFlashCrc() {
    auto result = setProgrammingOn();

    if( !result ) {
        std::cerr << __PRETTY_FUNCTION__ << ": set programming mode FAILED" << std::endl;
        setProgrammingOff();
        return result;
    }

    static constexpr uint8_t crcFailedMask = ( 1 << 2 );  // 0b00000100
    ChipMemoryVector buffer;
    sendRequest( m_cmdRunCrcCheck.getCommand(), m_cmdRunCrcCheck.getResultSize(), buffer );
    result = waitRunChipCrcReady();

    if( result ) {
        sendRequest( m_cmdCheckState.getCommand(), m_cmdCheckState.getResultSize(), buffer );
        if( buffer[ 0 ] & crcFailedMask ) {
            std::cerr << __PRETTY_FUNCTION__ << ": run CRC check FAILED" << std::endl;
            result = false;
        }
        else {
            result = true;
        }
    }
    else {
        std::cerr << __PRETTY_FUNCTION__ << ": run CRC check FAILED" << std::endl;
    }

    setProgrammingOff();
    return result;
}

size_t Zm5101::readFlash( ChipMemoryVector& outResult ) {
    auto result = setProgrammingOn();

    if( !result ) {
        std::cerr << __PRETTY_FUNCTION__ << ": set programming mode FAILED" << std::endl;
        setProgrammingOff();
        return false;
    }

    outResult.clear();

    auto readCmd = m_cmdReadFlash.getCommand();

    size_t totalBytesRead;
    ChipMemoryVector buffer;

    outResult.reserve( m_flashSize );
    size_t sectorNumber = 0;

    while( sectorNumber < m_flashTotalSectors ) {
        if( !waitChipContinue() ) {
            outResult.clear();
            break;
        }
        readCmd[ 1 ] = sectorNumber;
        totalBytesRead = sendRequest( readCmd, m_cmdReadFlash.getResultSize(), buffer );
        outResult.push_back( buffer[ 0 ] );

        while( true ) {
            if( !waitChipContinue() ) {
                break;
            }
            sendRequest( m_cmdReadContinue.getCommand(), m_cmdReadContinue.getResultSize(), buffer );

            outResult.push_back( buffer[ 1 ] );
            if( ++totalBytesRead >= m_flashSectorLength )
                break;
            outResult.push_back( buffer[ 2 ] );
            if( ++totalBytesRead >= m_flashSectorLength )
                break;
            outResult.push_back( buffer[ 3 ] );
            if( ++totalBytesRead >= m_flashSectorLength )
                break;
        }
        ++sectorNumber;
        printProgress( "Reading chip flash. ", sectorNumber, m_flashTotalSectors );
    }
    std::cout << std::endl;
    setProgrammingOff();
    return outResult.size();
}

size_t Zm5101::writeFlash( const ChipMemoryVector& data ) {
    auto result = setProgrammingOn();

    if( !result ) {
        std::cerr << __PRETTY_FUNCTION__ << ": set programming mode FAILED" << std::endl;
        setProgrammingOff();
        return 0;
    }

    if( data.empty() ) {
        std::cerr << "Z-Wave flash data is empty - skipping write" << std::endl;
        setProgrammingOff();
        return 0;
    }
    else if( data.size() != m_flashSize ) {
        std::cerr << "Wrong Z-Wave flash data size (" << data.size() << " bytes). Must be " << m_flashSize
                  << " bytes long" << std::endl;
        setProgrammingOff();
        return 0;
    }
    size_t positionOf3FFs;  // Special rules for FFs trimming as told in INS11681-8 document
    ChipMemoryVector buffer;

    size_t sectorNumber = 0;
    ChipMemoryVector command;

    while( sectorNumber < m_flashTotalSectors ) {
        positionOf3FFs = m_flashSectorLength - 1;

        ChipMemoryVector::const_iterator sectorBegin = data.begin() + ( sectorNumber * m_flashSectorLength );
        ChipMemoryVector::const_iterator sectorEnd = data.begin() + ( ( sectorNumber + 1 ) * m_flashSectorLength );
        ChipMemoryVector sectorData( sectorBegin, sectorEnd );

        stopAt3FFs( sectorData, &positionOf3FFs, m_flashSectorLength - 1 );
        if( positionOf3FFs == 1 && sectorData[ 0 ] == 0xFF ) {
            ChipMemoryVector eraseCmd = m_cmdEraseSector.getCommand();
            eraseCmd[ 1 ] = sectorNumber;
            sendRequest( eraseCmd, m_cmdEraseSector.getResultSize(), buffer );
            waitChipFlashFsm( CmdEraseDelayUs );
            waitChipContinue();
            ++sectorNumber;
            printProgress( "Writing chip flash. ", sectorNumber, m_flashTotalSectors );
            continue;
        }
        command = m_cmdWriteSram.getCommand();
        command[ 1 ] = 0x00;
        command[ 2 ] = 0x00;
        command[ 3 ] = sectorData[ 0 ];

        sendRequest( command, m_cmdWriteSram.getResultSize(), buffer );
        waitChipFlashFsm();
        waitChipContinue();
        size_t bytePosition = 1;
        while( true ) {
            if( bytePosition == ( m_flashSectorLength - 1 ) || bytePosition >= positionOf3FFs )
                break;
            command = m_cmdWriteContinue.getCommand();
            command[ 1 ] = sectorData[ bytePosition ];
            command[ 2 ] = sectorData[ bytePosition + 1 ];
            command[ 3 ] = sectorData[ bytePosition + 2 ];
            sendRequest( command, m_cmdWriteContinue.getResultSize(), buffer );
            waitChipContinue();
            bytePosition += 3;
        }
        command = m_cmdEraseSector.getCommand();
        command[ 1 ] = sectorNumber;
        waitChipFlashFsm( CmdEraseDelayUs );
        sendRequest( command, m_cmdEraseSector.getResultSize(), buffer );

        command = m_cmdWriteFlash.getCommand();
        command[ 1 ] = sectorNumber;
        waitChipFlashFsm( CmdEraseDelayUs );
        sendRequest( command, m_cmdWriteFlash.getResultSize(), buffer );
        waitChipFlashFsm( CmdEraseDelayUs );

        if( positionOf3FFs % 3 == 1 && positionOf3FFs > 1 && sectorData[ positionOf3FFs ] != 0xFF &&
            bytePosition == ( m_flashSectorLength - 1 ) ) {
            command = m_cmdWriteSram.getCommand();
            command[ 1 ] = 0x07;
            command[ 2 ] = 0xFF;
            command[ 3 ] = sectorData[ positionOf3FFs ];

            waitChipContinue();
            waitChipFlashFsm( CmdEraseDelayUs );
            sendRequest( command, m_cmdWriteSram.getResultSize(), buffer );

            waitChipContinue();
            waitChipFlashFsm( CmdEraseDelayUs );
            command = m_cmdWriteFlash.getCommand();
            command[ 1 ] = sectorNumber;
            sendRequest( command, m_cmdWriteFlash.getResultSize(), buffer );

            waitChipFlashFsm( CmdEraseDelayUs );
        }
        ++sectorNumber;
        printProgress( "Writing chip flash. ", sectorNumber, m_flashTotalSectors );
    }
    std::cout << std::endl;

    setProgrammingOff();
    return data.size();
}

size_t Zm5101::readNvr( ChipMemoryVector& outResult ) {
    auto result = setProgrammingOn();

    outResult.clear();
    if( !result ) {
        std::cerr << __PRETTY_FUNCTION__ << " failed: invalid descriptor" << std::endl;
        setProgrammingOff();
        return 0;
    }

    ChipMemoryVector buffer;

    ChipMemoryVector readCmd = m_cmdReadNvr.getCommand();
    size_t nvrAddress = nvrAddressFirst;
    while( nvrAddress <= nvrAddressLast ) {
        readCmd[ 2 ] = nvrAddress;
        sendRequest( readCmd, m_cmdReadNvr.getResultSize(), buffer );
        outResult.push_back( buffer[ 0 ] );
        ++nvrAddress;
        printProgress( "Reading chip NVR. ", nvrAddress - nvrAddressFirst, m_nvrSize );
    }
    std::cout << std::endl;

    setProgrammingOff();
    return outResult.size();
}

size_t Zm5101::writeNvr( const ChipMemoryVector& data ) {
    if( data.size() != m_nvrSize ) {
        std::cerr << "Wrong NVR data size ( " << data.size() << " bytes ). Must be " << m_nvrSize << " bytes long."
                  << std::endl;
        return false;
    }

    auto result = setProgrammingOn();
    if( !result ) {
        std::cerr << __PRETTY_FUNCTION__ << " failed: invalid descriptor" << std::endl;
        setProgrammingOff();
        return 0;
    }

    ChipMemoryVector buffer;

    waitChipFlashFsm( CmdEraseDelayUs );
    sendRequest( m_cmdEraseChip.getCommand(), m_cmdEraseChip.getResultSize(), buffer );
    waitChipFlashFsm( CmdEraseDelayUs );

    ChipMemoryVector writeCmd = m_cmdSetNvr.getCommand();
    size_t nvrAddress = nvrAddressFirst;
    size_t dataIndex = 0;
    while( nvrAddress <= nvrAddressLast ) {
        writeCmd[ 2 ] = nvrAddress;
        writeCmd[ 3 ] = data[ dataIndex ];
        sendRequest( writeCmd, m_cmdSetNvr.getResultSize(), buffer );
        if( !waitChipFlashFsm() ) {
            setProgrammingOff();
            return 0;
        }
        ++nvrAddress;
        ++dataIndex;
        printProgress( "Writing chip NVR. ", nvrAddress - nvrAddressFirst, m_nvrSize );
    }
    std::cout << std::endl;

    setProgrammingOff();
    return dataIndex;
}

uint16_t Zm5101::calcNvrCrc( const ChipMemoryVector& data ) {
    // CRC16 value calculated from address 0x10 to address 0x7D ! ! !
    return ccittCrc16( &data[ 7 ], ( 0x7D - 0x10 + 1 ) );
}

const size_t Zm5101::getNvrSize() const {
    return m_nvrSize;
}

void Zm5101::hwResetCycle( size_t afterDelayUs ) {
    hwResetDeassert();
    usleep( m_programmingOnTResetLowUs );

    hwResetAssert();
    usleep( m_programmingOnTResetLowUs );

    hwResetDeassert();
    if( afterDelayUs ) {
        usleep( afterDelayUs );
    }
}

void Zm5101::hwResetAssert() {
    HubPlatform::Environment::Zwave::resetAssert();
}

void Zm5101::hwResetDeassert() {
    HubPlatform::Environment::Zwave::resetDeAssert();
}

bool Zm5101::bulkErase() {
    auto result = setProgrammingOn();
    if( !result ) {
        std::cerr << __PRETTY_FUNCTION__ << " failed: invalid descriptor" << std::endl;
        setProgrammingOff();
        return false;
    }

    ChipMemoryVector buffer;

    waitChipFlashFsm( CmdEraseDelayUs );
    sendRequest( m_cmdEraseChip.getCommand(), m_cmdEraseChip.getResultSize(), buffer );
    waitChipFlashFsm( CmdEraseDelayUs );

    setProgrammingOff();
    return true;
}

std::string Zm5101::nvrGetRegion( const std::vector< uint8_t >& nvrData ) {
    auto frequency = nvrGetSawC( nvrData );

    for( const auto& item : frequencyToRegionCodeMap ) {
        if( frequency >= item.first - frequencyDeviationKHz && frequency <= item.first + frequencyDeviationKHz ) {
            return item.second;
        }
    }
    return std::string();
}

void Zm5101::nvrSetRegion( const std::string& newRegion, ChipMemoryVector& nvrData ) {
    const auto& regionIt = regionToFrequencyMap.find( newRegion );
    if( regionIt != regionToFrequencyMap.end() ) {
        nvrSetSawC( regionIt->second, nvrData );
    }
    else {
        std::cerr << "Getting frequency data for requested region \"" << newRegion << "\" FAILED - no region"
                  << std::endl;
    }
}

std::string Zm5101::nvrGetS2DskKey( const std::vector< uint8_t >& nvrData, std::vector< uint8_t >& outKey ) {
    // TODO implement
    return std::string();
}
void Zm5101::nvrSetS2DskKey( const std::vector< uint8_t >& newKey, std::vector< uint8_t >& nvrData ) {
    // TODO implement
}

uint32_t Zm5101::nvrGetSawC( const ChipMemoryVector& nvrData ) {
    uint8_t sawc1 = nvrData[ sawc1NvrDataOffset ];
    uint8_t sawc2 = nvrData[ sawc2NvrDataOffset ];
    uint8_t sawc3 = nvrData[ sawc3NvrDataOffset ];
    uint32_t nvrCenterFrequency = sawc1;
    nvrCenterFrequency = ( nvrCenterFrequency << 8 ) + sawc2;
    nvrCenterFrequency = ( nvrCenterFrequency << 8 ) + sawc3;
    return nvrCenterFrequency;
}
void Zm5101::nvrSetSawC( uint32_t nvrSawCValue, ChipMemoryVector& nvrData ) {
    nvrData[ sawc2NvrDataOffset ] = ( nvrSawCValue >> 8 );
    ;
    nvrData[ sawc1NvrDataOffset ] = ( nvrSawCValue >> 16 );
    nvrData[ sawc3NvrDataOffset ] = nvrSawCValue;

    auto newCrc = calcNvrCrc( nvrData );
    nvrData[ crc16HighNvrDataOffset ] = ( newCrc >> 8 );
    nvrData[ crc16LowNvrDataOffset ] = newCrc;
}

bool Zm5101::waitChipFlashFsm( const int32_t delayUs, int triesNumber ) {
    // Assume we are already in programming mode here
    static constexpr uint8_t waitFlashFsmBusy = ( 1 << 3 );  // 0b00001000
    ChipMemoryVector buffer;
    while( triesNumber ) {
        sendRequest( m_cmdCheckState.getCommand(), m_cmdCheckState.getResultSize(), buffer );

        if( ( buffer[ 0 ] & waitFlashFsmBusy ) == 0 ) {
            return true;
        }
        usleep( delayUs );
        --triesNumber;
    }
    std::cerr << __PRETTY_FUNCTION__ << " FAILED: chip flash FSM was busy too long..." << std::endl;
    std::cerr << "";
    return false;
}

bool Zm5101::waitChipContinue( const int32_t delayUs, int triesNumber ) {
    // Assume we are already in programming mode here
    static constexpr uint8_t waitContinueBitMask = ( 1 << 5 );  // 0b00100000
    ChipMemoryVector buffer;
    while( triesNumber ) {
        sendRequest( m_cmdCheckState.getCommand(), m_cmdCheckState.getResultSize(), buffer );

        if( ( buffer[ 0 ] & waitContinueBitMask ) == 0 ) {
            return true;
        }
        usleep( delayUs );
        --triesNumber;
    }
    std::cerr << __PRETTY_FUNCTION__ << " FAILED: chip flash was busy too long..." << std::endl;
    std::cerr << "";
    return false;
}

bool Zm5101::waitRunChipCrcReady( const int32_t delayUs, int triesNumber ) {
    // Assume we are already in programming mode here
    static constexpr uint8_t crcReadyMask = ( 1 << 1 );  // 0b00000010
    ChipMemoryVector buffer;

    while( triesNumber ) {
        usleep( delayUs );

        sendRequest( m_cmdCheckState.getCommand(), m_cmdCheckState.getResultSize(), buffer );

        if( buffer[ 0 ] & crcReadyMask ) {
            return true;
        }
        --triesNumber;
    }
    std::cerr << __PRETTY_FUNCTION__ << " FAILED: chip run crc was busy too long..." << std::endl;
    return false;
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
uint16_t Zm5101::ccittCrc16( const uint8_t* pData, uint16_t dataLength ) {
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

void Zm5101::printProgress( const char* message, const size_t done, const size_t max ) {
    static char progressMessage[ 128 ];
    memset( (void*) progressMessage, 0, sizeof( progressMessage ) );
    progressMessage[ 0 ] = '\r';
    strcpy( progressMessage + 1, message );
    float percentValue = ( (float) done * 100.0f / max );
    ( percentValue > 100.0f ? percentValue = 100.0f : percentValue = percentValue );
    snprintf(
        progressMessage + strlen( message ) + 1, sizeof( progressMessage ), "Progress: %3.1f %% done", percentValue );
    std::cout << progressMessage;
    std::cout.flush();
}
