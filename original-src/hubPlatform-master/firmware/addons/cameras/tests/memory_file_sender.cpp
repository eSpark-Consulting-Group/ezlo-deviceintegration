#include "memory_file_sender.h"

namespace HubPlatform {

MemoryFileSender::MemoryFileSender( std::string fileName )
: m_fileName( std::move( fileName ) ) {}

bool MemoryFileSender::finalizeFile() {
    return true;
}

bool MemoryFileSender::sendData( uint8_t* data, size_t size ) {
    m_output.write( reinterpret_cast< char* >( data ), size );
    return true;
}

std::stringstream& MemoryFileSender::getOutputStream() {
    return m_output;
}

const std::stringstream& MemoryFileSender::getOutputStream() const {
    return m_output;
}
}  // namespace HubPlatform
