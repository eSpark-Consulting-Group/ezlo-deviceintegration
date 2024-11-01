#include "vera_utils/provision_reader_base.h"
#include "common_utils/cpp14.h"

// This allows to turn on logging messages via spread
//#define EZLO_LOG_ENABLED

#ifdef EZLO_LOG_ENABLED
#include "log_utils/ezlo_log.h"
#endif

#include <iostream>
#include <iomanip>
#include <fstream>
#include <map>
#include <algorithm>
#include <sstream>

#include <sys/ioctl.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <mtd/mtd-user.h>

namespace HubPlatform {
namespace VeraProvision {

ProvisionReaderBase::Data::Data( std::string&& rawData )
: m_rawData( std::move( rawData ) ) {}

const char* ProvisionReaderBase::Data::getData( size_t offset, size_t length ) const {
    size_t end = offset + length;
    if( end >= m_rawData.size() )
        throw ReaderException( "Requested value is out of range" );

    return m_rawData.data() + offset;
}

std::string ProvisionReaderBase::Data::getAsciiString( size_t offset, size_t length ) const {
    constexpr size_t MaxAsciiLength = 256;
    if( length > MaxAsciiLength )
        length = MaxAsciiLength;

    const char* value = getData( offset, length );

    std::string stringBuf( length, '\0' );
    char* buf = &stringBuf[ 0 ];

    size_t actualLength = 0;
    for( size_t i = 0; i < length; ++i ) {
        char ch = value[ i ];
        if( isascii( ch ) && isprint( ch ) ) {
            buf[ i ] = ch;
            ++actualLength;
        }
        else
            break;
    }

    stringBuf.resize( actualLength );
    return stringBuf;
}

void ProvisionReaderBase::readAll( const std::string& devName ) {
    const std::string mtdFileName = findMtd( devName );
    if( mtdFileName.empty() ) {
        throw ReaderException( "Could not find device named " + devName );
    }

    constexpr size_t maxLength = 256 * 1024;
    std::string rawData = readAllFromDevice( mtdFileName, maxLength );
    m_data = Data( std::move( rawData ) );
}

ProvisionReaderBase::MacAddress ProvisionReaderBase::getMacAddress() const {
    const auto info = getInfoMacAddress();
    auto value = reinterpret_cast< const MacAddress::value_type* >( m_data.getData( info.offset, info.length ) );
    MacAddress macAddress{ value[ 0 ], value[ 1 ], value[ 2 ], value[ 3 ], value[ 4 ], value[ 5 ] };
    return macAddress;
}

std::string ProvisionReaderBase::getMacAddressString() const {
    const auto mac = getMacAddress();
    std::stringstream ss;
    ss << std::uppercase << std::hex << std::setfill( '0' );
    for( size_t i = 0; i < mac.size(); i++ ) {
        if( i )
            ss << ":";
        ss << std::setw( 2 ) << static_cast< unsigned >( mac[ i ] );
    }
    return ss.str();
}

std::string ProvisionReaderBase::getHwKey() const {
    const auto info = getInfoHwKey();
    return m_data.getAsciiString( info.offset, info.length );
}

std::string ProvisionReaderBase::getSerial() const {
    const auto info = getInfoSerialNumber();
    return m_data.getAsciiString( info.offset, info.length );
}

std::string ProvisionReaderBase::getPassPhrase() const {
    const auto info = getInfoPassPhrase();
    return m_data.getAsciiString( info.offset, info.length );
}

std::string ProvisionReaderBase::getCountry() const {
    const auto info = getInfoWifiDomain();
    const char* value = m_data.getData( info.offset, info.length );
    const std::map< char, std::string > countryMap = {
        { 0x10, "USA" },    { 0x20, "Canada" },    { 0x30, "Europe" },       { 0x31, "Spain" },
        { 0x32, "France" }, { 0x33, "UK" },        { 0x34, "Germany" },      { 0x35, "Australia" },
        { 0x40, "Japan" },  { 0x41, "Japan All" }, { 0x70, "Simp Chinese" }, { 0x80, "Trad Chinese" },
        { 0x90, "Korea" },
    };

    auto it = countryMap.find( *value );
    if( it == countryMap.end() )
        return {};
    return it->second;
}

std::string ProvisionReaderBase::getLanguage() const {
    const auto info = getInfoLanguage();
    const char* value = m_data.getData( info.offset, info.length );
    const std::map< char, std::string > languageMap = {
        { 0x01, "English" },  { 0x02, "French" },  { 0x03, "German" },       { 0x04, "Swedish" },
        { 0x05, "Spanish" },  { 0x06, "Italian" }, { 0x15, "Simp Chinese" }, { 0x16, "Trad Chinese" },
        { 0x18, "Japanese" }, { 0x19, "Korean" },
    };

    auto it = languageMap.find( *value );
    if( it == languageMap.end() )
        return {};
    return it->second;
}

std::string ProvisionReaderBase::getWifiSsid() const {
    const auto info = getInfoWifiSsid();
    return m_data.getAsciiString( info.offset, info.length );
}

std::string ProvisionReaderBase::findMtd( const std::string& mtdName ) {
    const std::string fileName = "/proc/mtd";

    std::ifstream procMtd( fileName );
    if( !procMtd.is_open() ) {
        throw ReaderException( "Unable to open file " + fileName );
    }

    std::string line;
    while( getline( procMtd, line ) ) {
        // Dev name to lowercase
        std::transform( line.begin(), line.end(), line.begin(), []( unsigned char c ) { return std::tolower( c ); } );

        if( line.find( mtdName ) == std::string::npos )
            continue;

        int mtdNum, size, eraseSize;
        int res = std::sscanf( line.c_str(), "mtd%d: %08x %08x", &mtdNum, &size, &eraseSize );
        if( res != 3 ) {
            throw ReaderException( "Unknown name format " + line );
        }

        std::string mtdFileName( "/dev/mtd" + std::to_string( mtdNum ) );
        struct stat s;
        int err = stat( mtdFileName.c_str(), &s );
        if( err < 0 )
            throw ReaderException( "Could not get stat for file " + mtdFileName );

        if( ( s.st_mode & S_IFBLK ) == 0 )
            throw ReaderException( "Wrong file type" );

        return mtdFileName;
    }
    return {};
}

std::string ProvisionReaderBase::readAllFromDevice( const std::string& fileName, size_t maxLen ) {
    int fileDescriptor = open( fileName.c_str(), 0 );
    if( fileDescriptor < 0 ) {
        throw ReaderException( "Unable to open file " + fileName );
    }

    try {
        mtd_info_t meminfo;
        int err = ioctl( fileDescriptor, MEMGETINFO, &meminfo );
        if( err ) {
            throw ReaderException( "Unable to get MTD information" );
        }
#ifdef EZLO_LOG_ENABLED
        Log::debug( "Received meminfo.size " + std::to_string( meminfo.size ) );
#endif

        std::string stringBuf( maxLen, '\0' );
        char* buf = &stringBuf[ 0 ];

        size_t done = 0;
        for( loff_t ofs = 0; ofs < meminfo.size; ofs += meminfo.erasesize ) {
            err = ioctl( fileDescriptor, MEMGETBADBLOCK, &ofs );
            if( err < 0 ) {
                throw ReaderException( "Unable to get bad block info, err " + std::to_string( err ) );
            }

            if( err == 1 ) {
#ifdef EZLO_LOG_ENABLED
                Log::warn( "Skipping bad block at " + std::to_string( ofs ) );
#endif
                continue;
            }

            size_t readlen;
            if( maxLen < meminfo.erasesize )
                readlen = maxLen;
            else
                readlen = meminfo.erasesize;

            size_t retlen = pread( fileDescriptor, buf + done, readlen, ofs );
            if( retlen != readlen ) {
                throw ReaderException( "MTD read failed" );
                break;
            }

            done += retlen;
            if( done == maxLen ) {
                break;
            }
        }

        if( done < maxLen ) {
#ifdef EZLO_LOG_ENABLED
            Log::warn( "Too many bad blocks" );
#endif
        }

        close( fileDescriptor );

        stringBuf.resize( done );
        return stringBuf;
    }
    catch( ... ) {
        close( fileDescriptor );
        throw;
    }
}

}  // namespace VeraProvision
}  // namespace HubPlatform
