#include "cameras/input_output/resolution.h"

extern "C" {
#include <libavformat/avformat.h>
}

namespace HubPlatform {

Resolution::Resolution( int width, int height )
: width( width )
, height( height ) {}

bool Resolution::operator==( const Resolution& rhs ) const {
    return width == rhs.width && height == rhs.height;
}

bool Resolution::operator!=( const Resolution& rhs ) const {
    return !( *this == rhs );
}

Resolution Resolution::fromStream( const AVStream& stream ) {
    return Resolution( stream.codecpar->width, stream.codecpar->height );
}

}  // namespace HubPlatform
