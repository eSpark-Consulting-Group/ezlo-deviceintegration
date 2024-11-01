#include "hardware_test/utils/test_log.h"

#include <exception>
#include <iostream>
#include <iomanip>
#include <cstdio>

namespace HubPlatform {
namespace HardwareTest {

TestLog::StreamHandler::StreamHandler( TestLog* log )
: m_log( log ) {}

TestLog::StreamHandler::~StreamHandler() {
    try {
        m_log->flushStream( m_stream, m_isRawStream );
    }
    catch( const std::exception& e ) {
        std::cout << "Exception in StreamHandler: " << e.what() << std::endl;
    }
    catch( ... ) {
        std::cout << "Unknown exception in StreamHandler" << std::endl;
    }
}

TestLog::StreamHandler& TestLog::StreamHandler::raw() {
    m_isRawStream = true;
    return *this;
}

void TestLog::StreamHandler::newline() {
    m_log->flushStream( std::stringstream( "\n" ), true );
}

TestLog::TestLog( const std::string& testName )
: m_testName( testName ) {}

TestLog::StreamHandler TestLog::operator()() {
    return { this };
}

void TestLog::flushStream( const std::stringstream& stream, bool raw ) {
    if( stream.str().empty() ) {
        fflush( stdout );
        return;
    }

    std::stringstream ss;
    if( !raw ) {
        ss << "[" << std::left << std::setfill( '.' ) << std::setw( 10 ) << m_testName;
        ss << ".." << std::right << std::setfill( '.' ) << std::setw( 2 ) << ++m_msgNumber << "] ";
    }
    ss << stream.str() << std::endl;
    fprintf( stdout, "%s", ss.str().c_str() );
    fflush( stdout );
}

void newline() {
    fprintf( stdout, "\n" );
    fflush( stdout );
}

}  // namespace HardwareTest
}  // namespace HubPlatform
