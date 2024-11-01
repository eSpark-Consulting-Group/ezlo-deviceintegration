#include "system_utils/process/shared_ram.h"
#include "system_utils/process/shared_ram_exception.h"

#include <fcntl.h>     // O_RDWR, O_CREATE
#include <sys/mman.h>  // shm_open, shm_unlink, mmap, munmap,
                       // PROT_READ, PROT_WRITE, MAP_SHARED, MAP_FAILED
#include <unistd.h>    // ftruncate, close

namespace HubPlatform {
namespace Process {

void releaseSharedRam( const std::string& id ) {
    if( shm_unlink( id.c_str() ) ) {
        throw SharedRamException( "releaseSharedRam failure" );
    }
}

SharedRam::SharedRam( const std::string& id, std::size_t size )
: m_id( id ) {
    // Open existing shared memory object, or create one.
    // Two separate calls are needed here, to mark fact of creation
    // for later initialization of pthread mutex.
    m_descriptor = SharedRamDescriptor( shm_open( m_id.c_str(), O_RDWR, 0660 ) );
    if( !m_descriptor ) {
        m_descriptor = SharedRamDescriptor( shm_open( m_id.c_str(), O_RDWR | O_CREAT, 0660 ) );
        m_isJustInitialized = true;
    }

    if( !m_descriptor ) {
        throw SharedRamException( "Open shared RAM failure: " + m_id );
    }

    // Truncate shared memory segment so it would contain data.
    if( ftruncate( m_descriptor, size ) != 0 ) {
        throw SharedRamException( "Truncate shared RAM failure: " + m_id );
    }

    // Map data into the shared memory.
    m_data = SharedData( m_descriptor, size );
}

const std::string& SharedRam::getId() const {
    return m_id;
}

void* SharedRam::get() const {
    return m_data.get();
}

std::size_t SharedRam::size() {
    return m_data.size();
}

bool SharedRam::isJustInitialized() const {
    return m_isJustInitialized;
}

void SharedRam::release() {
    m_data = SharedData();
    m_descriptor = SharedRamDescriptor();
    if( shm_unlink( m_id.c_str() ) ) {
        throw SharedRamException( "Release shared RAM failure" );
    }
}

}  // namespace Process
}  // namespace HubPlatform
