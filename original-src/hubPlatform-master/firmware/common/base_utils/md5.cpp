#include "md5.h"
#include "utils.h"

#include <fstream>

const size_t MD5_LENGTH = 16;

Md5::Md5() {
    websocketpp::md5::md5_init( &md5State_ );
}

void Md5::append( std::ifstream& stream ) {
    unsigned char buf[ 1024 ];
    int len;
    while( stream.good() ) {
        stream.read( (char*) buf, 1024 );  // note that return value of read is unusable.
        len = (int) stream.gcount();
        append( buf, len );
    }
}

void Md5::append( const std::string& str ) {
    websocketpp::md5::md5_append( &md5State_, reinterpret_cast< const unsigned char* >( str.c_str() ), str.size() );
}

void Md5::append( const std::vector< unsigned char >& data ) {
    websocketpp::md5::md5_append( &md5State_, &data[ 0 ], data.size() );
}

void Md5::append( const unsigned char* buff, int size ) {
    websocketpp::md5::md5_append( &md5State_, buff, size );
}

std::vector< unsigned char > Md5::getBin() {
    std::vector< unsigned char > md5( MD5_LENGTH );
    websocketpp::md5::md5_finish( &md5State_, md5.data() );
    websocketpp::md5::md5_init( &md5State_ );
    return md5;
}

std::string Md5::get() {
    std::vector< unsigned char > md5 = getBin();
    return std::string( md5.cbegin(), md5.cend() );
}

std::string Md5::getHex() {
    std::vector< unsigned char > md5( MD5_LENGTH );
    websocketpp::md5::md5_finish( &md5State_, md5.data() );
    websocketpp::md5::md5_init( &md5State_ );
    return getHexString( md5, true );
}

std::string Md5::getHex( const std::string& name,
                         const std::string& lenStr,
                         const std::vector< unsigned char >& buff ) {
    Md5 md5;
    md5.append( name );
    md5.append( lenStr );
    md5.append( buff );
    return md5.getHex();
}

std::vector< unsigned char > Md5::getBin( const std::vector< unsigned char >& buff ) {
    Md5 md5;
    md5.append( buff );
    return md5.getBin();
}

size_t Md5::getBinSize() {
    return MD5_LENGTH;
}
