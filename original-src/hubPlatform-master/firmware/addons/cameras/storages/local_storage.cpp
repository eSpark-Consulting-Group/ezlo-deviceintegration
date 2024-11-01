#include "cameras/storages/local_storage.h"
#include "cameras/storages/local_file_sender.h"
#include "common_utils/cpp14.h"
#include "log_utils/ezlo_log.h"

#include <tiny/tiny_consolecolors.h>

namespace HubPlatform {

LocalStorage::LocalStorage( std::string folderPath )
: m_path( std::move( folderPath ) ) {}

void LocalStorage::setProperties( const JsonObject& properties ) {
    if( !properties.is_object() ) {
        throw StorageException( "Properties is not a json object" );
    }
    auto pathIt = properties.find( "path" );
    if( pathIt == properties.end() || !pathIt->is_string() ) {
        throw StorageException( "\"path\" is not found or not a string" );
    }
    m_path = pathIt.value().get< std::string >();
}

FileSenderSharedPtr LocalStorage::createFileSender( const std::string& fileName ) {
    try {
        auto sender = std::make_unique< LocalFileSender >( m_path + "/" + fileName );
        return std::move( sender );
    }
    catch( std::exception& ex ) {
        Log::error( cc::error( std::string( "Creation of LocalFileSender failed: " ) + ex.what() ) );
        return {};
    }
}

std::string LocalStorage::getPath() const {
    return m_path;
}
}  // namespace HubPlatform
