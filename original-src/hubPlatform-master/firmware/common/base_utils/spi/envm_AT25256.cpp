#include "base_utils/spi/envm_AT25256.h"

#include "base_utils/zwave/zm5101_interface.h"

#include <fstream>
#include <unistd.h>

EnvmAt25256::EnvmAt25256()
: m_cmdReadID( { 0xFF } )
, m_cmdWriteEnable( { 0x06 } )
, m_cmdWriteDisable( { 0x04 } )
, m_cmdReadStatus( { 0x05 } )
, m_cmdWriteStatus( { 0x01 } )
, m_cmdReadDataBytes( { 0x03, 0x00, 0x00 } )
, m_cmdWriteDataBytes( { 0x02, 0x00, 0x00 } )
, m_transport() {}

bool EnvmAt25256::setProgrammingOn() {
    Zm5101::hwResetAssert();
    usleep( m_programmingOnTLowUs );
    Zm5101::hwResetDeassert();
    usleep( m_programmingOnTHighUs );
    Zm5101::hwResetAssert();
    m_transport.init( m_maxFrequency );
    if( m_transport.getDescriptor() < 0 ) {
        std::cerr << __PRETTY_FUNCTION__ << " failed: invalid descriptor" << std::endl;
        return false;
    }
    return true;
}

bool EnvmAt25256::setProgrammingOff() {
    m_transport.deinit();
    Zm5101::hwResetDeassert();
    return true;
}

bool EnvmAt25256::readStatus( EnvmMemoryVector& outResult ) {
    setProgrammingOn();

    if( m_transport.getDescriptor() < 0 ) {
        std::cerr << __PRETTY_FUNCTION__ << " failed: invalid descriptor" << std::endl;
        setProgrammingOff();
        return false;
    }

    m_transport.sendRequest( m_cmdReadStatus.getCommand(), m_cmdReadStatus.getResultSize(), outResult );

    setProgrammingOff();
    return true;
}

bool EnvmAt25256::readLocks( EnvmMemoryVector& outResult ) {
    setProgrammingOn();

    if( m_transport.getDescriptor() < 0 ) {
        std::cerr << __PRETTY_FUNCTION__ << " failed: invalid descriptor" << std::endl;
        setProgrammingOff();
        return false;
    }

    outResult.clear();
    EnvmMemoryVector buffer;
    auto result = m_transport.sendRequest( m_cmdReadStatus.getCommand(), m_cmdReadStatus.getResultSize(), buffer );

    const uint8_t lockBitMask = ( 3 << 2 );
    // 00 - protection disabled, 01 - quarter array, 10 - half array, 11 - full array write protection
    outResult.push_back( buffer[ 0 ] & lockBitMask );

    setProgrammingOff();
    return true;
}

bool EnvmAt25256::readID( std::string& outResult ) {
    // command is not natively supported by chip hardware

    const EnvmMemoryVector result = { 0x00, 0x0A, 0x12, 0x52, 0x56 };
    outResult = envm::hexToString( result.data(), result.size() );

    return true;
}

size_t EnvmAt25256::readData( EnvmMemoryVector& outResult ) {
    setProgrammingOn();

    if( m_transport.getDescriptor() < 0 ) {
        std::cerr << __PRETTY_FUNCTION__ << " failed: invalid descriptor" << std::endl;
        setProgrammingOff();
        return false;
    }

    EnvmMemoryVector buffer;
    outResult.clear();
    outResult.reserve( m_size );

    EnvmMemoryVector readCmd = m_cmdReadDataBytes.getCommand();

    for( size_t pageNumber = 0; pageNumber < m_pagesCount; ++pageNumber ) {
        const size_t address = ( pageNumber << 6 );
        m_transport.sendRequest( readCmd, m_cmdReadDataBytes.getResultSize(), buffer );
        for( size_t i = 0; i < buffer.size(); ++i ) {
            outResult.push_back( buffer[ i ] );
        }
    }

    setProgrammingOff();
    return outResult.size();
}

