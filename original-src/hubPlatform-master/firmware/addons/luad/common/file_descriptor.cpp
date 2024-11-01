#include "luad/common/file_descriptor.h"

#include <unistd.h>
#include <algorithm>

namespace HubPlatform {

FileDescriptor::FileDescriptor()
: m_fileDescriptor{ -1 } {}

FileDescriptor::FileDescriptor( int fd )
: m_fileDescriptor{ fd } {}

FileDescriptor::~FileDescriptor() {
    if( m_fileDescriptor >= 0 )
        close( m_fileDescriptor );
}

FileDescriptor::FileDescriptor( const FileDescriptor& rhs ) {
    m_fileDescriptor = dup( rhs.m_fileDescriptor );
}

FileDescriptor::FileDescriptor( FileDescriptor&& rhs )
: FileDescriptor() {
    swap( *this, rhs );
}

FileDescriptor& FileDescriptor::operator=( FileDescriptor rhs ) {
    swap( *this, rhs );

    return *this;
}

void FileDescriptor::swap( FileDescriptor& lhs, FileDescriptor& rhs ) noexcept {
    using std::swap;

    swap( lhs.m_fileDescriptor, rhs.m_fileDescriptor );
}

FileDescriptor::operator int() const {
    return m_fileDescriptor;
}

FileDescriptor::operator bool() const {
    return m_fileDescriptor >= 0;
}
int FileDescriptor::release() {
    int tmp = -1;
    std::swap( m_fileDescriptor, tmp );
    return tmp;
}

}  // namespace HubPlatform
