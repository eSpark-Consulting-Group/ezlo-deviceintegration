
#include "zwave/znet_store.h"

#include "log_utils/ezlo_log.h"
#include "tiny/tiny_consolecolors.h"

#include <cstring>
#include <memory>

namespace HubPlatform {
namespace Zwave {

using FilePtr = std::unique_ptr< FILE, std::function< void( FILE* ) > >;

namespace {
FilePtr openStoreFile( const std::string& storePath, const char* mode ) {
    return FilePtr( fopen( storePath.c_str(), mode ), []( FILE* file ) {
        if( file )
            fclose( file );
    } );
}
}  // namespace

ZNetStore::ZNetStore( const std::string& storePath )
: m_storePath( storePath )
, m_storeFile( storePath.substr( storePath.find_last_of( '/' ) + 1 ) ) {}

bool ZNetStore::save( std::size_t offset, const void* data, std::size_t size ) const {
    if( !data || !size ) {
        Log::error( cc::error( "ZNet store save: invalid args!" ) );
        return false;
    }

    auto f = openStoreFile( m_storePath, "rb+" );
    if( !f ) {
        Log::error( cc::error( "open store for write fail: " ) + cc::p( m_storeFile ) );
        return false;
    }

    if( fseek( f.get(), offset, SEEK_SET ) != 0 && ferror( f.get() ) ) {
        Log::error( cc::error( "seek store for write fail!" ) );
        return false;
    }

    if( fwrite( data, size, 1, f.get() ) != 1 ) {
        Log::error( cc::error( "write block to store fail!" ) );
        return false;
    }

    return true;
}

bool ZNetStore::load( std::size_t offset, void* data, std::size_t size ) const {
    if( !data || !size ) {
        Log::error( cc::error( "ZNet store load: invalid args!" ) );
        return false;
    }

    auto f = openStoreFile( m_storePath, "rb+" );
    if( !f ) {
        Log::error( cc::error( "open store for read fail: " ) + cc::p( m_storePath ) );
        return false;
    }

    if( fseek( f.get(), offset, SEEK_SET ) != 0 && ferror( f.get() ) ) {
        Log::error( cc::error( "seek store for read fail!" ) );
        return false;
    }

    if( fread( data, size, 1, f.get() ) != 1 ) {
        if( !ferror( f.get() ) && feof( f.get() ) ) {
            memset( data, 0, size );
            return true;
        }

        Log::error( cc::error( "read block from store fail!" ) );
        return false;
    }

    return true;
}

bool ZNetStore::reset() const {
    auto f = openStoreFile( m_storePath, "wb" );
    if( !f ) {
        Log::error( cc::error( "open store for reset fail!" ) );
        return false;
    }

    return true;
}

std::string ZNetStore::file() {
    return m_storeFile;
}

std::string ZNetStore::path() {
    return m_storePath;
}

}  // namespace Zwave
}  // namespace HubPlatform
