#pragma once

#include "hardware_test/hardware_tests/hardware_test_base.h"
#include "hardware_test/arg_parser.h"
#include "json_utils/json_utils.h"

#include <vector>
#include <string>
#include <chrono>

namespace HubPlatform {
namespace HardwareTest {

class LteTest final : public HardwareTestBase {
public:
    LteTest() = delete;
    explicit LteTest( const volatile sig_atomic_t& exitSignalRef );
    ~LteTest() = default;
    JsonObject getUniqueIMEI();
    JsonObject getUniqueICCID();
    bool hasUniqueICCID();

public:
    TestResult run() override;
    void setUp( const ArgParser& argParser );

private:
    JsonObject m_uniqueIMEI;
    JsonObject m_uniqueICCID;
    bool m_hasUniqueICCID = false;
    bool m_hasCallbox = false;
    std::string m_callboxIPAddress;
    std::vector< std::string > m_bandRSSIEdgeValuesArg;
    std::chrono::seconds m_getRssiDelay;
};

}  // namespace HardwareTest
}  // namespace HubPlatform
