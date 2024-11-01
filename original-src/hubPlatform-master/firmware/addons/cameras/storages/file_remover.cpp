#include "cameras/storages/file_sender.h"
#include "log_utils/ezlo_log.h"

namespace HubPlatform {

std::atomic_int FileRemover::m_counter( 0 );

FolderRemover::FolderRemover( std::string folder )
: m_folderPath( std::move( folder ) ) {}

FolderRemover::~FolderRemover() {
    filesystem::deleteFolder( m_folderPath );
}

FileRemover::FileRemover( std::string filePath, std::atomic_llong& storageSize )
: m_filePath( std::move( filePath ) )
, m_storageSize( storageSize ) {
    ++m_counter;
    LOGGER_TRACE( "filePath: " + m_filePath + ", counter: " + std::to_string( m_counter ) );
}

FileRemover::~FileRemover() {
    --m_counter;
    LOGGER_TRACE( "filePath: " + m_filePath + ", counter: " + std::to_string( m_counter ) +
                  ", storageSize: " + std::to_string( m_storageSize ) );
    m_storageSize -= filesystem::getFileSize( m_filePath.c_str() );
    filesystem::removeFile( m_filePath.c_str() );
}

}  // namespace HubPlatform
