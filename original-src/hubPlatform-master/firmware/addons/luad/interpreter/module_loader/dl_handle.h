#pragma once

#include <string>

namespace HubPlatform {

/* @brief represents a dynamic library handle */
class DlHandle final {
public:
    DlHandle( const std::string& dlName, int options = 0 );
    ~DlHandle();

    DlHandle( const DlHandle& rhs ) = delete;
    DlHandle& operator=( const DlHandle& rhs ) = delete;

    bool valid() const;
    void* getDlHandle();
    const void* getDlHandle() const;

private:
    void* m_hndl;
};

}  // namespace HubPlatform
