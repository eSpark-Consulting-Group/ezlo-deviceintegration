#include "base_utils/lzo_utils.h"
#include "log_utils/ezlo_log.h"

#include <fstream>
#include <minilzo.h>

namespace common {
namespace lzo_utils {

namespace {
constexpr size_t MaxReadBufferSize = 6000000;
}

bool decompressFile( const std::string& filePath, std::string& outData ) {
    std::ifstream file( filePath );
    if( !file.is_open() ) {
        Log::error( "Could not open file " + filePath );
        return false;
    }
    std::string data{ std::istreambuf_iterator< char >( file ), std::istreambuf_iterator< char >() };
    if( !decompressData( data, data ) )
        return false;

    outData = std::move( data );
    return true;
}

bool decompressData( const std::string& data, std::string& outData ) {
    // Seems that lzo stores in first 4 bytes size of uncompressed data.
    const uint32_t uncompressedDataSize = *( reinterpret_cast< const uint32_t* >( data.c_str() ) );
    if( MaxReadBufferSize < uncompressedDataSize ) {
        Log::error( "File is too big " + std::to_string( uncompressedDataSize ) );
        return false;
    }

    lzo_uint decompressedSize = uncompressedDataSize;
    std::string tempBuf( decompressedSize, '\0' );
    int result = lzo1x_decompress_safe(
        reinterpret_cast< lzo_byte* >( const_cast< char* >( data.c_str() ) ) + sizeof( uncompressedDataSize ),
        data.size() - sizeof( uncompressedDataSize ),
        reinterpret_cast< lzo_byte* >( &tempBuf[ 0 ] ),
        &decompressedSize,
        0 );

    // Ignoring LZO_E_INPUT_NOT_CONSUMED because this error probably is related to buffer alignment.
    if( ( result != LZO_E_OK && result != LZO_E_INPUT_NOT_CONSUMED ) || decompressedSize == 0 ) {
        Log::error( "lzo1x_decompress() failed with result " + std::to_string( result ) );
        return false;
    }

    outData = std::move( tempBuf );
    return true;
}

}  // namespace lzo_utils
}  // namespace common
