#include "system_utils/process/shared_data.h"
#include "system_utils/process/shared_ram_exception.h"

#include "system_utils/logger.h"

#include <string>

#include <errno.h>         // errno, ENOENT
#include <fcntl.h>         // O_RDWR, O_CREATE
#include <linux/limits.h>  // NAME_MAX
#include <sys/mman.h>      // shm_open, shm_unlink, mmap, munmap,
                           // PROT_READ, PROT_WRITE, MAP_SHARED, MAP_FAILED
#include <unistd.h>        // ftruncate, close

#include <iostream>
#include <string.h>

namespace HubPlatform {
namespace Process {

SharedData::SharedData() noexcept
: m_dataPtr( nullptr )
, m_dataSize( 0 ) {}

SharedData::SharedData( const SharedRamDescriptor& ramDescriptor, std::size_t size )
: m_dataSize( size ) {
    // Map pthread data into the shared memory.
    m_dataPtr = mmap( NULL, m_dataSize, PROT_READ | PROT_WRITE, MAP_SHARED, ramDescriptor, 0 );
    if( m_dataPtr == MAP_FAILED ) {
        throw SharedRamException( "Truncate shared RAM failure" );
    }
}

SharedData::SharedData( SharedData&& rhs ) noexcept {
    m_dataPtr = rhs.m_dataPtr;
    m_dataSize = rhs.m_dataSize;
    rhs.m_dataPtr = nullptr;
    rhs.m_dataSize = 0;
}

SharedData& SharedData::operator=( SharedData&& rhs ) {
    m_dataPtr = rhs.m_dataPtr;
    m_dataSize = rhs.m_dataSize;
    rhs.m_dataPtr = nullptr;
    rhs.m_dataSize = 0;
    return *this;
}

SharedData::~SharedData() {
    if( m_dataPtr != nullptr && munmap( m_dataPtr, m_dataSize ) ) {
        SystemUtils::logError() << "Unmap process shared data error: " << strerror( errno ) << std::endl;
    }
}

void* SharedData::get() const {
    return m_dataPtr;
}

std::size_t SharedData::size() const {
    return m_dataSize;
}

}  // namespace Process
}  // namespace HubPlatform
