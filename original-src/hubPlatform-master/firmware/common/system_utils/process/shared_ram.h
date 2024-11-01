#pragma once

#include "system_utils/process/shared_ram_descriptor.h"
#include "system_utils/process/shared_data.h"

#include <string>

namespace HubPlatform {
namespace Process {

/// @brief Creates a new shared RAM with given id.
/// If a shared RAM with such id exists in the system, it will be used.
/// Otherwise a new shared RAM will be created.
/// In case of creating error, the exception will be thrown.
/// The destructor unmaps data and closes shared RAM decriptor but shared RAM lives even after process overs
/// THIS SHARED RAM IS NOT ATOMIC AND IS NOT SYNCRONIZED BETWEEN PROCESSES
///
/// Useful links:
/// https://opensource.com/article/19/4/interprocess-communication-linux-storage
/// https://gist.github.com/yamnikov-oleg/abf61cf96b4867cbf72d
/// https://gist.github.com/walchko/6d211a2dc8b2f5158642fb2fd65cf295
class SharedRam {
public:
    /// @brief Creates a new shared RAM with given id.
    /// @param id of shared RAM.
    /// @param size of data
    SharedRam( const std::string& id, std::size_t size );

    /// @return id of shared RAM
    const std::string& getId() const;

    /// @return pointer to shared data
    void* get() const;

    /// @return size of data
    std::size_t size();

    bool isJustInitialized() const;

    void release();

private:
    std::string m_id;
    SharedRamDescriptor m_descriptor;
    SharedData m_data;
    bool m_isJustInitialized = false;
};

/// @brief convenient wrapper of SharedRam class for direct reference to DataType
/// DON'T use std containers or other object uses heap.
/// Use only simple types and data structes.
template < typename DataType >
class SharedRamWrapper final {
public:
    SharedRamWrapper( const std::string& id )
    : m_data( id, sizeof( DataType ) ) {}

    SharedRamWrapper( SharedRamWrapper&& rhs ) noexcept
    : m_data( std::move( rhs ) ) {}

    SharedRamWrapper& operator=( SharedRamWrapper&& rhs ) {
        return m_data.operator=( std::move( rhs ) );
    }

    DataType& get() const {
        return *static_cast< DataType* >( m_data.get() );
    }

private:
    SharedRam m_data;
};

}  // namespace Process
}  // namespace HubPlatform
