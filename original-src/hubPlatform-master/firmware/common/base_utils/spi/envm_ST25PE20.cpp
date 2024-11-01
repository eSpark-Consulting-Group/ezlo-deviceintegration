#include "base_utils/spi/envm_ST25PE20.h"

#include "base_utils/zwave/zm5101_interface.h"

#include <fstream>
#include <unistd.h>
#include <ctime>

EnvmSt25Pe20::EnvmSt25Pe20()
: m_cmdReadID( { 0x9F } )
, m_cmdReadStatus( { 0x05 } )
, m_cmdReadLocks( { 0xE8, 0x00, 0x00, 0x00 } )
, m_cmdReadDataBytes( { 0x03, 0x00, 0x00, 0x00 } )
, m_cmdWritePage( { 0x0A, 0x00, 0x00, 0x00 } )
, m_cmdWriteEnable( { 0x06 } )
, m_cmdEraseSector( { 0xD8, 0x00, 0x00, 0x00 } )
, m_cmdErasePage( { 0xDB, 0x00, 0x00, 0x00 } )
, m_transport() {}

bool EnvmSt25Pe20::setProgrammingOn() {
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

bool EnvmSt25Pe20::setProgrammingOff() {
    m_transport.deinit();
    Zm5101::hwResetDeassert();
    return true;
}

bool EnvmSt25Pe20::readStatus( EnvmMemoryVector& outResult ) {
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

bool EnvmSt25Pe20::readLocks( EnvmMemoryVector& outResult ) {
    setProgrammingOn();

    if( m_transport.getDescriptor() < 0 ) {
        std::cerr << __PRETTY_FUNCTION__ << " failed: invalid descriptor" << std::endl;
        setProgrammingOff();
        return false;
    }

    outResult.clear();
    EnvmMemoryVector buffer;
    EnvmMemoryVector readCmd = m_cmdReadLocks.getCommand();
    for( size_t pageNumber = 0; pageNumber < m_pagesCount; ++pageNumber ) {
        readCmd[ 3 ] = 0;
        readCmd[ 2 ] = pageNumber;
        readCmd[ 1 ] = ( pageNumber >> 8 );

        m_transport.sendRequest( readCmd, m_cmdReadLocks.getResultSize(), buffer );
        for( size_t i = 0; i < buffer.size(); ++i ) {
            outResult.push_back( buffer[ i ] );
        }
    }

    setProgrammingOff();
    return true;
}

bool EnvmSt25Pe20::readID( std::string& outResult ) {
    setProgrammingOn();

    if( m_transport.getDescriptor() < 0 ) {
        std::cerr << __PRETTY_FUNCTION__ << " failed: invalid descriptor" << std::endl;
        setProgrammingOff();
        return false;
    }

    EnvmMemoryVector result;
    m_transport.sendRequest( m_cmdReadID.getCommand(), m_cmdReadID.getResultSize(), result );

    outResult.erase();
    outResult = envm::hexToString( result.data(), m_cmdReadID.getResultSize() );

    setProgrammingOff();
    return true;
}

size_t EnvmSt25Pe20::readData( EnvmMemoryVector& outResult ) {
    setProgrammingOn();

    if( m_transport.getDescriptor() < 0 ) {
        std::cerr << __PRETTY_FUNCTION__ << " failed: invalid descriptor" << std::endl;
        setProgrammingOff();
        return false;
    }

    EnvmMemoryVector buffer;
    size_t envmSize = m_bytesPerPage * m_pagesCount;
    outResult.clear();
    outResult.reserve( envmSize );

    EnvmMemoryVector readCmd = m_cmdReadDataBytes.getCommand();

    for( size_t pageNumber = 0; pageNumber < m_pagesCount; ++pageNumber ) {
        for( size_t byteCount = 0; byteCount < m_bytesPerPage; byteCount += m_bytesReadCount ) {
            readCmd[ 3 ] = byteCount;
            readCmd[ 2 ] = pageNumber;
            readCmd[ 1 ] = ( pageNumber >> 8 );
            m_transport.sendRequest( readCmd, m_bytesReadCount, buffer );
            for( size_t i = 0; i < m_bytesReadCount; ++i ) {
                outResult.push_back( buffer[ i ] );
            }
        }
    }

    setProgrammingOff();
    return outResult.size();
}

size_t EnvmSt25Pe20::writeData( const EnvmMemoryVector& data ) {
    setProgrammingOn();

    if( m_transport.getDescriptor() < 0 ) {
        std::cerr << __PRETTY_FUNCTION__ << " failed: invalid descriptor" << std::endl;
        setProgrammingOff();
        return false;
    }

    EnvmMemoryVector buffer;
    size_t dataIndex = 0;

    EnvmMemoryVector writeCmd = m_cmdWritePage.getCommand();
    writeCmd.resize( m_cmdWritePage.getCommandSize() + m_bytesWriteCount );
    for( size_t addr2 = 0; addr2 < m_pagesCount; ++addr2 ) {
        for( size_t addr3 = 0; addr3 < m_bytesPerPage; addr3 += m_bytesWriteCount ) {
            writeCmd[ 3 ] = addr3;
            writeCmd[ 2 ] = addr2;
            writeCmd[ 1 ] = ( addr2 >> 8 );

            for( size_t i = 0; i < m_bytesWriteCount; ++i ) {
                writeCmd[ i + 4 ] = data[ dataIndex++ ];
            }

            m_transport.sendRequest( m_cmdWriteEnable.getCommand(), m_cmdWriteEnable.getResultSize(), buffer );
            if( !waitChipWriteEnabled() ) {
                setProgrammingOff();
                std::cerr << "Chip wait write enable timeout" << std::endl;
                return 0;
            }
            m_transport.sendRequest( writeCmd, m_cmdWritePage.getResultSize(), buffer );
            if( !waitChipReady() ) {
                setProgrammingOff();
                std::cerr << "Chip wait ready timeout" << std::endl;
                return 0;
            }
        }
    }

    setProgrammingOff();

    return dataIndex;
}

bool EnvmSt25Pe20::bulkErase() {
    setProgrammingOn();

    if( m_transport.getDescriptor() < 0 ) {
        std::cerr << __PRETTY_FUNCTION__ << " failed: invalid descriptor" << std::endl;
        setProgrammingOff();
        return false;
    }
    EnvmMemoryVector buffer;

    EnvmMemoryVector eraseCmd = m_cmdErasePage.getCommand();
    for( size_t pageNumber = 0; pageNumber < m_pagesCount; ++pageNumber ) {
        m_transport.sendRequest( m_cmdWriteEnable.getCommand(), m_cmdWriteEnable.getResultSize(), buffer );
        if( !waitChipWriteEnabled() ) {
            setProgrammingOff();
            std::cerr << "Chip wait write enable timeout" << std::endl;
            return false;
        }

        eraseCmd[ 3 ] = 1;
        eraseCmd[ 2 ] = pageNumber;
        eraseCmd[ 1 ] = ( pageNumber >> 8 );

        m_transport.sendRequest( eraseCmd, m_cmdErasePage.getResultSize(), buffer );
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

size_t EnvmSt25Pe20::size() const {
    return m_size;
}

bool EnvmSt25Pe20::waitChipReady( const int32_t delayUs, int triesNumber ) {
    EnvmMemoryVector outResult;
    const uint8_t waitBusyBitMask = 1;  // 0b00000001
    while( triesNumber-- ) {
        m_transport.sendRequest( m_cmdReadStatus.getCommand(), m_cmdReadStatus.getResultSize(), outResult );
        debug_out( "Waiting busy bit. Status: " << hexToString( &outResult[ 0 ], 1 ) );
        // break if BUSY bit is cleared
        if( ( outResult[ 0 ] & waitBusyBitMask ) == 0 ) {
            debug_out( "Chip is ready. Status: " << hexToString( &outResult[ 0 ], 1 ) );
            return true;
        }
        usleep( delayUs );
    }
    std::cerr << "ENVM timeout wait busy bit." << std::endl;
    return false;
}

bool EnvmSt25Pe20::waitChipWriteEnabled( const int32_t delayUs, int triesNumber ) {
    EnvmMemoryVector outResult;
    const uint8_t waitWriteEnableBitMask = 2;  // 0b00000010
    while( triesNumber-- ) {
        m_transport.sendRequest( m_cmdReadStatus.getCommand(), m_cmdReadStatus.getResultSize(), outResult );
        debug_out( "Waiting write enable bit. Status: " << hexToString( &outResult[ 0 ], 1 ) );
        // break if BUSY bit is cleared
        if( ( outResult[ 0 ] & waitWriteEnableBitMask ) == waitWriteEnableBitMask ) {
            debug_out( "Chip is write enabled. Status: " << hexToString( &outResult[ 0 ], 1 ) );
            return true;
        }
        usleep( delayUs );
    }
    std::cerr << "ENVM timeout write enable bit." << std::endl;
    return false;
}
