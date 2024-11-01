#include <exception>
#include <string>

#pragma once

namespace HubPlatform {
namespace Process {

class SharedRamException : public std::exception {
public:
    explicit SharedRamException( const std::string& error ) noexcept
    : m_error( error ) {}

    virtual const char* what() const _GLIBCXX_USE_NOEXCEPT override {
        return m_error.c_str();
    }

private:
    const std::string m_error;
};

}  // namespace Process
}  // namespace HubPlatform
