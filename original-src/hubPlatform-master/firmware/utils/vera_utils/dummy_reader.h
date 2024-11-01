#pragma once

#include <string>
#include <array>
#include <stdexcept>

namespace HubPlatform {
namespace VeraProvision {

class DummyReader {
public:
    void readAll( const std::string& devName = "factory" ){};

    using MacAddress = std::array< uint8_t, 6 >;
    MacAddress getMacAddress() const {
        return { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    };
    std::string getMacAddressString() const {
        return {};
    };
    std::string getHwKey() const {
        return {};
    };
    std::string getSerial() const {
        return {};
    };
    std::string getPassPhrase() const {
        return {};
    };
    std::string getCountry() const {
        return {};
    };
    std::string getLanguage() const {
        return {};
    };
    std::string getWifiSsid() const {
        return {};
    };
};

}  // namespace VeraProvision
}  // namespace HubPlatform
