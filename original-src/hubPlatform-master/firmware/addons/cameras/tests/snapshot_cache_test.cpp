#include "cameras/snapshot/snapshot_cache.h"
#include "json_utils/json_utils.h"

#include <memory>
#include <chrono>
#include <utility>

#include <gtest/gtest.h>

#include "cored/tests/service/test_core_service.h"
#include "common_utils/cpp14.h"

using namespace ::testing;
namespace HubPlatform {
namespace UT {
const static std::string FilePath = "filePath";
const static int CacheTimeoutSec = 2;
const static auto StreamId1 = "StreamId1";
const static auto Metadata1 = JSON( "metadata"
                                    << "Metadata1" );
const static auto SnapshotId1_1 = "SnapshotId1_1";
const static auto SnapshotId1_3 = "SnapshotId1_3";

const static auto StreamId2 = "StreamId2";
const static auto Metadata2 = JSON( "metadata"
                                    << "Metadata2" );
const static auto SnapshotId2_1 = "SnapshotId2_1";
const static auto SnapshotId2_2 = "SnapshotId2_2";
const static auto SnapshotId2_3 = "SnapshotId2_3";

const static auto StreamId3 = "StreamId3";
const static auto Metadata3 = JSON( "metadata"
                                    << "Metadata3" );
const static auto SnapshotId3_1 = "SnapshotId3_1";
const static auto SnapshotId3_2 = "SnapshotId3_2";

const std::string SnapshotFilePath1 = "/path/to/snapshots/snapshot1.jpg";
const std::string SnapshotFilePath2 = " /path/to/snapshots/snapshot2.jpg";
const std::string SnapshotFilePath3 = " /path/to/snapshots/snapshot3.jpg";

class CameraSnapshotCacheTestSuite : public Test {
public:
    std::unique_ptr< IDbClient > m_dbClientPtr;
    void SetUp() override {
        m_dbClientPtr = std::make_unique< HubPlatform::UT::FakeDbClient >();
    }

    IDbClient& getDBClient() {
        return *m_dbClientPtr;
    }

    Json makeRequest( const std::string& streamId, const std::string& snapshotId, const Json& metadata ) {
        return JSON( "streamId" << streamId << "snapshotId" << snapshotId << "metadata" << metadata );
    }

    void requestProcessing( Cameras::SnapshotCache& snapshotCache,
                            std::vector< Json > testRequestParams,
                            int codeLine ) {
        std::string codeLineStr = "code line: " + std::to_string( codeLine );
        int index = 0;
        snapshotCache.requestsProcessing( [&testRequestParams, &codeLineStr, &index]( const Json& params ) {
            auto testRequestParam = testRequestParams[ index++ ].dump();
            EXPECT_TRUE( params.dump() == testRequestParam ) << codeLineStr;
        } );
    }

