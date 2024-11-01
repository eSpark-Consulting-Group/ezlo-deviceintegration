#include "cameras/storages/cloud_file_sender.h"
#include "log_utils/ezlo_log.h"
#include "json_utils/json_utils.h"

namespace HubPlatform {

CloudFileSender::CloudFileSender( const FilePath& filePath,
                                  FileRemoverSharedPtr remover,
                                  CloudStorageContextPtr cloudStorageContext,
                                  SaveFileCallback saveFile )
: m_filePath( filePath )
, m_output( filePath )
, m_saveFile( std::move( saveFile ) )
, m_storageContext( std::move( cloudStorageContext ) )
, m_remover( std::move( remover ) ) {
    if( !m_output ) {
        throw FileSenderException( "Error opening file " + filePath );
    }
}

CloudFileSender::~CloudFileSender() {
    m_output.close();
    if( !m_isFinalized ) {
        return;
    }
    if( !m_resultCallback ) {
        LOGGER_ERROR( "no result callback" );
        return;
    }
    if( !m_isValid ) {
        m_resultCallback( createError( "sender not valid" ) );
        return;
    }
    if( !m_saveFile ) {
        m_resultCallback( createError( "no file saving callback" ) );
        return;
    }
    Json metaData = mergeMetaData( m_storageContext->metaData, m_dynamicMetaData );
    m_saveFile( m_filePath, m_resultCallback, getRemover(), metaData );
}

bool CloudFileSender::finalizeFile() {
    m_output.close();
    m_isFinalized = true;
    return true;
}

bool CloudFileSender::sendData( uint8_t* data, size_t size ) {
    m_output.write( reinterpret_cast< char* >( data ), size );
    return true;
}

FileRemoverSharedPtr CloudFileSender::getRemover() {
    return m_remover;
}

void CloudFileSender::setDynamicMetaData( Json metaData ) {
    m_dynamicMetaData = std::move( metaData );
    if( !m_dynamicMetaData.is_object() && !m_dynamicMetaData.is_null() ) {
        Log::error( cc::error( "Unsupported json is passed as a file's dynamic metadata, it will be ignored: " ) +
                    cc::j( m_dynamicMetaData ) );
    }
}

Json CloudFileSender::mergeMetaData( const Json& userMetaData, const Json& dynamicMetaData ) {
    Json result = userMetaData;

    const bool userMetaDataValid = userMetaData.is_object() || userMetaData.is_null();
    if( userMetaDataValid && dynamicMetaData.is_object() ) {
        for( auto it = dynamicMetaData.begin(); it != dynamicMetaData.end(); ++it ) {
            result[ it.key() ] = it.value();
        }
    }

    return result;
}

Json CloudFileSender::createError( const std::string& errorMessage ) {
    return JSON( "error" << JSON( "message" << errorMessage ) );
}

}  // namespace HubPlatform
