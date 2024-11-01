#pragma once

#include "hardware_test/hardware_tests/hardware_test_interface.h"
#include "json_utils/json_utils.h"

#include <csignal>
#include <string>

namespace HubPlatform {
namespace HardwareTest {

class HardwareTestBase : public HardwareTestInterface {
protected:
    struct EnumHash {
        template < typename T >
        std::size_t operator()( T t ) const {
            return static_cast< std::size_t >( t );
        }
    };
    static const std::unordered_map< TestResult, std::string, EnumHash > TestResultToStringMap;

public:
    HardwareTestBase() = delete;
    explicit HardwareTestBase( const std::string& name,
                               const std::chrono::seconds& timeout,
                               const volatile sig_atomic_t& exitSignalRef );
    ~HardwareTestBase() = default;
    std::string getName() const;
    std::chrono::seconds getTimeout() const;

protected:
    std::string getTestResultReport( const TestResult testResult = TestResult::UNKNOWN,
                                     const time_t testTimeSeconds = -1,
                                     const std::string& failMessage = "" );
    std::string getTestResultReport( const std::string& testName,
                                     const TestResult testResult = TestResult::UNKNOWN,
                                     const time_t testTimeSeconds = -1,
                                     const std::string& failMessage = "" );
    std::string getTestResultJsonReport( const Json& jResult );
    std::string getTestResultJsonReport( const std::string& name, const Json& jResult );

protected:
    const volatile sig_atomic_t& m_exitSignal;
    const std::string m_name;
    const std::chrono::seconds m_timeout;
};

}  // namespace HardwareTest
}  // namespace HubPlatform
