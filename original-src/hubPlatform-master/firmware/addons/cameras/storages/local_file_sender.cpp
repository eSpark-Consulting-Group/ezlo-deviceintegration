#include "cameras/storages/local_file_sender.h"
#include "json_utils/json_utils.h"

namespace HubPlatform {

LocalFileSender::LocalFileSender( const std::string& filePath )
: m_output( filePath )
, m_filePath( filePath ) {
    if( !m_output ) {
        throw FileSenderException( "Error opening file " + filePath );
    }
}

bool LocalFileSender::finalizeFile() {
    if( !m_output ) {
        return false;
    }
    m_output.close();
    if( m_resultCallback ) {
        m_resultCallback( JSON( "filePath" << m_filePath ) );
    }
    return true;
}

bool LocalFileSender::sendData( uint8_t* data, size_t size ) {
    if( !m_output ) {
        return false;
    }
    m_output.write( reinterpret_cast< char* >( data ), size );
    return true;
}

FileRemoverSharedPtr LocalFileSender::getRemover() {
    return std::make_shared< EmptyFileRemover >();
}

}  // namespace HubPlatform
