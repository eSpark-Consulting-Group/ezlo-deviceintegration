#include "luad/interpreter/module_loader/dl_handle.h"

#include <dlfcn.h>

namespace HubPlatform {

DlHandle::DlHandle( const std::string& dlName, int options ) {
    m_hndl = dlopen( dlName.empty() ? nullptr : dlName.c_str(), options );
}

DlHandle::~DlHandle() {
    if( m_hndl )
        dlclose( m_hndl );
}

bool DlHandle::valid() const {
    return m_hndl != nullptr;
}

void* DlHandle::getDlHandle() {
    return m_hndl;
}

const void* DlHandle::getDlHandle() const {
    return m_hndl;
}

}  // namespace HubPlatform
