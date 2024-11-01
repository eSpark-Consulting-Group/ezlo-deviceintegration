
#include "base_utils/utils.h"
#include "log_utils/ezlo_log.h"

#include <fstream>
#include <iomanip>
#include <map>
#include <mutex>
#include <set>

#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>

#include <algorithm>
#include <iostream>

#include <plog/Severity.h>
#include <tiny/tiny_consolecolors.h>
#include <tiny/tiny_utils.h>

std::string time_str( time_t now ) {
    std::string t( ctime( &now ) );
    t.erase( t.size() - 1 );
    return t;
}

std::string hexToString( const unsigned char* data, size_t size, bool upperCase ) {
    std::ostringstream result;
    result << std::hex << std::setfill( '0' ) << ( upperCase ? std::uppercase : std::nouppercase );
    for( size_t i = 0; i < size; ++i ) {
        result << std::setw( 2 ) << (int) data[ i ];
    }
    return result.str();
}

std::string intToHexString( int value, int width ) {
    std::stringstream stream;
    stream << std::hex << std::setfill( '0' ) << std::setw( width ) << value;
    return stream.str();
}

std::string ullToHexString( unsigned long long value, int width ) {
    std::stringstream stream;
    stream << std::hex << std::setfill( '0' ) << std::setw( width ) << value;
    std::string result( stream.str() );

    return result;
}

std::string zwaveDskToString( const unsigned char* data ) {
    // DSK key is first 16 bytes from public key
    static constexpr size_t dskSizeWords = 0x08;

    uint16_t* pDskStart = (uint16_t*) data;
    uint16_t* pDskEnd = pDskStart + dskSizeWords;
    std::ostringstream ss;
    do {
        ss << htons( *pDskStart );
        ++pDskStart;
    } while( pDskStart < pDskEnd && ( ss << "-" ) );
    return std::string( ss.str() );
}

DSKAsInts getDSKAsArrayOfInts( const unsigned char* keyData, std::size_t size ) {
    DSKAsInts dsk;

    if( !keyData || size != 16 ) {
        Log::warn( "invalid dsk key" );
        return {};
    }

    for( auto i = 0; i < size; i += 2 ) {
        uint16_t two = 0;

        two = ( ( (uint16_t) keyData[ i ] ) << 8 ) | keyData[ i + 1 ];  // big endian
        dsk.emplace_back( two );
    }

    return dsk;
}

void tryToCall( std::function< void() > f, int times, const std::string& message ) {
    bool isOk = false;
    int tryCount = 0;
    while( !isOk && times-- ) {
        ++tryCount;
        try {
            f();
            isOk = true;
        }
        catch( const std::exception& ex ) {
            const char* strWhat = ex.what();
            if( strWhat == nullptr || strWhat[ 0 ] == '\0' )
                strWhat = "unknown";
            std::cerr << message << " Try Count: " << tryCount << ", Exception: " << strWhat << std::endl;
        }
        catch( ... ) {
            std::cerr << message << " Exception! Try Count: " << tryCount << std::endl;
        }
    }
}

bool waitForCondition( std::function< bool() > condition,
                       const std::chrono::milliseconds& timeout,
                       const std::chrono::milliseconds& checkInterval ) {
    const auto start = std::chrono::steady_clock::now();
    while( std::chrono::steady_clock::now() - start < timeout ) {
        if( condition() )
            return true;
        std::this_thread::sleep_for( checkInterval );
    }
    return false;
}

void replaceAll( std::string& str, const std::string& before, const std::string& after ) {
    std::size_t position = 0;
    while( true ) {
        position = str.find( before, position );
        if( position == std::string::npos )
            break;
        str.replace( position, before.size(), after );
    }
}

std::string getHostName() {
    const int HOSTNAME_MAX_LEN = 150;
    char hostName[ HOSTNAME_MAX_LEN ];
    gethostname( hostName, HOSTNAME_MAX_LEN );
    return std::string( hostName );
}

int64_t versionCalc( const std::string& version ) {
    const int maxDotsInVersionCount = 4;
    int64_t result = 0;
    std::string::size_type dotPosition = 0;
    try {
        for( int i = maxDotsInVersionCount; i; --i ) {
            result *= 1000;
            if( version.size() > dotPosition && std::isdigit( version[ dotPosition ] ) ) {
                std::string::size_type nextDotPosition = version.find( '.', dotPosition );
                if( nextDotPosition == std::string::npos )
                    nextDotPosition = version.size();
                result += std::stoi( version.substr( dotPosition, nextDotPosition - dotPosition ) );
                dotPosition = nextDotPosition + 1;
            }
        }
    }
    catch( ... ) {
        result = std::numeric_limits< int64_t >::max();
        std::cerr << "Exception handled ! versionCalc(): version - " << version << std::endl;
    }
    return result;
}

std::string getHexString( const std::vector< unsigned char >& data, bool upperCase ) {
    std::ostringstream result;
    result << std::hex << std::setfill( '0' ) << ( upperCase ? std::uppercase : std::nouppercase );
    for( const auto& dataByte : data )
        result << std::setw( 2 ) << (int) dataByte;
    return result.str();
}

std::string cutUriPort( const std::string& uri ) {
    const std::string::size_type dotsPosition = uri.rfind( ':' );
    const std::string::size_type slashPosition = uri.rfind( '/' );
    return ( dotsPosition > slashPosition ) ? uri.substr( 0, dotsPosition ) : uri;
}

