#include "opkg_utils/keys/keys_manager.h"

#include <dirent.h>
#include <fstream>

namespace HubPlatform {
namespace Opkg {

KeysManager::KeysManager( std::string keysDirectoryPath )
: m_keysDirectoryPath( std::move( keysDirectoryPath ) ) {}

bool KeysManager::forEachFingerprint( FingerprintCallback callback ) const {
    DIR* dir = ::opendir( m_keysDirectoryPath.c_str() );
    if( dir == nullptr )
        return true;

    struct dirent* entry = nullptr;
    while( ( entry = ::readdir( dir ) ) != nullptr ) {
        if( entry->d_type != DT_REG )
            continue;

        if( !callback( entry->d_name ) )
            return false;
    }

    ::closedir( dir );
    return true;
}

KeysManager::Fingerprints KeysManager::getFingerprints() const {
    Fingerprints fingerprints;
    forEachFingerprint( [&]( std::string fingerprint ) {
        fingerprints.emplace_back( std::move( fingerprint ) );
        return true;
    } );
    return fingerprints;
}

KeyFile KeysManager::getKeyFile( const std::string& fingerprint ) const {
    std::ifstream stream( getKeyFilePath( fingerprint ) );

    if( !stream.is_open() )
        return {};

    std::string string;

    stream.seekg( 0, std::ios::end );
    string.reserve( stream.tellg() );
    stream.seekg( 0, std::ios::beg );

    string.assign( ( std::istreambuf_iterator< char >( stream ) ), std::istreambuf_iterator< char >() );

    return KeyFile::fromString( fingerprint, string );
}

bool KeysManager::addKeyFile( KeyFile key ) {
    std::ofstream stream( getKeyFilePath( key.fingerprint ) );
    if( !stream.is_open() )
        return false;

    stream << KeyFile::toString( key );
    return true;
}

bool KeysManager::removeKeyFile( std::string fingerprint ) {
    auto result = ::remove( getKeyFilePath( fingerprint ).c_str() );
    return result == 0;
}

bool KeysManager::clearKeyFiles() {
    return forEachFingerprint( [&]( const std::string& fingerprint ) { return removeKeyFile( fingerprint ); } );
}

std::string KeysManager::getKeyFilePath( const std::string& fingerprint ) const {
    return std::string( m_keysDirectoryPath ) + "/" + fingerprint;
}

}  // namespace Opkg
}  // namespace HubPlatform