#include "vera_utils/g150_reader_impl.h"

#include <gtest/gtest.h>

/*

TESTPLAN

DONE    1. Mac address format

*/

using namespace ::testing;

namespace HubPlatform {
namespace VeraProvision {
namespace UT {

class TestReaderImpl : public G150ReaderImpl {
public:
    void setMacAddress( const ProvisionReaderBase::MacAddress& macAddress ) {
        setData( getInfoMacAddress(), reinterpret_cast< const char* >( &macAddress ) );
    }

    void readAll( const std::string& devName = "factory" ) override {
        m_data = ProvisionReaderBase::Data( std::move( m_initData ) );
    };

private:
    void setData( const ValueInfo& info, const char* data ) {
        size_t end = info.offset + info.length;
        if( end >= m_initData.size() ) {
            m_initData.resize( end + 1 );
        }
        m_initData.replace( info.offset, info.length, data, info.length );
    }

    std::string m_initData;
};

class VeraProvisionTestSuite : public Test {};

TEST_F( VeraProvisionTestSuite, testMacAddressFormat /*1*/ ) {
    auto macAddress = ProvisionReaderBase::MacAddress( { 0xE0, 0x60, 0x66, 0x0F, 0x00, 0xAC } );
    TestReaderImpl reader;
    reader.setMacAddress( macAddress );
    reader.readAll();
    EXPECT_EQ( reader.getMacAddressString(), "E0:60:66:0F:00:AC" );
}

}  // namespace UT
}  // namespace VeraProvision
}  // namespace HubPlatform