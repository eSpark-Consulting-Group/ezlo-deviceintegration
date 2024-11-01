#pragma once
#include <cstdint>
#include <exception>
#include <string>

namespace HubPlatform {
namespace system_utils {

class FreeLocalPortException : public std::exception {
public:
    explicit FreeLocalPortException( const std::string& error ) noexcept
    : m_error( error ) {}

    virtual const char* what() const _GLIBCXX_USE_NOEXCEPT override {
        return m_error.c_str();
    }

private:
    const std::string m_error;
};

bool isLocalPortFree( std::uint16_t port );
std::uint16_t getUniqueFreeLocalPort();

}  // namespace system_utils
}  // namespace HubPlatform
