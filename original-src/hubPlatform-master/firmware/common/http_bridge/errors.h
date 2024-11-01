#pragma once
#include <stdexcept>

namespace HubPlatform {
namespace HttpBridge {

class ApiBridgeError : public std::runtime_error {
public:
    ApiBridgeError( const std::string& s, int status )
    : std::runtime_error( s )
    , m_status( status ) {}
    int status() const {
        return m_status;
    }
    void status( int value ) {
        m_status = value;
    }

private:
    int m_status{ 0 };
};

}  // namespace HttpBridge
}  // namespace HubPlatform