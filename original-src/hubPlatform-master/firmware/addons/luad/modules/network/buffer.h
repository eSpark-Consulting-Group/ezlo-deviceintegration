#pragma once

#include <string>
#include <array>
#include <memory>
#include <limits>

namespace HubPlatform {
namespace LuaNetwork {

class IBuffer {
public:
    static constexpr int MaxCapacity = 8192; /* Default size in lua sockets library */

    virtual ~IBuffer();

    virtual char* front() = 0;
    virtual std::size_t size() const = 0;
    virtual void commit( std::size_t providedDataAmount ) = 0;

    virtual std::string fetchData( std::size_t amount ) = 0;
    virtual std::string inspectData( std::size_t amount ) const = 0;
    virtual std::size_t inspectDataSize() const = 0;
};

class IDataSourceManager {
public:
    virtual ~IDataSourceManager();

    virtual void pauseDataSource() = 0;
    virtual void resumeDataSource() = 0;
};

/* @brief implements a cycle-buffer suitable for recv/read syscalls */
class Buffer final : public IBuffer {
public:
    Buffer( IDataSourceManager& dataSourceManager );

    char* front() override;
    std::size_t size() const override;
    void commit( std::size_t providedDataAmount ) override;

    std::string fetchData( std::size_t amount = IBuffer::MaxCapacity ) override;
    std::string inspectData( std::size_t amount = IBuffer::MaxCapacity ) const override;
    std::size_t inspectDataSize() const override;

private:
    void reset();

private:
    IDataSourceManager& m_dataSourceManager;
    std::array< char, MaxCapacity > m_buffer;
    std::size_t m_readIndex;
    std::size_t m_writeIndex;
    std::size_t m_spaceToRead;
    std::size_t m_spaceToWrite;
    bool m_dataSourcePaused;
};

std::unique_ptr< IBuffer > makeCycleBuffer( IDataSourceManager& dataSourceManager );

}  // namespace LuaNetwork
}  // namespace HubPlatform
