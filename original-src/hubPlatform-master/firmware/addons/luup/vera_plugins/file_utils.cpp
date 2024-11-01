
#include "luup/vera_plugins/file_utils.h"
#include "log_utils/ezlo_log.h"
#include "base_utils/lzo_utils.h"

#include <fstream>
#include <sys/stat.h>

namespace FileUtils {

void makeDir( std::string directory ) {
    if( !directory.empty() && ( directory.back() != '/' ) ) {
        directory.append( "/" );
    }

    std::string::size_type pos = 0;
    while( ( pos = directory.find( '/', pos + 1 ) ) != std::string::npos ) {
        mkdir( directory.substr( 0, pos ).c_str(), 0777 );
    }
}

std::string getDir( const std::string& absoluteFileName ) {
    std::string::size_type pos = absoluteFileName.rfind( '/' );
    if( pos != std::string::npos && pos != ( absoluteFileName.length() - 1U ) ) {
        return absoluteFileName.substr( 0, pos + 1 );
    }
    return {};
}

std::string readFile( const std::string& fileName ) {
    std::string outData;
    std::ifstream file( fileName );
    if( !file.is_open() ) {
        Log::error( "Could not open file " + fileName );
        return {};
    }
    outData = { std::istreambuf_iterator< char >( file ), std::istreambuf_iterator< char >() };

    // Find extension.
    size_t pos = fileName.rfind( '.' );
    if( pos != std::string::npos ) {
        std::string extension = fileName.substr( pos );
        bool encrypted = extension == ".lzoenc";
        bool compressed = encrypted || ( extension == ".lzo" );
        if( encrypted )
            decryptFile( outData );
        if( compressed )
            if( !common::lzo_utils::decompressData( outData, outData ) )
                return {};
    }

    return outData;
}

bool dirExists( const std::string& dirName ) {
    struct stat buf;
    int result;
    if( dirName.length() && dirName.back() == '/' ) {
        result = stat( dirName.substr( 0, dirName.length() - 1 ).c_str(), &buf );
    }
    else {
        result = stat( dirName.c_str(), &buf );
    }
    return result == 0 && buf.st_mode & S_IFDIR;
}

void decryptFile( std::string& inOutData ) {
    static const unsigned char lek[] = {
        8,   219, 100, 200, 18,  238, 64,  210, 88,  31,  39,  70,  207, 80,  143, 216, 156, 227, 112, 193, 19,  76,
        170, 69,  93,  182, 38,  206, 53,  138, 120, 196, 136, 17,  12,  146, 130, 1,   154, 240, 208, 68,  4,   241,
        160, 173, 62,  221, 38,  252, 118, 100, 37,  231, 21,  50,  91,  112, 210, 100, 59,  98,  105, 240, 217, 36,
        56,  127, 115, 193, 82,  205, 130, 88,  61,  7,   176, 209, 8,   59,  172, 14,  205, 99,  51,  100, 92,  5,
        64,  177, 48,  2,   202, 196, 76,  144, 244, 101, 54,  234, 27,  210, 181, 192, 170, 231, 114, 234, 20,  190,
        44,  115, 237, 28,  166, 107, 116, 93,  241, 167, 140, 58,  69,  222, 122, 37,  4,   57,  91,  141, 25,  212,
        181, 196, 90,  70,  12,  227, 9,   167, 233, 91,  51,  44,  46,  149, 251, 209, 39,  85,  141, 43,  72,  48,
        218, 217, 238, 225, 189, 167, 25,  188, 191, 231, 131, 12,  30,  7,   245, 167, 49,  242, 86,  120, 235, 56,
        62,  78,  139, 148, 147, 181, 51,  205, 3,   36,  131, 122, 206, 61,  235, 191, 193, 125, 205, 116, 209, 9,
        39,  209, 57,  247, 225, 37,  134, 205, 109, 143, 115, 131, 225, 89,  7,   208, 43,  237, 215, 11,  154, 241,
        204, 100, 73,  141, 108, 192, 175, 116, 242, 0,   227, 155, 52,  254, 216, 69,  175, 251, 25,  82,  172, 218,
        76,  110, 220, 223, 8,   24,  99,  70,  66,  20,  161, 106, 221, 70
    };
    int ptr = 0;
    const int size = inOutData.size();
    unsigned char round = 5, byte;

    while( ptr < size ) {
        for( int i = 0; i < 256; ++i ) {
            byte = lek[ round ] ^ lek[ i ];
            inOutData[ ptr ] ^= byte;
            if( ptr++ == size )
                break;
        }
        round++;
    }
}

XmlDocument parseXmlFile( const std::string& fileName ) {
    const std::string fileData = readFile( fileName );
    if( fileData.empty() )
        return {};

    return XmlDocument( fileData );
}

}  // namespace FileUtils
