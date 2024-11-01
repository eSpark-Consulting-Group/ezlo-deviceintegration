#include "hardware_test/hardware_tests/usb_test.h"

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>
#include <iterator>
#include <cstdio>
#include <exception>
#include <sys/stat.h>
#include <sys/mount.h>

#include "base_utils/timeutils.h"
#include "json_utils/json_utils.h"

#include "hardware_test/utils/test_log.h"

namespace HubPlatform {
namespace HardwareTest {

#define TEST_NAME "USB"
constexpr std::chrono::seconds TEST_TIMEOUT{ 10 };

namespace {
static constexpr std::chrono::milliseconds testLoopDelayMs( 40 );
TestLog testLog( TEST_NAME );
}  // namespace

UsbTest::UsbTest( const volatile sig_atomic_t& exitSignalRef )
: HardwareTestBase( TEST_NAME, TEST_TIMEOUT, exitSignalRef ) {}

HardwareTestInterface::TestResult UsbTest::run() {
    const std::vector< const char* > partitions = { "/dev/sdb1", "/dev/sda1" };
    const std::string mnt_path = "/mnt";
    const std::string file_path = mnt_path + "/usb_test_file";
    const std::string expected_str = "usb_test_string";

    TestResult result = HardwareTestInterface::TestResult::UNKNOWN;
    bool device_mounted = false;
    std::stringstream err_msg;

    testLog() << "Starting " << m_name << " test...";
    auto startTestTime = TimeUtils::getMonotonicTime();

    try {
        struct stat st;
        auto exist = [&st]( const char* partition ) -> bool { return stat( partition, &st ) != -1; };
        auto partition_it = std::find_if( partitions.cbegin(), partitions.cend(), exist );
        if( partition_it == partitions.cend() ) {
            err_msg << "USB device/partition not found! Probed partitions: ";
            std::copy( partitions.cbegin(), partitions.cend(), std::ostream_iterator< std::string >( err_msg, " " ) );
            err_msg << std::endl;
            throw std::runtime_error( err_msg.str() );
        }
        testLog() << "Partition found: " << *partition_it;

        if( mount( *partition_it, mnt_path.c_str(), "vfat", MS_RDONLY, nullptr ) == -1 ) {
            if( errno == EBUSY ) {
                err_msg << "Mount failed: device already mounted or busy: " << *partition_it;
                throw std::runtime_error( err_msg.str() );
            }
            else {
                err_msg << "Failed to mount: " << *partition_it << " to " << mnt_path;
                throw std::runtime_error( err_msg.str() );
            }
        }
        device_mounted = true;
        testLog() << "Partition mounted: " << *partition_it << " -> " << mnt_path;

        std::ifstream usb_file( file_path );
        if( !usb_file.is_open() ) {
            err_msg << "Failed to open USB test file: " << file_path;
            throw std::runtime_error( err_msg.str() );
        }
        testLog() << "USB test file found: " << file_path;

        std::string str;
        std::getline( usb_file, str );
        usb_file.close();
        if( str != expected_str ) {
            err_msg << "String mismatch: expected: \"" << expected_str << "\" actual: \"" << str << "\"";
            throw std::runtime_error( err_msg.str() );
        }
        testLog() << "File content is valid";

        result = TestResult::DONE;
    }
    catch( std::runtime_error& e ) {
        result = TestResult::FAILED;
        testLog() << e.what();
    }

    if( device_mounted ) {
        umount( mnt_path.c_str() );
        testLog() << "Partition unmounted";
    }

    auto testTime = TimeUtils::getDiffMonotonicTime( startTestTime );
    testLog() << getTestResultReport( result, testTime );

    Json jTestResult;
    jTestResult[ "name" ] = m_name;
    jTestResult[ "result" ] = result == TestResult::DONE ? true : false;
    jTestResult[ "threshold" ] = 1;
    jTestResult[ "value" ] = result == TestResult::DONE ? 1 : 0;
    testLog() << getTestResultJsonReport( jTestResult );

    std::string resultStr = ( result == TestResult::DONE ) ? "SUCCESS" : "FAIL";
    testLog() << "test finished with " << resultStr;

    return result;
}

}  // namespace HardwareTest
}  // namespace HubPlatform
