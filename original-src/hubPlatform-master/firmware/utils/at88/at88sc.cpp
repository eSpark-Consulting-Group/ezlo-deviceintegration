
#include "at88/at88sc.h"

#include <sstream>

At88sc::At88sc( const std::string& devPath ) {
    i2cControl = std::unique_ptr< i2c::I2cControl >( new i2c::I2cControl( devPath ) );
}

bool At88sc::write( i2c::Byte command, const i2c::ByteData& data ) {
    i2cControl->write( command >> 1, data );
    usleep( 20000 );
    return true;
}

bool At88sc::write( i2c::Byte command, const i2c::Byte msb, const i2c::Byte lsb, const std::string& data ) {
    if( data.size() > 31 )
        return false;
    const i2c::Byte maxMessageSize = 0x0F;
    const char* dataHex = data.c_str();
    int attemptCount = 0;
    for( i2c::Byte i = 0; i <= ( data.size() / maxMessageSize ); i++ ) {
        i2c::Byte currentSize = i * maxMessageSize;
        i2c::Byte currentEndSize = currentSize + maxMessageSize;
        if( currentEndSize > data.size() )
            currentEndSize = currentSize + ( static_cast< i2c::Byte >( data.size() ) - currentSize );
        i2c::ByteData dataVector( dataHex + currentSize, dataHex + currentEndSize );
        std::cerr << "s: " + std::string( dataVector.begin(), dataVector.end() ) + "\n";
        i2c::ByteData txData = {
            msb, static_cast< i2c::Byte >( lsb + currentSize ), static_cast< i2c::Byte >( dataVector.size() )
        };
        txData.insert( txData.end(), dataVector.begin(), dataVector.end() );
        if( !write( command, txData ) ) {
            i = 0;
            attemptCount++;
            if( attemptCount > 10 )
                return false;
        }
    }
    return true;
}

i2c::ByteData At88sc::read( i2c::Byte command, const i2c::Byte msb, const i2c::Byte lsb, const i2c::Byte size ) {
    return i2cControl->read( command >> 1, msb, lsb, size );
}

bool At88sc::writePassword() {
    write( 0xBA, defaulPassword );
    // call read commands to allow next operations
    usleep( 20000 );
    read( 0xB4, 0x00, 0x0A, 0x02 );
    usleep( 20000 );
    read( 0xB4, 0x00, 0x0A, 0x02 );
    usleep( 20000 );
    read( 0xB4, 0x00, 0x0A, 0x02 );
    usleep( 20000 );
    return read( 0xB4, 0x00, 0x0A, 0x02 ).size() == 2;
}

void At88sc::blowFuses() {
    usleep( 20000 );
    write( 0xB4, { 0x01, 0x06, 0x00 } );
    usleep( 20000 );
    write( 0xB4, { 0x01, 0x04, 0x00 } );
    usleep( 20000 );
    write( 0xB4, { 0x01, 0x00, 0x00 } );
    usleep( 20000 );
}

bool At88sc::saveSerial( const std::string& serial ) {
    const char* serialHex = serial.c_str();
    i2c::ByteData serialVector( serialHex, serialHex + serial.size() );
    i2c::ByteData txData = { 0x00, 0x40, 0x10 };
    txData.insert( txData.end(), serialVector.begin(), serialVector.end() );
    return write( 0xB4, txData );
}

std::string At88sc::getSerial() {
    i2c::ByteData data = read( 0xB4, 0x00, 0x0A, 0x46 );
    std::string out( data.begin() + 54, data.end() );
    for( char symbol : out ) {
        if( !isalpha( symbol ) && !isdigit( symbol ) )
            return "";
    }
    return out;
}

bool At88sc::saveMac( const std::string& mac ) {
    writePassword();
    const char* macHex = mac.c_str();
    i2c::ByteData macVector( macHex, macHex + mac.size() );
    // todo: set mac size;
    i2c::ByteData txData = { 0x00, 0x00, 0x0C };
    txData.insert( txData.end(), macVector.begin(), macVector.end() );
    write( 0xB4, { 0x03, 0x00, 0x00 } );
    return write( 0xB0, txData );
}

std::string At88sc::getMac() {
    writePassword();
    write( 0xB4, { 0x03, 0x00, 0x00 } );
    i2c::ByteData data = read( 0xB0, 0x00, 0x00, 0x0C );
    std::stringstream ss;
    std::string out( data.begin(), data.end() );
    ss << out[ 0 ];
    for( unsigned int i = 1; i < out.size(); i++ ) {
        if( i % 2 == 0 )
            ss << ':';
        ss << out[ i ];
    }
    return ss.str();
}

bool At88sc::saveRegion( const std::string& region ) {
    if( region.size() > 31 )
        return false;
    writePassword();
    write( 0xB4, { 0x03, 0x01, 0x00 } );
    return ( write( 0xB0, 0x00, 0x01, region ) && write( 0xB0, { 0x00, 0x00, 0x01, (i2c::Byte) region.size() } ) );
}

std::string At88sc::getRegion() {
    writePassword();
    write( 0xB4, { 0x03, 0x01, 0x00 } );
    i2c::ByteData size = read( 0xB0, 0x00, 0x00, 0x01 );
    i2c::ByteData data = read( 0xB0, 0x00, 0x01, size.at( 0 ) );
    return std::string( data.begin(), data.end() );
    ;
}
