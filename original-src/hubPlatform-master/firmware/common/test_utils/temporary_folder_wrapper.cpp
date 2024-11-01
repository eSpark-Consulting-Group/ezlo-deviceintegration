
#include "test_utils/temporary_folder_wrapper.h"

#include "test_utils/test_environment.h"

#include "system_utils/environment.h"
#include "system_utils/filesystem.h"

#include <unistd.h>
#include <memory>
#include <cerrno>
#include <string.h>

#include <iostream>

namespace HubPlatform {
namespace Tests {

TemporaryFolderWrapper::TemporaryFolderWrapper()
: m_removeContent( true ) {
    filesystem::createFolder( getTestsPoligonFolder() );
    m_path = getTestsPoligonFolder() + "/" + Environment::getCurrentProcessName() + ".XXXXXX";

    std::unique_ptr< char[] > folderPathTemplate( new char[ m_path.size() + 1 ] );
    strcpy( folderPathTemplate.get(), m_path.c_str() );
    auto resultTemplatePtr = mkdtemp( folderPathTemplate.get() );

    if( resultTemplatePtr == nullptr ) {
        const auto errorCode = errno;
        std::string errorText =
            std::string( "TemporaryFolderWrapper: error: Could not generate unique name for tmp directory: " ) +
            ::strerror( errorCode );
        throw TemporaryFolderError( errorText );
    }

    m_path = folderPathTemplate.get();

    if( chdir( m_path.c_str() ) == -1 ) {
        throw TemporaryFolderError( std::string( "TemporaryFolderWrapper: error: Could not change dir on: " ) +
                                    m_path );
    }
}

TemporaryFolderWrapper::~TemporaryFolderWrapper() {
    chdir( "/tmp" );

    if( m_removeContent ) {
        if( !filesystem::deleteFolder( m_path ) ) {
            throw TemporaryFolderError( std::string( "TemporaryFolderWrapper: error: Could not delete folder: " ) +
                                        m_path );
        }
    }
}

TemporaryFolderWrapper::TemporaryFolderWrapper( TemporaryFolderWrapper&& source ) noexcept
: m_path( std::move( source.m_path ) )
, m_removeContent( static_cast< bool >( source.m_removeContent ) ) {}

TemporaryFolderWrapper& TemporaryFolderWrapper::operator=( TemporaryFolderWrapper&& source ) noexcept {
    m_path = std::move( source.m_path );
    m_removeContent = static_cast< bool >( source.m_removeContent );

    return *this;
}

void TemporaryFolderWrapper::keepContent() {
    m_removeContent = false;
}

const std::string& TemporaryFolderWrapper::getPath() const noexcept {
    return m_path;
}

}  // namespace Tests
}  // namespace HubPlatform
