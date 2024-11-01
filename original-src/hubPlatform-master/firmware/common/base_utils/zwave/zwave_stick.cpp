
#include "base_utils/zwave/zwave_stick.h"
#include "base_utils/md5.h"

#include <string.h>
#include <iostream>
#include <fstream>
#include <map>

////////////////////////////////////////////////////////////
// Reads status byte of ENVM chip
////////////////////////////////////////////////////////////

bool ZWaveStick::envmReadStatus( std::string& outStatus ) {
    std::vector< uint8_t > status;
    const bool result = m_envm->readStatus( status );
    outStatus = envm::hexToString( status.data(), status.size() );
    return result;
}

////////////////////////////////////////////////////////////
// Reads lock bytes of ENVM chip
////////////////////////////////////////////////////////////

bool ZWaveStick::envmReadLocks( std::string& outStatus ) {
    std::vector< uint8_t > status;
    const bool result = m_envm->readLocks( status );
    outStatus = envm::hexToString( status.data(), status.size() );
    return result;
}

////////////////////////////////////////////////////////////
// Reads data from ENVM
////////////////////////////////////////////////////////////

bool ZWaveStick::envmRead( std::vector< unsigned char >& envmData ) {
    m_envm->readData( envmData );
    return true;
}

////////////////////////////////////////////////////////////
// Reads data from ENVM and homeID ( Z-Wave )
////////////////////////////////////////////////////////////

bool ZWaveStick::envmRead( std::vector< unsigned char >& envmData, std::string& homeId ) {
    m_envm->readData( envmData );
    homeId = getHomeId( envmData );
    return true;
}

////////////////////////////////////////////////////////////
// Writes data to ENVM
////////////////////////////////////////////////////////////

bool ZWaveStick::envmWrite( const std::vector< uint8_t >& envmData ) {
    auto result = m_envm->writeData( envmData );

    return ( result == envmData.size() );
}

////////////////////////////////////////////////////////////
// Reads ENVM chip contents and saves it to file
////////////////////////////////////////////////////////////

bool ZWaveStick::envmSaveToFile( const std::string& filePath, std::string& reason ) {
    std::vector< unsigned char > envmData;
    int tryToRead = 5;
    while( tryToRead-- ) {
        if( envmRead( envmData ) ) {
            std::ofstream ofs( filePath, std::ios::trunc );
            if( !ofs.good() ) {
                reason = "Envm file is not opened";
                return false;
            }
            const std::vector< unsigned char > md5 = Md5::getBin( envmData );
            envmData.insert( envmData.end(), md5.cbegin(), md5.cend() );
            ofs.write( (char*) envmData.data(), envmData.size() );
            ofs.close();
            if( !envmGetDataFromFile( filePath, envmData, reason ) ) {
                return false;
            }
            return true;
        }
    }
    reason = "Envm read error";
    return false;
}

bool ZWaveStick::envmGetDataFromFile( const std::string& filePath,
                                      std::vector< unsigned char >& envmData,
                                      std::string& reason ) {
    std::ifstream ifs( filePath );
    if( !ifs.good() ) {
        reason = "Envm file is not opened";
        return false;
    }
    std::filebuf* pbuf = ifs.rdbuf();
    const std::size_t size = pbuf->pubseekoff( 0, ifs.end, ifs.in );
    pbuf->pubseekpos( 0, ifs.in );
    envmData.resize( size );
    pbuf->sgetn( (char*) envmData.data(), size );
    if( envmData.size() < Md5::getBinSize() ) {
        reason = "Envm file bad size";
        return false;
    }
    const std::vector< unsigned char > md5( envmData.cend() - Md5::getBinSize(), envmData.cend() );
    envmData.resize( envmData.size() - Md5::getBinSize() );
    if( md5 != Md5::getBin( envmData ) ) {
        reason = "Envm file bad md5";
        return false;
    }
    return true;
}

bool ZWaveStick::envmWriteFromFile( const std::string& filePath, std::string& reason ) {
    std::vector< uint8_t > envmData;
    if( !envmGetDataFromFile( filePath, envmData, reason ) ) {
        return false;
    }
    if( !envmWrite( envmData ) ) {
        reason = "Envm write error";
        return false;
    }
    return true;
}

////////////////////////////////////////////////////////////
// Reads ENVM JEDEC signature
////////////////////////////////////////////////////////////

bool ZWaveStick::envmGetChipInfo( std::string& envmSignature ) {
    m_envm->readID( envmSignature );
    return true;
}

////////////////////////////////////////////////////////////
// Erases ENVM storage (homeId and Z-Wave data)
////////////////////////////////////////////////////////////

