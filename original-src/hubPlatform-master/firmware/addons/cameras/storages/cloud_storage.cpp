#include "cameras/storages/cloud_storage.h"
#include "common_utils/cpp14.h"
#include "log_utils/ezlo_log.h"
#include "system_utils/filesystem.h"
#include "json_utils/json_utils.h"

namespace HubPlatform {

CloudStorage::CloudStorage( const std::string& folderPath )
: m_folderPath( folderPath )
, m_cloudStorageContext( std::make_shared< CloudStorageContext >() ) {
    filesystem::createFolder( folderPath );
}

void CloudStorage::setProperties( const JsonObject& properties ) {
    if( !properties.is_object() ) {
        throw StorageException( "Properties is not a json object" );
    }
    auto pathIt = properties.find( "path" );
    if( pathIt == properties.end() || !pathIt->is_string() ) {
        throw StorageException( "\"path\" is not found or not a string" );
    }
    // m_folderPath = pathIt.value().get< std::string >();
}

FileSenderSharedPtr CloudStorage::createFileSender( const std::string& fileName ) {
    try {
        if( m_folderPath.empty() ) {
            Log::error( cc::error( "Cloud Storage empty folder path" ) );
        }
        auto filePath = m_folderPath + "/" + fileName;
        auto remover = std::make_shared< FileRemover >( filePath, getStorageVolume() );
        return std::make_shared< CloudFileSender >( filePath, remover, m_cloudStorageContext, m_saveFile );
    }
    catch( std::exception& ex ) {
        Log::error( cc::error( std::string( "Creation of CloudFileSender failed: " ) + ex.what() ) );
        return {};
    }
}

void CloudStorage::clearStorage() {
    filesystem::clearFolder( m_folderPath );
}

void CloudStorage::setSaveFileCallback( SaveFileCallback saveFile ) {
    m_saveFile = std::move( saveFile );
}

std::string CloudStorage::getPath() const {
    return m_folderPath;
}

void CloudStorage::setMetaData( const Json& metaData ) {
    m_cloudStorageContext->metaData = metaData;
}

}  // namespace HubPlatform
