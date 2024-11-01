#include "opkg_utils/keys/key_file.h"

#include <sstream>
#include <fstream>
#include <cstring>

namespace HubPlatform {
namespace Opkg {

namespace {
constexpr const char CommentPrefix[] = "untrusted comment: ";
}  // namespace

KeyFile KeyFile::fromString( std::string fingerprint, const std::string& string ) {
    if( strncmp( string.c_str(), CommentPrefix, strlen( CommentPrefix ) ) != 0 )
        return {};

    auto newLinePos = string.find( '\n' );
    if( newLinePos == std::string::npos )
        return {};

    std::string comment = string.substr( strlen( CommentPrefix ), newLinePos - strlen( CommentPrefix ) );
    if( comment.empty() )
        return {};

    std::string key = string.substr( newLinePos + 1 );

    if( key.empty() )
        return {};

    if( key.find( '\n' ) != std::string::npos )
        return {};

    return { std::move( fingerprint ), std::move( comment ), std::move( key ) };
}

std::string KeyFile::toString( const KeyFile& keyFile ) {
    if( !keyFile.isValid() )
        return {};

    return std::string( "untrusted comment: " ) + keyFile.comment + "\n" + keyFile.key;
}

bool KeyFile::isValid() const {
    return !fingerprint.empty();
}

}  // namespace Opkg
}  // namespace HubPlatform
