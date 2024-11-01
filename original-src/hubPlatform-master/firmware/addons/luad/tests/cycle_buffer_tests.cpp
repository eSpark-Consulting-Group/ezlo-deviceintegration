#include "luad/modules/network/buffer.h"

#include <algorithm>
#include <string>

#include <gtest/gtest.h>

/*

TESTPLAN

DONE    1. Buffer data providing
DONE        1.1 Check data insertion
DONE        1.2 Check buffer size after insertion
DONE        1.3 Check pausing data provider
DONE    2. Buffer data consuming
DONE        2.1 Check data inspection
DONE        2.2 Check portion data inspection
DONE        2.3 Check data size inspection
DONE        2.4 Check buffer size after inspection
DONE        2.5 Check buffer size after portion inspection
DONE        2.6 Check data size after inspection
DONE        2.7 Check data size after portion inspection
DONE        2.8 Check no resuming data provider while data inspection
DONE        2.9 Check data fetching
DONE        2.10 Check portion data fetching
DONE        2.11 Check buffer size after fetching
DONE        2.12 Check buffer size after portion fetching
DONE        2.13 Check data size after fetching
DONE        2.14 Check data size after portion fetching
DONE        2.15 Check resuming data provider
DONE        2.16 Check data inspection after portion fetching
DONE    3. Buffer data providing/consuming
DONE        3.1 Check pausing/resuming data provider

*/

namespace HubPlatform {
namespace UT {

using namespace LuaNetwork;

class FakeDataSourceManager final : public IDataSourceManager {
public:
    FakeDataSourceManager()
    : m_requestToPauseCount( 0 )
    , m_requestToResumeCount( 0 ) {}

    void pauseDataSource() override {
        m_requestToPauseCount++;
    }

    void resumeDataSource() override {
        m_requestToResumeCount++;
    }

    std::size_t getRequestToPauseCount() const {
        return m_requestToPauseCount;
    }

    std::size_t getRequestToResumeCount() const {
        return m_requestToResumeCount;
    }

    void setRequestToResumeCount( std::size_t requestToResumeCount ) {
        m_requestToResumeCount = requestToResumeCount;
    }

private:
    std::size_t m_requestToPauseCount;
    std::size_t m_requestToResumeCount;
};

class BufferTestSuite : public testing::Test {
protected:
    BufferTestSuite()
    : m_buffer( m_dataSourceManager ) {
        clearBuffer();
    }

    Buffer& getBuffer() {
        return m_buffer;
    }

    void clearBuffer() {
        std::fill( getBuffer().front(), getBuffer().front() + getBuffer().size(), '\0' );
        getBuffer().fetchData();
        m_dataSourceManager.setRequestToResumeCount( 0 );
    }

