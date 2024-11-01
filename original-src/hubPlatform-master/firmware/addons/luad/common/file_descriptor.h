#pragma once

/* @brief this class represents a file descriptor semantic (RAII over a UNIX file descriptor)
 *
 * @note may be used as a base-wrapper over OS-objects which can be represented
 * by a file descriptors
 * @note supports copy and move semantics
 * @note provides an operator int() conversion operator for usability
 */

namespace HubPlatform {

class FileDescriptor {
public:
    FileDescriptor();
    explicit FileDescriptor( int fd );
    virtual ~FileDescriptor();

    FileDescriptor( const FileDescriptor& rhs );
    FileDescriptor( FileDescriptor&& rhs );
    FileDescriptor& operator=( FileDescriptor rhs );

    void swap( FileDescriptor& lhs, FileDescriptor& rhs ) noexcept;

    operator int() const;
    operator bool() const;

    int release();

private:
    int m_fileDescriptor;
};

}  // namespace HubPlatform
