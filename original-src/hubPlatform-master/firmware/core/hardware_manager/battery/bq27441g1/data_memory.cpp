#include "hardware_manager/battery/bq27441g1/data_memory.h"
#include "hardware_manager/battery/bq27441g1/bq27441g1.h"
#include "hardware_manager/battery/bq27441g1/classes.h"

#include "log_utils/ezlo_log.h"
#include "tiny/tiny_consolecolors.h"

#include <chrono>
#include <thread>

namespace HubPlatform {
namespace HardwareManager {
namespace Bq27441g1 {

DataMemory::DataMemory( const Bq27441g1& chip, uint8_t subclassId, uint8_t blockIdx, bool configUpdateMode )
: m_chip( const_cast< Bq27441g1* >( &chip ) )
, m_subclassId( subclassId )
, m_blockIdx( blockIdx )
, m_configUpdateMode( configUpdateMode ) {
    Log::trace( cc::trace( "Accessing data memory for subclass " ) + cc::num10( subclassId ) +
                cc::trace( " and block: " ) + cc::num10( blockIdx ) );

    if( m_configUpdateMode ) {
        m_chip->control( ControlFunction::SET_CFGUPDATE );
        int i = 0;
        while( !m_chip->readFlags().CFGUPMODE ) {
            if( ++i > 20 ) {
                throw CommunicationError( cc::error( "Failed to enter CFGUPDATE mode" ) );
            }

            std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
        }

        if( m_chip->writeUint8( static_cast< uint8_t >( ExtendedCommand::BlockDataControl ), 0x00 ) < 0 ) {
            throw CommunicationError( cc::error( "Failed to enter block data control" ) );
        }
        std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
    }
    m_chip->extendedCommand( ExtendedCommand::DataClass, m_subclassId, m_blockIdx );
    std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
    if( m_configUpdateMode ) {
        m_checksum = m_chip->readUint8( static_cast< uint8_t >( ExtendedCommand::BlockDataCheckSum ) );
    }
    // verifyBlock();
}

void DataMemory::commit() {
    if( m_configUpdateMode ) {
        if( m_chip->writeUint8( static_cast< uint8_t >( ExtendedCommand::BlockDataCheckSum ), m_checksum ) < 0 ) {
            throw CommunicationError( cc::error( "Failed to write new checksum" ) );
        }

        m_chip->control( ControlFunction::SOFT_RESET );
        int i = 0;
        while( m_chip->readFlags().CFGUPMODE ) {
            if( ++i > 20 ) {
                throw CommunicationError( cc::error( "Failed to exit CFGUPDATE mode" ) );
            }

            std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
        }
    }
}

bool DataMemory::verifyBlock() const {
    __uint8_t hash = 0;
    for( int offset = 0; offset < 32; offset += 2 ) {
        uint16_t word = m_chip->readUint16( address( offset ) );
        hash += ( word >> 8u ) + ( word & 0xFFu );
    }
    hash = 255 - hash;
    uint8_t checkSum = m_chip->readUint8( static_cast< uint8_t >( ExtendedCommand::BlockDataCheckSum ) );
    std::string status = ( hash == checkSum ) ? cc::success( "EQUAL" ) : cc::error( "MISSMATCH" );
    Log::debug( cc::debug( "HASH: calculated: " ) + cc::num0x( hash ) + cc::debug( ", actual: " ) +
                cc::num0x( checkSum ) + " " + status );
    return hash == checkSum;
}

void DataMemory::ensureH1( uint8_t offset, uint8_t value ) {
    auto oldValue = readH1( offset );
    if( value == oldValue ) {
        return;
    }
    rewriteH1( offset, oldValue, value );
}

void DataMemory::ensureH2( uint8_t offset, uint16_t value ) {
    auto oldValue = readH2( offset );
    if( value == oldValue ) {
        return;
    }
    rewriteH2( offset, oldValue, value );
}

void DataMemory::ensureI1( uint8_t offset, int8_t value ) {
    ensureH1( offset, static_cast< uint8_t >( value ) );
}

void DataMemory::ensureI2( uint8_t offset, int16_t value ) {
    ensureH2( offset, static_cast< uint16_t >( value ) );
}

void DataMemory::ensureU1( uint8_t offset, uint8_t value ) {
    ensureH1( offset, value );
}

void DataMemory::ensureU2( uint8_t offset, uint16_t value ) {
    ensureH2( offset, value );
}

void DataMemory::rewriteH1( uint8_t offset, uint8_t oldValue, uint8_t newValue ) {
    if( m_chip->writeUint8( address( offset ), newValue ) < 0 ) {
        throw CommunicationError( cc::error( "Failed to write new value" ) );
    }

    m_checksum = m_checksum + oldValue - newValue;
}

void DataMemory::rewriteH2( uint8_t offset, uint16_t oldValue, uint16_t newValue ) {
    Log::trace( cc::trace( "Rewrite H2 at " ) + cc::num10( m_subclassId ) + cc::trace( "/" ) + cc::num10( m_blockIdx ) +
                cc::trace( "/" ) + cc::num10( offset ) + cc::trace( ": " ) + cc::num0x( oldValue ) +
                cc::trace( " -> " ) + cc::num0x( newValue ) );
    newValue = ( ( newValue & 0x00FFu ) << 8u ) | ( ( newValue & 0xFF00u ) >> 8u );
    if( m_chip->writeUint16( address( offset ), newValue ) < 0 ) {
        throw CommunicationError( cc::error( "Failed to write new value" ) );
    }

    uint8_t oldChecksum = m_checksum;
    m_checksum = m_checksum + ( ( ( oldValue & 0xff00u ) >> 8u ) + ( oldValue & 0xffu ) ) -
                 ( ( ( newValue & 0xff00u ) >> 8u ) + ( newValue & 0xffu ) );

    Log::trace( cc::trace( "Update checksum: " ) + cc::num0x( oldChecksum ) + cc::trace( " -> " ) +
                cc::num0x( m_checksum ) );
}

float DataMemory::readF4( uint8_t offset ) const {
    auto val = readH4( offset );
    // Convert from MBF to IEEE 754 single-precision format
    val = ( ( val & 0x00800000u ) << 8u ) | ( ( val & 0xFF000000 ) >> 1u ) | ( val & 0x007FFFFFu );
    return reinterpret_cast< float& >( val );
}

uint8_t DataMemory::readH1( uint8_t offset ) const {
    return m_chip->readUint8( address( offset ) );
}

uint16_t DataMemory::readH2( uint8_t offset ) const {
    uint16_t val = m_chip->readUint16( address( offset ) );
    return ( ( val & 0x00FFu ) << 8u ) | ( ( val & 0xFF00u ) >> 8u );
}

uint32_t DataMemory::readH4( uint8_t offset ) const {
    uint16_t msb = readU2( offset );
    uint16_t lsb = readU2( offset + 2 );
    return msb << 16u | lsb;
}

uint8_t DataMemory::readU1( uint8_t offset ) const {
    return readH1( offset );
}

uint16_t DataMemory::readU2( uint8_t offset ) const {
    return readH2( offset );
}

int8_t DataMemory::readI1( uint8_t offset ) const {
    return static_cast< int8_t >( readH1( offset ) );
}

int16_t DataMemory::readI2( uint8_t offset ) const {
    return static_cast< int16_t >( readH2( offset ) );
}

uint8_t DataMemory::address( uint8_t offset ) const {
    return static_cast< uint8_t >( ExtendedCommand::BlockData ) + ( offset % 32 );
}

}  // namespace Bq27441g1
}  // namespace HardwareManager
}  // namespace HubPlatform