size_t EnvmAt25256::writeData( const EnvmMemoryVector& data ) {
    setProgrammingOn();

    if( m_transport.getDescriptor() < 0 ) {
        std::cerr << __PRETTY_FUNCTION__ << " failed: invalid descriptor" << std::endl;
        setProgrammingOff();
        return false;
    }

    EnvmMemoryVector buffer;
    size_t dataIndex = 0;

    EnvmMemoryVector writeCmd = m_cmdWriteDataBytes.getCommand();
    writeCmd.resize( m_cmdWriteDataBytes.getCommandSize() + m_bytesWriteCount );

    for( size_t address = 0; address < m_size; address += m_bytesWriteCount ) {
        writeCmd[ 2 ] = address;
        writeCmd[ 1 ] = ( address >> 8 );

        for( size_t index = 0; index < m_bytesWriteCount; ++index ) {
            writeCmd[ m_cmdWriteDataBytes.getCommandSize() + index ] = data[ dataIndex++ ];
        }

        m_transport.sendRequest( m_cmdWriteEnable.getCommand(), m_cmdWriteEnable.getResultSize(), buffer );
        if( !waitChipWriteEnabled() ) {
            setProgrammingOff();
            std::cerr << "Chip wait write enable timeout" << std::endl;
            return 0;
        }
        m_transport.sendRequest( writeCmd, m_cmdWriteDataBytes.getResultSize(), buffer );
        if( !waitChipReady() ) {
            setProgrammingOff();
            std::cerr << "Chip wait ready timeout" << std::endl;
            return 0;
        }
    }

    setProgrammingOff();

    return dataIndex;
}

bool EnvmAt25256::bulkErase() {
    setProgrammingOn();

    if( m_transport.getDescriptor() < 0 ) {
        std::cerr << __PRETTY_FUNCTION__ << " failed: invalid descriptor" << std::endl;
        setProgrammingOff();
        return false;
    }
    EnvmMemoryVector buffer;

    EnvmMemoryVector eraseCmd = m_cmdWriteDataBytes.getCommand();
    eraseCmd.resize( m_cmdWriteDataBytes.getCommandSize() + m_bytesWriteCount, 0xFF );
    m_transport.sendRequest( m_cmdWriteEnable.getCommand(), m_cmdWriteEnable.getResultSize(), buffer );
    if( !waitChipWriteEnabled() ) {
        setProgrammingOff();
        std::cerr << "Chip wait write enable timeout" << std::endl;
        return false;
    }

    for( size_t pageNumber = 0; pageNumber < m_pagesCount; ++pageNumber ) {
        const size_t address = pageNumber << 6;
        eraseCmd[ 2 ] = address;
        eraseCmd[ 1 ] = ( address >> 8 );

        debug_out( "Erasing page: " << std::to_string( pageNumber ) );

        m_transport.sendRequest( eraseCmd, m_cmdWriteDataBytes.getResultSize(), buffer );
        usleep( 10000 );

        if( !waitChipReady() ) {
            setProgrammingOff();
            std::cerr << "Chip wait ready timeout" << std::endl;
            return false;
        }
    }

    setProgrammingOff();
    return true;
}

size_t EnvmAt25256::size() const {
    return m_size;
}

bool EnvmAt25256::waitChipReady( const int32_t delayUs, int triesNumber ) {
    EnvmMemoryVector outResult;
    const uint8_t waitBusyBitMask = 1;  // 0b00000001
    while( triesNumber-- ) {
        m_transport.sendRequest( m_cmdReadStatus.getCommand(), m_cmdReadStatus.getResultSize(), outResult );
        debug_out( "Waiting busy bit. Status: " << hexToString( &outResult[ 0 ], 1 ) );
        // break if BUSY bit is cleared
        if( ( outResult[ 0 ] & waitBusyBitMask ) == 0 ) {
            debug_out( "Chip is ready. Status:  " << hexToString( &outResult[ 0 ], 1 ) );
            return true;
        }
        usleep( delayUs );
    }
    std::cerr << "ENVM timeout wait busy bit." << std::endl;
    return false;
}

bool EnvmAt25256::waitChipWriteEnabled( const int32_t delayUs, int triesNumber ) {
    EnvmMemoryVector outResult;
    const uint8_t waitWriteEnableBitMask = 2;  // 0b00000010
    while( triesNumber-- ) {
        m_transport.sendRequest( m_cmdReadStatus.getCommand(), m_cmdReadStatus.getResultSize(), outResult );
        debug_out( "Waiting write enable bit. Status: " << hexToString( &outResult[ 0 ], 1 ) );
        // break if BUSY bit is cleared
        if( ( outResult[ 0 ] & waitWriteEnableBitMask ) == waitWriteEnableBitMask ) {
            debug_out( "Chip is write enabled. Status: " << hexToString( outResult[ 0 ], 1 ) );
            return true;
        }
        usleep( delayUs );
    }
    std::cerr << "ENVM timeout write enable bit." << std::endl;
    return false;
}
