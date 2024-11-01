#include "system_utils/process/shared_ram_descriptor.h"

#include <unistd.h>

namespace HubPlatform {
namespace Process {

SharedRamDescriptor::SharedRamDescriptor()
: m_descriptor{ -1 } {}

SharedRamDescriptor::SharedRamDescriptor( int descriptor )
: m_descriptor{ descriptor } {}

SharedRamDescriptor::~SharedRamDescriptor() {
    if( m_descriptor >= 0 ) {
        close( m_descriptor );
    }
}

SharedRamDescriptor::SharedRamDescriptor( SharedRamDescriptor&& rhs ) {
    m_descriptor = rhs.m_descriptor;
    rhs.m_descriptor = -1;
}

SharedRamDescriptor& SharedRamDescriptor::operator=( SharedRamDescriptor&& rhs ) {
    m_descriptor = rhs.m_descriptor;
    rhs.m_descriptor = -1;
    return *this;
}

int SharedRamDescriptor::get() const {
    return m_descriptor;
}

SharedRamDescriptor::operator bool() const {
    return m_descriptor >= 0;
}

SharedRamDescriptor::operator int() const {
    return m_descriptor;
}

}  // namespace Process
}  // namespace HubPlatform
