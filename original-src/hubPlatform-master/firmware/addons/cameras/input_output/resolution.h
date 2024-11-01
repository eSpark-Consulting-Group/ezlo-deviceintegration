#pragma once

struct AVStream;

namespace HubPlatform {

struct Resolution {
    Resolution() = default;
    Resolution( int width, int height );

    Resolution( const Resolution& ) = default;
    Resolution& operator=( const Resolution& ) = default;

    bool operator==( const Resolution& rhs ) const;
    bool operator!=( const Resolution& rhs ) const;

    static Resolution fromStream( const AVStream& stream );

    int width = 0;
    int height = 0;
};

}  // namespace HubPlatform
