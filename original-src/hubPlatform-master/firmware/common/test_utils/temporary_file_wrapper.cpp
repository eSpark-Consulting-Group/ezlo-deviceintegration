
#include "test_utils/temporary_file_wrapper.h"

#include "test_utils/test_environment.h"

#include "system_utils/filesystem.h"
#include "system_utils/environment.h"

#include <unistd.h>
#include <memory>
#include <string.h>

namespace HubPlatform {
namespace Tests {

TemporaryFileWrapper::TemporaryFileWrapper() {
    filesystem::createFolder( getTestsPoligonFolder() );
    m_path = getTestsPoligonFolder() + "/" + Environment::getCurrentProcessName() + ".XXXXXX";

    std::unique_ptr< char[] > uniqueFilePath( new char[ m_path.size() + 1 ] );
    strcpy( uniqueFilePath.get(), m_path.c_str() );

    const auto fd = mkstemp( uniqueFilePath.get() );
    if( fd == -1 ) {
        throw TemporaryFileError( "Cannot create temp file" );
    }

    close( fd );
    m_path = uniqueFilePath.get();
}

TemporaryFileWrapper::~TemporaryFileWrapper() {
    remove( m_path.c_str() );
}

const std::string& TemporaryFileWrapper::getFilePath() const noexcept {
    return m_path;
}

}  // namespace Tests
}  // namespace HubPlatform