bool ZWaveStick::envmErase() {
    return m_envm->bulkErase();
}

////////////////////////////////////////////////////////////
// Reads data from Z-Wave chip
////////////////////////////////////////////////////////////

bool ZWaveStick::zwChipRead( std::vector< uint8_t >& nvrData, std::vector< uint8_t >& flashData, const bool nvrOnly ) {
    nvrData.clear();
    flashData.clear();

    m_zwave->readNvr( nvrData );
    if( !nvrOnly ) {
        m_zwave->readFlash( flashData );
    }

    return true;
}

////////////////////////////////////////////////////////////
// Writes data to Z-Wave chip
////////////////////////////////////////////////////////////

bool ZWaveStick::zwChipWrite( const std::vector< uint8_t >& nvrData,
                              const std::vector< uint8_t >& flashData,
                              bool forced ) {
    auto nvrWriteResult = m_zwave->writeNvr( nvrData );
    auto flashWriteResult = m_zwave->writeFlash( flashData );
    return ( nvrWriteResult == nvrData.size() && flashWriteResult == flashData.size() );
}

bool ZWaveStick::zwChipErase() {
    return m_zwave->bulkErase();
}

uint16_t ZWaveStick::zwCalcNvrCrc( const std::vector< uint8_t >& nvrData ) {
    return m_zwave->calcNvrCrc( nvrData );
}

const size_t ZWaveStick::zwGetNvrSize() const {
    return m_zwave->getNvrSize();
}

std::string ZWaveStick::getHomeId( const std::vector< uint8_t >& envmData ) {
    const int homeIdBytesCount = 4;
    const uint8_t homeIdAddrStart = 0x08;

    std::vector< uint8_t >::const_iterator first = envmData.begin() + homeIdAddrStart;
    std::vector< uint8_t >::const_iterator last = envmData.begin() + ( homeIdAddrStart + homeIdBytesCount );
    std::vector< uint8_t > homeIdData( first, last );
    return envm::hexToString( homeIdData.data(), homeIdBytesCount, false );
}

ZWaveStick::ZWaveStick() {
    m_envm.reset( new EnvmChip() );

    m_zwave.reset( new ZwaveChip() );

    if( m_zwave->isSupported() ) {
        std::cout << "Z-Wave stick based on " << m_zwave->getName() << " is supported\n";
        return;
    }

    m_zwave.reset( new HubPlatform::Efr32Zg14() );
    if( m_zwave->isSupported() ) {
        m_envm.reset( new EnvmMock() );
        std::cout << "Z-Wave stick based on " << m_zwave->getName() << " is supported\n";
        return;
    }

    std::cerr << "Z-Wave stick: no supported device found\n";
    std::flush( std::cerr );
    exit( EXIT_FAILURE );
}

ZWaveStick::~ZWaveStick() {
    if( m_zwave ) {
        m_zwave.reset();
    }
}

void ZWaveStick::zwResetCycle( const size_t delayAfterUs ) {
    std::cout << "Z-Wave stick reset cycle...\n";
    Zm5101::hwResetCycle( delayAfterUs );
    std::cout << "Reseting Z-Wave stick done successfully\n";
}

void ZWaveStick::zwResetAssert() {
    Zm5101::hwResetAssert();
}

void ZWaveStick::zwResetRelease() {
    Zm5101::hwResetDeassert();
}

std::string ZWaveStick::nvrGetRegion( const std::vector< uint8_t >& nvrData ) {
    return ( m_zwave->nvrGetRegion( nvrData ) );
}

void ZWaveStick::nvrSetRegion( const std::string& newRegion, std::vector< uint8_t >& nvrData ) {
    m_zwave->nvrSetRegion( newRegion, nvrData );
}

uint32_t ZWaveStick::nvrGetSawC( const std::vector< uint8_t >& nvrData ) {
    return m_zwave->nvrGetSawC( nvrData );
}
void ZWaveStick::nvrSetSawC( uint32_t nvrSawCValue, std::vector< uint8_t >& nvrData ) {
    m_zwave->nvrSetSawC( nvrSawCValue, nvrData );
}

bool ZWaveStick::zwGetChipInfo( std::string& chipSignature, std::string& chipFlashCrc ) {
    chipSignature.erase();
    chipFlashCrc.erase();

    std::vector< uint8_t > result;
    m_zwave->readID( result );
    chipSignature = envm::hexToString( result.data(), result.size(), true );

    m_zwave->readFlashCrc( result );
    chipFlashCrc = envm::hexToString( result.data(), result.size(), true );

    return true;
}
