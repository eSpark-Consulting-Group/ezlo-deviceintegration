
#pragma once

#include <string>
#include <stdexcept>

#include <atomic>

namespace HubPlatform {
namespace Tests {

class TemporaryFolderError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class TemporaryFolderWrapper {
public:
    TemporaryFolderWrapper();

    virtual ~TemporaryFolderWrapper();

    const std::string& getPath() const noexcept;

    TemporaryFolderWrapper( const TemporaryFolderWrapper& ) = delete;
    TemporaryFolderWrapper& operator=( const TemporaryFolderWrapper& ) = delete;

    TemporaryFolderWrapper( TemporaryFolderWrapper&& ) noexcept;

    TemporaryFolderWrapper& operator=( TemporaryFolderWrapper&& ) noexcept;

    void keepContent();

private:
    std::string m_path;
    std::atomic_bool m_removeContent;
};

}  // namespace Tests
}  // namespace HubPlatform
