#pragma once

#include <string>
#include <sstream>
#include <chrono>
#include <atomic>

namespace HubPlatform {
namespace HardwareTest {

class TestLog {
public:
    class StreamHandler {
        std::stringstream m_stream;
        TestLog* m_log = nullptr;
        bool m_isRawStream = false;

    public:
        StreamHandler( TestLog* log );
        ~StreamHandler();

        template < typename T >
        StreamHandler& operator<<( T const& value );
        StreamHandler& raw();
        void newline();
    };

public:
    TestLog( const std::string& testName );
    StreamHandler operator()();

protected:
    void flushStream( const std::stringstream& stream, bool raw );

private:
    std::string m_testName;
    std::atomic_size_t m_msgNumber{ 0 };
};

template < typename T >
TestLog::StreamHandler& TestLog::StreamHandler::operator<<( const T& value ) {
    m_stream << value;
    return *this;
}

}  // namespace HardwareTest
}  // namespace HubPlatform
