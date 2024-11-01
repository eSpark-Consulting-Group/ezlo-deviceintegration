#pragma once

#include <string>
#include <chrono>

namespace HubPlatform {
namespace Network {

class GenericModem {
public:
    GenericModem() = default;
    virtual ~GenericModem() = default;

    virtual bool isEmbedded() const = 0;

    virtual std::string executeCustomATCommand(
        const std::string& ATCommand,
        const std::chrono::milliseconds& timeout = std::chrono::milliseconds( 300 ) ) = 0;
};

}  // namespace Network
}  // namespace HubPlatform
