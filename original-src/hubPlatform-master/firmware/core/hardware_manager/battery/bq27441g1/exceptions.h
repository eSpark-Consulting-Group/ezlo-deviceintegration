#pragma once

#include <memory>

namespace HubPlatform {
namespace HardwareManager {
namespace Bq27441g1 {

class CommunicationError : public std::runtime_error {
public:
    explicit CommunicationError( const std::string& message )
    : runtime_error{ message } {}
};

}  // namespace Bq27441g1
}  // namespace HardwareManager
}  // namespace HubPlatform