bool hasSpecialChars( const std::string& input ) {
    static const std::set< char > illegalCharacters = {
        '/', '{', '}', '~', '%', '$', ':', ',', '#', '@', '+', '-', '&', '^', '!', ';', '`', '\t', '\\', '\'', '"',
    };
    for( auto& inputChar : input )
        if( illegalCharacters.count( inputChar ) )
            return true;
    return false;
}

std::string toUpperFront( const std::string& str ) {
    std::string res = str;
    if( !str.empty() ) {
        res.front() = static_cast< char >( toupper( str.front() ) );
    }

    return res;
}

std::string toLower( const std::string& str ) {
    return tiny::tools::to_lower( str );
}

std::string toUpper( const std::string& str ) {
    return tiny::tools::to_upper( str );
}

int hexToDec( unsigned char c ) {
    switch( c ) {
    default:
        return c - '0';
    case 'a':
    case 'A':
        return 10;
    case 'b':
    case 'B':
        return 11;
    case 'c':
    case 'C':
        return 12;
    case 'd':
    case 'D':
        return 13;
    case 'e':
    case 'E':
        return 14;
    case 'f':
    case 'F':
        return 15;
    }
}

bool getBinaryFromHex( const std::string& data, std::string& result ) {
    if( data.empty() )
        return false;
    const auto size = data.size();
    if( size % 2 == 1 )
        return false;

    result.reserve( size / 2 );
    result.resize( 0 );
    unsigned char octets[ 2 ] = { 0 };
    for( int i = 0; i < size; ++i ) {
        unsigned char decimal = hexToDec( data[ i ] );
        if( decimal < 0 || decimal > 0xF )
            return false;

        octets[ i % 2 ] = decimal;

        if( i % 2 == 1 ) {
            unsigned char byte = ( octets[ 0 ] << 4 | octets[ 1 ] );
            result.push_back( reinterpret_cast< char& >( byte ) );
        }
    }
    return true;
}

std::string getLogLevelQuery( const std::string& logLevel ) {
    std::string logLevelQuery = "";
    if( !logLevel.empty() ) {
        if( toUpper( logLevel ) == "TRACE" )
            logLevelQuery = "Level LIKE \'%FATAL%\' OR "
                            "Level LIKE \'%ERROR%\' OR "
                            "Level LIKE \'%WARN%\' OR "
                            "Level LIKE \'%INFO%\' OR "
                            "Level LIKE \'%DEBUG%\' OR "
                            "Level LIKE \'%TRACE%\'";
        else {
            int level = plog::severityFromString( toUpper( logLevel ).c_str() );
            switch( level ) {
            case Log::PriorityLevel::FATAL:
                logLevelQuery = "Level LIKE \'%FATAL%\'";
                break;
            case Log::PriorityLevel::ERROR:
                logLevelQuery = "Level LIKE \'%FATAL%\' OR "
                                "Level LIKE \'%ERROR%\'";
                break;
            case Log::PriorityLevel::WARN:
                logLevelQuery = "Level LIKE \'%FATAL%\' OR "
                                "Level LIKE \'%ERROR%\' OR "
                                "Level LIKE \'%WARN%\'";
                break;
            case Log::PriorityLevel::INFO:
                logLevelQuery = "Level LIKE \'%FATAL%\' OR "
                                "Level LIKE \'%ERROR%\' OR "
                                "Level LIKE \'%WARN%\' OR "
                                "Level LIKE \'%INFO%\'";
                break;
            case Log::PriorityLevel::DEBUG:
                logLevelQuery = "Level LIKE \'%FATAL%\' OR "
                                "Level LIKE \'%ERROR%\' OR "
                                "Level LIKE \'%WARN%\' OR "
                                "Level LIKE \'%INFO%\' OR "
                                "Level LIKE \'%DEBUG%\'";
                break;
            default:
                break;
            }
        }
    }

    return logLevelQuery;
}

void delayMilliseconds( int milliseconds ) {
    std::this_thread::sleep_for( std::chrono::milliseconds( milliseconds ) );
}

std::string escapeSymbolsByFirst( const std::string& source, const char* symbols, size_t escapesCount ) {
    std::string result;
    for( const auto c : source ) {
        if( strchr( symbols, c ) ) {
            result.append( escapesCount, *symbols );
        }

        result += c;
    }

    return result;
}

bool utils::writeToFile( const std::string& fileName, const std::string& text ) {
    std::ofstream file;
    file.open( fileName );
    if( file.good() ) {
        file << text;
        return true;
    }
    return false;
}

std::string utils::readPipe( const char* command ) {
    std::string result;

    FILE* pFile;
    char buffer[ 128 ];
    /* Open the command for reading. */
    pFile = popen( command, "r" );
    if( pFile == NULL ) {
        std::cerr << "Failed to run: \"" << command << "\"" << std::endl;
    }
    else {
        /* Read the output a line at a time - output it. */
        while( fgets( buffer, sizeof( buffer ) - 1, pFile ) != NULL ) {
            result.append( buffer );
        }
    }
    /* close */
    pclose( pFile );

    return result;
}

std::string utils::readPipe( const std::string& command ) {
    return readPipe( command.c_str() );
}

utils::AfterReturn::AfterReturn( Action action )
: m_action( std::move( action ) ) {}

utils::AfterReturn::~AfterReturn() {
    m_action();
}
