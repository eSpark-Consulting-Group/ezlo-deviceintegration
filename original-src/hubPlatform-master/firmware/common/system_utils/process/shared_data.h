#pragma once

#include "system_utils/process/shared_ram_descriptor.h"

#include <cstddef>
#include <utility>

namespace HubPlatform {
namespace Process {

/// @brief Maps data into the shared memory. Unmaps data automatically
/// Returns void pointer on data. User should control size of written data manually.
class SharedData final {
public:
    /// @brief empty shared data
    SharedData() noexcept;

    /// @brief maps data into the shared memory. Throws exception if data is not mapped
    /// @param ramDescriptor shared RAM descriptor
    /// @param size of data
    SharedData( const SharedRamDescriptor& ramDescriptor, std::size_t size );
    SharedData( const SharedData& rhs ) = delete;
    SharedData( SharedData&& rhs ) noexcept;

    SharedData& operator=( const SharedData& lhs ) = delete;
    SharedData& operator=( SharedData&& rhs );

    /// @brief in case of unsuccessful unmapping writes an error to std::cerr
    ~SharedData();

    /// @return pointer on mapped data or nullptr
    void* get() const;

    /// @return size of mapped data
    std::size_t size() const;

private:
    void* m_dataPtr;
    std::size_t m_dataSize;
};

}  // namespace Process
}  // namespace HubPlatform
