#pragma once

namespace HubPlatform {
namespace Process {

class SharedRamDescriptor final {
public:
    SharedRamDescriptor();
    explicit SharedRamDescriptor( int descriptor );
    ~SharedRamDescriptor();

    SharedRamDescriptor( const SharedRamDescriptor& lhs ) = delete;
    SharedRamDescriptor( SharedRamDescriptor&& rhs );
    SharedRamDescriptor& operator=( const SharedRamDescriptor& lhs ) = delete;
    SharedRamDescriptor& operator=( SharedRamDescriptor&& rhs );

    int get() const;

    operator int() const;
    operator bool() const;

private:
    int m_descriptor;
};

}  // namespace Process
}  // namespace HubPlatform