    bool canRemoveSnapshotByStreamId( Cameras::SnapshotCache& snapshotCache,
                                      const std::string& toRemoveSnapshotStreamId ) {
        std::string removedSnapshotStreamId;

        std::set< std::string > removedSnapshotStreamIds;
        snapshotCache.tryToClearNotProcessedImages( [&removedSnapshotStreamIds]( const std::string& streamId ) {
            removedSnapshotStreamIds.insert( streamId );
        } );

        return removedSnapshotStreamIds.count( toRemoveSnapshotStreamId ) == 1;
    }
};

TEST_F( CameraSnapshotCacheTestSuite, addToCacheAndCheckOfExpired ) {
    Cameras::SnapshotCache snapshotCache( getDBClient(), CacheTimeoutSec );
    Json imageInfo1 = JSON( FilePath << SnapshotFilePath1 );
    Json imageInfo2 = JSON( FilePath << SnapshotFilePath2 );

    snapshotCache.addSnapshotToCache( StreamId1, imageInfo1, {} );
    std::this_thread::sleep_for( std::chrono::milliseconds( 1000 ) );
    snapshotCache.addSnapshotToCache( StreamId2, imageInfo2, {} );

    EXPECT_TRUE( snapshotCache.isCachedImage( StreamId1 ) );
    EXPECT_TRUE( snapshotCache.isCachedImage( StreamId2 ) );
    EXPECT_EQ( snapshotCache.getCachedImagePath( StreamId1 ), SnapshotFilePath1 );
    EXPECT_EQ( snapshotCache.getCachedImagePath( StreamId2 ), SnapshotFilePath2 );
    EXPECT_FALSE( snapshotCache.isExpiredCachedImage( StreamId1 ) );
    EXPECT_FALSE( snapshotCache.isExpiredCachedImage( StreamId2 ) );
    std::this_thread::sleep_for( std::chrono::milliseconds( 1800 ) );
    EXPECT_FALSE( snapshotCache.isExpiredCachedImage( StreamId2 ) );
    std::this_thread::sleep_for( std::chrono::milliseconds( 1500 ) );
    EXPECT_TRUE( snapshotCache.isExpiredCachedImage( StreamId2 ) );
}

TEST_F( CameraSnapshotCacheTestSuite, processRequestQueue ) {
    Cameras::SnapshotCache snapshotCache( getDBClient(), CacheTimeoutSec );

    auto id11 = snapshotCache.appendRequest( makeRequest( StreamId1, SnapshotId1_1, Metadata1 ) );
    auto id21 = snapshotCache.appendRequest( makeRequest( StreamId2, SnapshotId2_1, Metadata2 ) );
    auto id31 = snapshotCache.appendRequest( makeRequest( StreamId3, SnapshotId3_1, Metadata3 ) );
    auto id22 = snapshotCache.appendRequest( makeRequest( StreamId2, SnapshotId2_2, Metadata2 ) );
    auto id12 = snapshotCache.appendRequest( makeRequest( StreamId1, SnapshotId1_1, Metadata1 ) );

    std::vector< Json > testRequestCachedQueueParams = {
        JSON( "cacheId" << id11 << "metadata" << Metadata1 << "snapshotId" << SnapshotId1_1 << "streamId"
                        << StreamId1 ),
        JSON( "cacheId" << id21 << "metadata" << Metadata2 << "snapshotId" << SnapshotId2_1 << "streamId"
                        << StreamId2 ),
        JSON( "cacheId" << id31 << "metadata" << Metadata3 << "snapshotId" << SnapshotId3_1 << "streamId"
                        << StreamId3 ),
    };

    requestProcessing( snapshotCache, testRequestCachedQueueParams, __LINE__ );

    EXPECT_FALSE( canRemoveSnapshotByStreamId( snapshotCache, StreamId1 ) );
    EXPECT_FALSE( canRemoveSnapshotByStreamId( snapshotCache, StreamId2 ) );
    EXPECT_FALSE( canRemoveSnapshotByStreamId( snapshotCache, StreamId3 ) );

    snapshotCache.setRequestStatusDone( id11 );
    Json imageInfo1 = JSON( FilePath << SnapshotFilePath1 );
    snapshotCache.addSnapshotToCache( StreamId1, imageInfo1, {} );

    testRequestCachedQueueParams = { JSON( "cacheId" << id12 << "metadata" << Metadata1 << "snapshotId" << SnapshotId1_1
                                                     << "streamId" << StreamId1 ),
                                     JSON( "cacheId" << id22 << "metadata" << Metadata2 << "snapshotId" << SnapshotId2_2
                                                     << "streamId" << StreamId2 ) };

    requestProcessing( snapshotCache, testRequestCachedQueueParams, __LINE__ );

    EXPECT_TRUE( canRemoveSnapshotByStreamId( snapshotCache, StreamId1 ) );
    EXPECT_FALSE( canRemoveSnapshotByStreamId( snapshotCache, StreamId2 ) );
    EXPECT_FALSE( canRemoveSnapshotByStreamId( snapshotCache, StreamId3 ) );

    auto id13 = snapshotCache.appendRequest( makeRequest( StreamId1, SnapshotId1_3, Metadata1 ) );
    auto id23 = snapshotCache.appendRequest( makeRequest( StreamId2, SnapshotId2_3, Metadata2 ) );

    snapshotCache.setRequestStatusDone( id21 );
    auto imageInfo2 = JSON( FilePath << SnapshotFilePath2 );
    snapshotCache.addSnapshotToCache( StreamId2, imageInfo2, {} );

    snapshotCache.setRequestStatusDone( id31 );
    auto imageInfo3 = JSON( FilePath << SnapshotFilePath3 );
    snapshotCache.addSnapshotToCache( StreamId3, imageInfo3, {} );

    auto id32 = snapshotCache.appendRequest( makeRequest( StreamId2, SnapshotId3_2, Metadata3 ) );

    testRequestCachedQueueParams = { JSON( "cacheId" << id22 << "metadata" << Metadata2 << "snapshotId" << SnapshotId2_2
                                                     << "streamId" << StreamId2 ),
                                     JSON( "cacheId" << id32 << "metadata" << Metadata3 << "snapshotId" << SnapshotId3_2
                                                     << "streamId" << StreamId3 ) };

    requestProcessing( snapshotCache, testRequestCachedQueueParams, __LINE__ );

    snapshotCache.setRequestStatusDone( id12 );
    testRequestCachedQueueParams = { JSON( "cacheId" << id13 << "metadata" << Metadata1 << "snapshotId" << SnapshotId1_3
                                                     << "streamId" << StreamId1 ) };

    requestProcessing( snapshotCache, testRequestCachedQueueParams, __LINE__ );
    snapshotCache.setRequestStatusDone( id13 );

    testRequestCachedQueueParams = { JSON( "cacheId" << id13 << "metadata" << Metadata1 << "snapshotId" << SnapshotId1_3
                                                     << "streamId" << StreamId1 ) };
    requestProcessing( snapshotCache, testRequestCachedQueueParams, __LINE__ );
    snapshotCache.setRequestStatusDone( id22 );
    testRequestCachedQueueParams = { JSON( "cacheId" << id23 << "metadata" << Metadata2 << "snapshotId" << SnapshotId2_3
                                                     << "streamId" << StreamId2 ) };
    requestProcessing( snapshotCache, testRequestCachedQueueParams, __LINE__ );

    snapshotCache.setRequestStatusDone( id23 );
    snapshotCache.setRequestStatusDone( id31 );
    snapshotCache.setRequestStatusDone( id32 );

    requestProcessing( snapshotCache, {}, __LINE__ );

    EXPECT_TRUE( canRemoveSnapshotByStreamId( snapshotCache, StreamId1 ) );
    EXPECT_TRUE( canRemoveSnapshotByStreamId( snapshotCache, StreamId2 ) );
    EXPECT_TRUE( canRemoveSnapshotByStreamId( snapshotCache, StreamId3 ) );
}

}  // namespace UT
}  // namespace HubPlatform
