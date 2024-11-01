#include "cameras/ffmpeg_helpers.h"

#include "log_utils/ezlo_log.h"
#include "tiny/tiny_consolecolors.h"

extern "C" {
#include <libavformat/avformat.h>
}

namespace HubPlatform {
namespace FFmpegHelpers {

std::string avStringError( int errnum ) {
    std::string buf( AV_ERROR_MAX_STRING_SIZE, '\0' );
    if( av_strerror( errnum, &buf[ 0 ], buf.capacity() ) )
        return {};
    buf.resize( strlen( buf.c_str() ) );
    return buf;
}

std::string formatAvErrorMessage( const std::string& messageBase, const std::string& avError ) {
    return messageBase + " [desc: " + avError + "]";
}

void logAvError( const std::string& messageBase, int errnum ) {
    std::string avErr = avStringError( errnum );
    std::string message = formatAvErrorMessage( messageBase, avErr );
    Log::error( cc::error( message ) );
}

void logAvError( const std::string& message ) {
    Log::error( cc::error( message ) );
}

Exception::Exception( std::string text )
: errorCode( AVERROR_UNKNOWN )
, m_text( std::move( text ) ) {}

Exception::Exception( std::string text, int errc )
: errorCode( errc )
, m_text( std::move( text ) ) {
    const std::string avErr = avStringError( errorCode );
    if( !avErr.empty() )
        m_text = formatAvErrorMessage( m_text, avErr );
}

const char* Exception::what() const noexcept {
    return m_text.c_str();
}

}  // namespace FFmpegHelpers
}  // namespace HubPlatform