    const FakeDataSourceManager& getFakeDataSourceManager() const {
        return m_dataSourceManager;
    }

private:
    FakeDataSourceManager m_dataSourceManager;
    Buffer m_buffer;
};

TEST_F( BufferTestSuite, testCheckDataInsertion ) {
    const auto* data = getBuffer().front();

    std::fill( getBuffer().front(), getBuffer().front() + 2, 'S' );
    getBuffer().commit( 2 );

    EXPECT_EQ( std::string( &data[ 0 ], &data[ 4 ] ), std::string( "SS\0\0", 4 ) );
}

TEST_F( BufferTestSuite, testCheckBufferSizeAfterInsertion ) {
    const auto size = getBuffer().size();

    std::fill( getBuffer().front(), getBuffer().front() + 2, 'S' );
    getBuffer().commit( 2 );

    EXPECT_EQ( getBuffer().size(), size - 2 );
}

TEST_F( BufferTestSuite, testCheckPausingDataProvider ) {
    std::fill( getBuffer().front(), getBuffer().front() + 1, 'S' );
    getBuffer().commit( 1 );
    EXPECT_EQ( getFakeDataSourceManager().getRequestToPauseCount(), 0 );

    std::fill( getBuffer().front(), getBuffer().front() + getBuffer().size(), 'S' );
    getBuffer().commit( getBuffer().size() );
    EXPECT_EQ( getFakeDataSourceManager().getRequestToPauseCount(), 1 );

    ASSERT_EQ( getBuffer().size(), 0 );
    getBuffer().commit( getBuffer().size() );
    EXPECT_EQ( getFakeDataSourceManager().getRequestToPauseCount(), 1 );
}

TEST_F( BufferTestSuite, testCheckDataInspection ) {
    std::string data( "wassup" );

    std::copy( data.begin(), data.end(), getBuffer().front() );
    getBuffer().commit( data.size() );

    EXPECT_EQ( getBuffer().inspectData(), data );
}

TEST_F( BufferTestSuite, testCheckPortionDataInspection ) {
    std::string data( "wassup" );

    std::copy( data.begin(), data.end(), getBuffer().front() );
    getBuffer().commit( data.size() );

    EXPECT_EQ( getBuffer().inspectData( 2 ), std::string( data, 0, 2 ) );
}

TEST_F( BufferTestSuite, testCheckDataSizeInspection ) {
    std::string data( "wassup" );

    std::copy( data.begin(), data.end(), getBuffer().front() );
    getBuffer().commit( data.size() );

    EXPECT_EQ( getBuffer().inspectDataSize(), data.size() );
}

TEST_F( BufferTestSuite, testCheckBufferSizeAfterInspection ) {
    std::string data( "wassup" );

    std::copy( data.begin(), data.end(), getBuffer().front() );
    getBuffer().commit( data.size() );

    const auto size = getBuffer().size();
    getBuffer().inspectData();
    EXPECT_EQ( getBuffer().size(), size );
}

TEST_F( BufferTestSuite, testCheckBufferSizeAfterPortionInspection ) {
    std::string data( "wassup" );

    std::copy( data.begin(), data.end(), getBuffer().front() );
    getBuffer().commit( data.size() );

    const auto size = getBuffer().size();
    getBuffer().inspectData( 2 );
    EXPECT_EQ( getBuffer().size(), size );
}

TEST_F( BufferTestSuite, testCheckDataSizeAfterInspection ) {
    std::string data( "wassup" );

    std::copy( data.begin(), data.end(), getBuffer().front() );
    getBuffer().commit( data.size() );

    const auto size = getBuffer().inspectDataSize();
    getBuffer().inspectData();
    EXPECT_EQ( getBuffer().inspectDataSize(), size );
}

TEST_F( BufferTestSuite, testCheckDataSizeAfterPortionInspection ) {
    std::string data( "wassup" );

    std::copy( data.begin(), data.end(), getBuffer().front() );
    getBuffer().commit( data.size() );

    const auto size = getBuffer().inspectDataSize();
    getBuffer().inspectData( 2 );
    EXPECT_EQ( getBuffer().inspectDataSize(), size );
}

TEST_F( BufferTestSuite, testCheckNoResumingDataProviderWhileDataInspection ) {
    std::string data( "wassup" );

    std::copy( data.begin(), data.end(), getBuffer().front() );
    getBuffer().commit( data.size() );

    getBuffer().inspectData();
    EXPECT_EQ( getFakeDataSourceManager().getRequestToResumeCount(), 0 );
}

TEST_F( BufferTestSuite, testCheckDataFetching ) {
    std::string data( "wassup" );

    std::copy( data.begin(), data.end(), getBuffer().front() );
    getBuffer().commit( data.size() );

    EXPECT_EQ( getBuffer().fetchData(), data );
}

TEST_F( BufferTestSuite, testCheckPortionDataFetching ) {
    std::string data( "wassup" );

    std::copy( data.begin(), data.end(), getBuffer().front() );
    getBuffer().commit( data.size() );

    EXPECT_EQ( getBuffer().fetchData( 2 ), std::string( data, 0, 2 ) );
    EXPECT_EQ( getBuffer().inspectData(), std::string( data, 2 ) );
}

TEST_F( BufferTestSuite, testCheckBufferSizeAfterFetching ) {
    std::string data( "wassup" );
    const auto maxSize = getBuffer().size();

    std::copy( data.begin(), data.end(), getBuffer().front() );
    getBuffer().commit( data.size() );

    getBuffer().fetchData();
    EXPECT_EQ( getBuffer().size(), maxSize );
    EXPECT_EQ( getFakeDataSourceManager().getRequestToResumeCount(), 0 );
}

TEST_F( BufferTestSuite, testCheckBufferSizeAfterPortionFetching ) {
    const auto maxSize = getBuffer().size();

    std::fill( getBuffer().front(), getBuffer().front() + getBuffer().size(), 'S' );
    getBuffer().commit( getBuffer().size() );

    getBuffer().fetchData( 1 );
    EXPECT_NE( getBuffer().size(), maxSize );
    EXPECT_EQ( getFakeDataSourceManager().getRequestToResumeCount(), 0 );
}

TEST_F( BufferTestSuite, testCheckDataSizeAfterFetching ) {
    std::string data( "wassup" );

    std::copy( data.begin(), data.end(), getBuffer().front() );
    getBuffer().commit( data.size() );

    getBuffer().fetchData();
    EXPECT_EQ( getBuffer().inspectDataSize(), 0 );
}

TEST_F( BufferTestSuite, testCheckDataSizeAfterPortionFetching ) {
    std::string data( "wassup" );

    std::copy( data.begin(), data.end(), getBuffer().front() );
    getBuffer().commit( data.size() );

    getBuffer().fetchData( 2 );
    EXPECT_EQ( getBuffer().inspectDataSize(), data.size() - 2 );
}

TEST_F( BufferTestSuite, testCheckResumingDataProvider ) {
    std::fill( getBuffer().front(), getBuffer().front() + getBuffer().size(), 'S' );
    getBuffer().commit( getBuffer().size() );

    getBuffer().fetchData();
    EXPECT_EQ( getFakeDataSourceManager().getRequestToResumeCount(), 1 );
}

TEST_F( BufferTestSuite, testCheckDataInspectionAfterPortionFetching ) {
    std::string data( "wassup" );

    std::copy( data.begin(), data.end(), getBuffer().front() );
    getBuffer().commit( data.size() );

    getBuffer().fetchData( 2 );
    EXPECT_EQ( getBuffer().inspectDataSize(), data.size() - 2 );
    EXPECT_EQ( getBuffer().inspectData(), std::string( data, 2 ) );
}

TEST_F( BufferTestSuite, testCheckPausingResumingDataProvider ) {
    std::fill( getBuffer().front(), getBuffer().front() + getBuffer().size(), 'S' );
    getBuffer().commit( getBuffer().size() );
    EXPECT_EQ( getFakeDataSourceManager().getRequestToPauseCount(), 1 );

    getBuffer().fetchData();
    EXPECT_EQ( getFakeDataSourceManager().getRequestToResumeCount(), 1 );
}

}  // namespace UT
}  // namespace HubPlatform
