#include "autoremove_file.h"

#include <iostream>
#include <cstring>

#include "system_utils/filesystem.h"

namespace HubPlatform {
namespace filesystem {

AutoremoveFile::AutoremoveFile( std::string filename, bool autoremove )
: m_filename( std::move( filename ) )
, m_autoremove( autoremove ) {}

AutoremoveFile::~AutoremoveFile() {
    try {
        if( m_autoremove ) {
            if( !filesystem::removeFile( m_filename.c_str() ) )
                std::cerr << "Failed to autoremove file (\"" << m_filename << "\")"
                          << " reason: " << strerror( errno ) << std::endl;
        }
    }
    catch( std::exception& e ) {
        std::cerr << "Exception during autoremoving file (\"" << m_filename << "\"): " << e.what() << std::endl;
    }
    catch( ... ) {
        std::cerr << "Unknown exception during autoremoving file (\"" << m_filename << "\")" << std::endl;
    }
}

void AutoremoveFile::setAutoremove( bool autoremove ) {
    m_autoremove = autoremove;
}

}  // namespace filesystem
}  // namespace HubPlatform
