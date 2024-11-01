#include "base_utils/spi/envm_AT45DB041.h"

#include "base_utils/zwave/zm5101_interface.h"

#include <fstream>
#include <unistd.h>

namespace {
static constexpr uint8_t const dummyByte = 0x55;

static constexpr int const CmdEraseChipPollingTimeUs = 1700000;  // 1.7 seconds
static constexpr int const CmdErasePageDelayUs = 5000;           // 5 milliseconds

}  // namespace

EnvmAt45Db041::EnvmAt45Db041()
: m_cmdReadID( { 0x9F } )
, m_cmdReadStatus( { 0xD7 } )
, m_cmdReadDataBytes( { 0xD2, 0x00, 0x00, 0x00, dummyByte, dummyByte, dummyByte, dummyByte } )
, m_cmdReadSectorProtection( { 0x32, dummyByte, dummyByte, dummyByte } )
, m_cmdBuffer1Write( { 0x84, 0x00, 0x00, 0x00 } )
,  // 256-byte width buffer
    m_cmdBuffer2Write( { 0x87, 0x00, 0x00, 0x00 } )
,  // 256-byte width buffer
    m_cmdWritePageBuffer1( { 0x83, 0x00, 0x00, 0x00 } )
,  // 256-byte width page
    m_cmdWritePageBuffer2( { 0x86, 0x00, 0x00, 0x00 } )
,  // 256-byte width page
    m_cmdErasePage( { 0x81, 0x00, 0x00, 0x00 } )
, m_cmdEraseSector( { 0x7C, 0x00, 0x00, 0x00 } )
, m_cmdEraseChip( { 0xC7, 0x94, 0x80, 0x9A } )
, m_cmdResetChip( { 0xF0, 0x00, 0x00, 0x00 } )
, m_transport() {}

bool EnvmAt45Db041::setProgrammingOn() {
    Zm5101::hwResetAssert();
    usleep( m_deviceResetTimeoutUs );
    auto fd = m_transport.init( m_maxFrequency );
    if( fd < 0 ) {
        std::cerr << __PRETTY_FUNCTION__ << " failed: invalid descriptor " << fd << std::endl;
        return false;
    }
    return true;
}

bool EnvmAt45Db041::setProgrammingOff() {
    m_transport.deinit();
    Zm5101::hwResetDeassert();
    return true;
}

