#pragma once

#include "hardware_test/hardware_tests/hardware_test_base.h"
#include "hardware_test/hardware_tests/network_interface.h"
#include "hardware_test/arg_parser.h"

#include <unordered_set>

namespace HubPlatform {
namespace HardwareTest {

class ConnectionTest final : public HardwareTestBase {
public:
    ConnectionTest() = delete;
    explicit ConnectionTest( const volatile sig_atomic_t& exitSignalRef );
    ~ConnectionTest() = default;

public:
    TestResult run() override;
    void setUp( const ArgParser& argParser );

private:
    TestResult testPing( Json& jTestResults,
                         std::unordered_set< NetworkInterface::NetworkInterfaceUPtr >& interfaceAddresses );
    TestResult testWiFi( Json& jTestResults, std::vector< std::string >& supportedInterfaces );
    JsonArray scanWiFiNetworks( const std::string& ifaceName );

private:
    const volatile sig_atomic_t& m_exitSignal;
    std::string m_pingAddr = "8.8.8.8";
};

}  // namespace HardwareTest
}  // namespace HubPlatform
