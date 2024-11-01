
#pragma once

#include <string>
#include <stdexcept>

namespace HubPlatform {
namespace Tests {

class TemporaryFileError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class TemporaryFileWrapper final {
public:
    TemporaryFileWrapper();

    ~TemporaryFileWrapper();

    const std::string& getFilePath() const noexcept;

    TemporaryFileWrapper( const TemporaryFileWrapper& ) = delete;
    TemporaryFileWrapper& operator=( const TemporaryFileWrapper& ) = delete;

    TemporaryFileWrapper( TemporaryFileWrapper&& ) noexcept = default;

    // create custom implementation if needed
    TemporaryFileWrapper& operator=( TemporaryFileWrapper&& ) noexcept = delete;

private:
    std::string m_path;
};

}  // namespace Tests
}  // namespace HubPlatform