bool EnvmAt45Db041::readStatus( EnvmMemoryVector& outResult ) {
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

bool EnvmAt45Db041::readLocks( EnvmMemoryVector& outResult ) {
    setProgrammingOn();

    if( m_transport.getDescriptor() < 0 ) {
        std::cerr << __PRETTY_FUNCTION__ << " failed: invalid descriptor" << std::endl;
        setProgrammingOff();
        return false;
    }

    auto result = m_transport.sendRequest(
        m_cmdReadSectorProtection.getCommand(), m_cmdReadSectorProtection.getResultSize(), outResult );

    setProgrammingOff();
    return ( result == m_cmdReadSectorProtection.getResultSize() );
}

bool EnvmAt45Db041::readID( std::string& outResult ) {
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

size_t EnvmAt45Db041::readData( EnvmMemoryVector& outResult ) {
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
        readCmd[ 3 ] = 0;
        readCmd[ 2 ] = pageNumber;
        readCmd[ 1 ] = ( pageNumber >> 8 );
        m_transport.sendRequest( readCmd, m_cmdReadDataBytes.getResultSize(), buffer );
        for( size_t i = 0; i < m_bytesPerPage; ++i ) {
            outResult.push_back( buffer[ i ] );
        }
    }

    setProgrammingOff();
    return outResult.size();
}

size_t EnvmAt45Db041::writeData( const EnvmMemoryVector& data ) {
    setProgrammingOn();

    if( m_transport.getDescriptor() < 0 ) {
        std::cerr << __PRETTY_FUNCTION__ << " failed: invalid descriptor" << std::endl;
        setProgrammingOff();
        return false;
    }

    EnvmMemoryVector buffer;
    size_t dataIndex = 0;
    auto pagesToWrite = data.size() / m_bytesPerPage + ( data.size() & 0xFF ? 1 : 0 );
    std::cout << "Pages to write: " << pagesToWrite << "; bytes to write: " << data.size() << std::endl;
    if( pagesToWrite > m_pagesCount ) {
        std::cerr << __PRETTY_FUNCTION__ << " failed: file does not fit into memory" << std::endl;
        setProgrammingOff();
        return false;
    }

    EnvmMemoryVector writeCmd;
    for( size_t pageNumber = 0; pageNumber < pagesToWrite; ++pageNumber ) {
        writeCmd = m_cmdBuffer1Write.getCommand();
        writeCmd.resize( m_cmdBuffer1Write.getCommandSize() + m_bytesPerPage );

        size_t byteNumber = 0;
        dataIndex = pageNumber * m_bytesPerPage;
        while( dataIndex < data.size() && byteNumber < m_bytesPerPage ) {
            writeCmd[ byteNumber + 4 ] = data[ dataIndex ];
            ++dataIndex;
            ++byteNumber;
        }
        if( byteNumber < m_bytesPerPage ) {
            writeCmd.resize( m_cmdBuffer1Write.getCommandSize() + byteNumber );
        }

        m_transport.sendRequest( writeCmd, m_cmdBuffer1Write.getResultSize(), buffer );

        writeCmd = m_cmdWritePageBuffer1.getCommand();
        writeCmd[ 3 ] = 0;
        writeCmd[ 2 ] = pageNumber;
        writeCmd[ 1 ] = ( pageNumber >> 8 );

        m_transport.sendRequest( writeCmd, m_cmdWritePageBuffer1.getResultSize(), buffer );

        if( !waitChipReady() ) {
            dataIndex = 0;
            std::cerr << __PRETTY_FUNCTION__ << ": Chip wait ready timeout at page " << (int) pageNumber << std::endl;
            break;
        }
    }
    setProgrammingOff();
    return dataIndex;
}

bool EnvmAt45Db041::bulkErase() {
    return bulkEraseByEraseChip();
}

size_t EnvmAt45Db041::size() const {
    return m_size;
}

bool EnvmAt45Db041::waitChipReady( const int32_t delayUs, int triesNumber ) {
    EnvmMemoryVector outResult;
    const uint8_t waitReadyBitMask = ( 1 << 7 );
    while( triesNumber-- ) {
        m_transport.sendRequest( m_cmdReadStatus.getCommand(), m_cmdReadStatus.getResultSize(), outResult );
        if( outResult[ 1 ] & waitReadyBitMask ) {
            return true;
        }
        usleep( delayUs );
    }
    std::cerr << "ENVM timeout wait busy bit: "
              << envm::hexToString( (const uint8_t*) &outResult[ 0 ], sizeof( uint8_t ), true )
              << envm::hexToString( (const uint8_t*) &outResult[ 1 ], sizeof( uint8_t ), true ) << std::endl;
    return false;
}

bool EnvmAt45Db041::eraseProgrammSucceded() {
    EnvmMemoryVector outResult;
    const uint8_t epeBitMask = 0x20;

    m_transport.sendRequest( m_cmdReadStatus.getCommand(), m_cmdReadStatus.getResultSize(), outResult );
    if( outResult[ 1 ] & epeBitMask ) {
        std::cerr << "ENVM erase/program failed: "
                  << envm::hexToString( (const uint8_t*) &outResult[ 1 ], sizeof( uint8_t ), true ) << std::endl;
        return false;
    }

    return true;
}

bool EnvmAt45Db041::bulkEraseByErasePages() {
    setProgrammingOn();

    if( m_transport.getDescriptor() < 0 ) {
        std::cerr << __PRETTY_FUNCTION__ << " failed: invalid descriptor" << std::endl;
        setProgrammingOff();
        return false;
    }
    EnvmMemoryVector buffer;
    m_transport.sendRequest( m_cmdResetChip.getCommand(), m_cmdResetChip.getResultSize(), buffer );
    usleep( CmdErasePageDelayUs );

    EnvmMemoryVector eraseCmd = m_cmdErasePage.getCommand();
    size_t pageNumber = 0;
    int failedPageNumber = -1;
    while( pageNumber < m_pagesCount ) {
        eraseCmd[ 3 ] = 0;
        eraseCmd[ 2 ] = pageNumber;
        eraseCmd[ 1 ] = ( pageNumber >> 8 );

        m_transport.sendRequest( eraseCmd, m_cmdErasePage.getResultSize(), buffer );

        if( !eraseProgrammSucceded() ) {
            if( failedPageNumber != pageNumber ) {
                failedPageNumber = pageNumber;
                std::cerr << "ENVM erase/programm error bit set for page number " << pageNumber << std::endl;
            }
            usleep( CmdErasePageDelayUs );
            continue;
        }
        if( !waitChipReady() ) {
            setProgrammingOff();
            std::cerr << "Chip wait ready timeout" << std::endl;
            return false;
        }
        ++pageNumber;
    }

    setProgrammingOff();
    return true;
}

bool EnvmAt45Db041::bulkEraseByEraseChip() {
    setProgrammingOn();

    if( m_transport.getDescriptor() < 0 ) {
        std::cerr << __PRETTY_FUNCTION__ << " failed: invalid descriptor" << std::endl;
        setProgrammingOff();
        return false;
    }
    EnvmMemoryVector buffer;

    for( size_t retries = 0; retries < 5; ++retries ) {
        m_transport.sendRequest( m_cmdEraseChip.getCommand(), m_cmdEraseChip.getResultSize(), buffer );

        if( !waitChipReady( CmdEraseChipPollingTimeUs ) ) {
            std::cerr << "Chip wait ready timeout" << std::endl;
        }
        else {
            if( eraseProgrammSucceded() ) {
                return true;
            }
        }
        std::cerr << "Erase chip retrying..." << std::endl;
    }
    setProgrammingOff();
    return false;
}
