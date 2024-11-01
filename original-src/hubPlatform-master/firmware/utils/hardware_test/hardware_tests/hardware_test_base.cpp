#include "hardware_test/hardware_tests/hardware_test_base.h"
#include "base_utils/ezlo_common.h"

#include <iostream>
#include <cstring>

namespace HubPlatform {
namespace HardwareTest {

const std::unordered_map< HardwareTestInterface::TestResult, std::string, HardwareTestBase::EnumHash >
    HardwareTestBase::TestResultToStringMap = {
        { HardwareTestInterface::TestResult::FAILED, "FAILED" },
        { HardwareTestInterface::TestResult::UNKNOWN, "UNKNOWN" },
        { HardwareTestInterface::TestResult::DONE, "DONE" },
    };

HardwareTestBase::HardwareTestBase( const std::string& name,
                                    const std::chrono::seconds& timeout,
                                    const volatile sig_atomic_t& exitSignalRef )
: m_name( name )
, m_timeout( timeout )
, m_exitSignal( exitSignalRef ) {}

std::string HardwareTestBase::getTestResultReport( const std::string& testName,
                                                   const TestResult testResult,
                                                   const time_t testTimeSeconds,
                                                   const std::string& failMessage ) {
    static constexpr size_t prefixMaxWidth = 70;
    static constexpr size_t resultPosition = 72;
    static constexpr size_t bufferMaxWidth = 80;
    static constexpr size_t resultWidth = bufferMaxWidth - resultPosition;

    char reportBuffer[ bufferMaxWidth ];
    memset( reportBuffer, '.', resultPosition );
    memset( reportBuffer + resultPosition, 0, resultWidth );
    int position = 0;
    if( testTimeSeconds > 0 ) {
        position =
            snprintf( reportBuffer, prefixMaxWidth, "%s test finished in %ld (s)", testName.c_str(), testTimeSeconds );
    }
    else {
        position = snprintf( reportBuffer, prefixMaxWidth, "%s test finished", testName.c_str() );
    }
    memset( reportBuffer + position, '.', resultPosition - position );
    auto resultPair = TestResultToStringMap.find( testResult );
    snprintf( reportBuffer + resultPosition, resultWidth, "%s", resultPair->second.c_str() );
    return reportBuffer;
}

std::string HardwareTestBase::getTestResultReport( const TestResult testResult,
                                                   const time_t testTimeSeconds,
                                                   const std::string& failMessage ) {
    return getTestResultReport( m_name, testResult, testTimeSeconds, failMessage );
}

std::string HardwareTestBase::getTestResultJsonReport( const Json& jResult ) {
    std::string jstr( jResult.dump() );
    std::vector< unsigned char > vec( jstr.size() );
    vec.assign( jstr.begin(), jstr.end() );
    jstr = ezlo::common::APIs::base64Encode( vec );

    std::stringstream ss;
    ss << JSON( "name" << m_name << "data" << jstr );
    return ss.str();
}

std::string HardwareTestBase::getTestResultJsonReport( const std::string& name, const Json& jResult ) {
    std::string jstr( jResult.dump() );
    std::vector< unsigned char > vec( jstr.size() );
    vec.assign( jstr.begin(), jstr.end() );
    jstr = ezlo::common::APIs::base64Encode( vec );

    std::stringstream ss;
    ss << JSON( "name" << name << "data" << jstr );
    return ss.str();
}

std::string HardwareTestBase::getName() const {
    return m_name;
}

std::chrono::seconds HardwareTestBase::getTimeout() const {
    return m_timeout;
}

}  // namespace HardwareTest
}  // namespace HubPlatform
