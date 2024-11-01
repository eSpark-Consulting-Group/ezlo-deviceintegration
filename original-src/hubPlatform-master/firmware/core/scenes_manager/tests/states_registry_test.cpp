#include "scenes_manager/tests/fixtures/states_registry_fixture.h"
#include "scenes_manager/scenes_engine/states_registry_resources.h"

#include "common_utils/cpp14.h"

namespace SceneEngine {
namespace UT {

/*

TEST PLAN

DONE    1. Store and read block state
DONE    2. Update of stored block state
DONE    3. Store and read scene state
DONE    4. Update of stored scene state
DONE    5. Removal of scene records with one scene
DONE    6. Removal of all scene records with few scenes
DONE    7. Corrupted operator state in DB
DONE    8. Flushing to persistent table
DONE    9. Flush and then read the data from persistent table
DONE    10. Updating existing records in persistent table
DONE    11. Removing records from persistent table
DONE    12. Read state of unknown block
DONE    13. Read state of unknown state
DONE    14. Initial stored state doesn't record timestamp

*/

bool operator==( const StatesRegistry::Record& left, const StatesRegistry::Record& right ) {
    auto toTuple = []( const StatesRegistry::Record& record ) {
        return std::tie( record.lastChangeTime, record.state, record.isValid );
    };
    return toTuple( left ) == toTuple( right );
}

class StatesRegistryTest : public StatesRegistryFixture {
public:
    using OperatorState = AbstractOperator::OperatorState;

    void compareTempAndPersistentTables();

private:
    void compareTempAndPersistentTables( const std::string& tempTableName, const std::string& persistentTableName );
};

void StatesRegistryTest::compareTempAndPersistentTables() {
    compareTempAndPersistentTables( HaTempSceneStatesDBName, HaSceneStatesDBName );
    compareTempAndPersistentTables( HaTempSceneBlockStatesDBName, HaSceneBlockStatesDBName );
}

void StatesRegistryTest::compareTempAndPersistentTables( const std::string& tempTableName,
                                                         const std::string& persistentTableName ) {
    auto tempTableContent = dbClient().findAll( tempTableName, JSON_EMPTY_OBJ, nullptr );
    auto persistentTableContent = dbClient().findAll( persistentTableName, JSON_EMPTY_OBJ, nullptr );

    auto sortPredicate = []( const Json& left, const Json& right ) {
        return left[ StateRegistryDbFields::Id ] < right[ StateRegistryDbFields::Id ];
    };
    std::sort( tempTableContent.begin(), tempTableContent.end(), sortPredicate );
    std::sort( persistentTableContent.begin(), persistentTableContent.end(), sortPredicate );
    EXPECT_EQ( tempTableContent, persistentTableContent );
}

TEST_F( StatesRegistryTest, blockState ) {  // 1.
    const std::string blockId = "blockId";
    const auto state = OperatorState::True;
    const auto changeTime = StatesRegistry::TimePoint() + std::chrono::hours( 33 );

    setClockTime( changeTime.time_since_epoch() );
    storeBlockState( blockId, state );

    const auto record = readBlockState( blockId );
    EXPECT_TRUE( record == StatesRegistry::Record( changeTime, state, true ) );
}

TEST_F( StatesRegistryTest, blockStateUpdate ) {  // 2.
    const std::string blockId = "blockId";

    const auto originalState = OperatorState::Disabled;
    const auto originalChangeTime = StatesRegistry::TimePoint() + std::chrono::hours( 33 );
    setClockTime( originalChangeTime.time_since_epoch() );
    storeBlockState( blockId, originalState );

    const auto newState = OperatorState::True;
    const auto newChangeTime = StatesRegistry::TimePoint() + std::chrono::hours( 44 );
    setClockTime( newChangeTime.time_since_epoch() );
    storeBlockState( blockId, newState );

    const StatesRegistry::Record record = readBlockState( blockId );
    EXPECT_TRUE( record == StatesRegistry::Record( newChangeTime, newState, true ) );
}

TEST_F( StatesRegistryTest, sceneState ) {  // 3.
    const auto state = OperatorState::True;
    const auto changeTime = StatesRegistry::TimePoint() + std::chrono::hours( 33 );

    setClockTime( changeTime.time_since_epoch() );
    storeSceneState( state );

    const StatesRegistry::Record record = readSceneState();
    EXPECT_TRUE( record == StatesRegistry::Record( changeTime, state, true ) );
}

TEST_F( StatesRegistryTest, sceneStateUpdate ) {  // 4.
    const auto originalState = OperatorState::Disabled;
    const auto originalChangeTime = StatesRegistry::TimePoint() + std::chrono::hours( 33 );
    setClockTime( originalChangeTime.time_since_epoch() );
    storeSceneState( originalState );

    const auto newState = OperatorState::True;
    const auto newChangeTime = StatesRegistry::TimePoint() + std::chrono::hours( 44 );
    setClockTime( newChangeTime.time_since_epoch() );
    storeSceneState( newState );

    const StatesRegistry::Record record = readSceneState();
    EXPECT_TRUE( record == StatesRegistry::Record( newChangeTime, newState, true ) );
}

TEST_F( StatesRegistryTest, removingRecordsSingleScene ) {  // 5.
    const std::string blockId11 = "blockId1";
    const std::string blockId12 = "blockId2";

    const auto changeTime = StatesRegistry::TimePoint() + std::chrono::hours( 33 );
    setClockTime( changeTime.time_since_epoch() );
    storeSceneState( OperatorState::True );
    storeBlockState( blockId11, OperatorState::True );
    storeBlockState( blockId12, OperatorState::True );

    m_statesRegistry->removeRecordsFromScene( m_testSceneId );
    EXPECT_FALSE( StatesRegistry::Record().isValid );
    EXPECT_TRUE( readSceneState() == StatesRegistry::Record() );
    EXPECT_TRUE( readBlockState( blockId11 ) == StatesRegistry::Record() );
    EXPECT_TRUE( readBlockState( blockId12 ) == StatesRegistry::Record() );
}

TEST_F( StatesRegistryTest, removingRecordsWithTwoScenes ) {  // 6.
    const std::string sceneId1 = "sceneId1";
    const std::string sceneId2 = "sceneId2";

    const std::string scene2State = "scene2State";

    const std::string blockId11 = "blockId11";
    const std::string blockId12 = "blockId12";
    const std::string blockId21 = "blockId21";
    const std::string blockId22 = "blockId22";

    const auto scene1BlocksState = OperatorState::True;
    const auto scene2BlocksState = OperatorState::Disabled;

    const auto changeTime1 = StatesRegistry::TimePoint() + std::chrono::hours( 33 );
    setClockTime( changeTime1.time_since_epoch() );
    m_statesRegistry->storeSceneState( sceneId1, scene1BlocksState );
    m_statesRegistry->storeBlockState( sceneId1, blockId11, scene1BlocksState );
    m_statesRegistry->storeBlockState( sceneId1, blockId12, scene1BlocksState );

    const auto changeTime2 = StatesRegistry::TimePoint() + std::chrono::hours( 44 );
    setClockTime( changeTime2.time_since_epoch() );
    m_statesRegistry->storeSceneState( sceneId2, scene2BlocksState );
    m_statesRegistry->storeBlockState( sceneId2, blockId21, scene2BlocksState );
    m_statesRegistry->storeBlockState( sceneId2, blockId22, scene2BlocksState );

    m_statesRegistry->removeRecordsFromScene( sceneId1 );
    EXPECT_TRUE( m_statesRegistry->readSceneState( sceneId1 ) == StatesRegistry::Record() );
    EXPECT_TRUE( m_statesRegistry->readBlockState( sceneId1, blockId11 ) == StatesRegistry::Record() );
    EXPECT_TRUE( m_statesRegistry->readBlockState( sceneId1, blockId12 ) == StatesRegistry::Record() );

    const auto scene2Record = m_statesRegistry->readSceneState( sceneId2 );
    const auto block21Record = m_statesRegistry->readBlockState( sceneId2, blockId21 );
    const auto block22Record = m_statesRegistry->readBlockState( sceneId2, blockId22 );

    const StatesRegistry::Record expected{ changeTime2, scene2BlocksState, true };
    EXPECT_TRUE( m_statesRegistry->readSceneState( sceneId2 ) == expected );
    EXPECT_TRUE( m_statesRegistry->readBlockState( sceneId2, blockId21 ) == expected );
    EXPECT_TRUE( m_statesRegistry->readBlockState( sceneId2, blockId22 ) == expected );
}

TEST_F( StatesRegistryTest, corruptedOperatorStateInDb ) {  // 7.
    using namespace StateRegistryDbFields;

    const std::chrono::minutes time( 10 );
    const std::string blockId = "blockId";
    setClockTime( time );
    storeBlockState( blockId, OperatorState::True );

    int64_t timestamp = StatesRegistry::TimePoint( time ).time_since_epoch().count();
    const Json dbRecord =
        JSON( Id << blockId << SceneId << m_testSceneId << State << "corrupted" << LastChangeTime << timestamp );
    dbClient().insert( HaTempSceneBlockStatesDBName, dbRecord );

    const auto blockState = readBlockState( blockId );
    const StatesRegistry::Record expected{ StatesRegistry::TimePoint(), OperatorState::False, false };
    EXPECT_TRUE( blockState == expected );
}

TEST_F( StatesRegistryTest, flusingToPersistentTable ) {  // 8.
    setClockTime( std::chrono::minutes( 10 ) );

    storeSceneState( OperatorState::True );
    storeBlockState( "blockId1", OperatorState::True );
    storeBlockState( "blockId2", OperatorState::False );
    storeBlockState( "blockId3", OperatorState::Disabled );
    m_statesRegistry->flushToPersistentMemory();

    compareTempAndPersistentTables();
}

TEST_F( StatesRegistryTest, readingFromPersistentTable ) {  // 9.
    storeSceneState( OperatorState::True );
    storeBlockState( "blockId1", OperatorState::True );
    storeBlockState( "blockId2", OperatorState::False );
    storeBlockState( "blockId3", OperatorState::Disabled );
    m_statesRegistry->flushToPersistentMemory();

    dbClient().clear( HaTempSceneStatesDBName );
    dbClient().clear( HaTempSceneBlockStatesDBName );

    recreateStatesRegistry();
    compareTempAndPersistentTables();
}

TEST_F( StatesRegistryTest, updatingExistingRecordsInPersistentTable ) {  // 10.
    storeSceneState( OperatorState::True );
    storeBlockState( "blockId1", OperatorState::True );
    storeBlockState( "blockId2", OperatorState::False );
    storeBlockState( "blockId3", OperatorState::Disabled );
    m_statesRegistry->flushToPersistentMemory();

    storeSceneState( OperatorState::False );
    storeBlockState( "blockId1", OperatorState::True );
    storeBlockState( "blockId2", OperatorState::True );
    storeBlockState( "blockId3", OperatorState::True );
    m_statesRegistry->flushToPersistentMemory();

    compareTempAndPersistentTables();
}

TEST_F( StatesRegistryTest, removingRecordsFromPersistentTable ) {  // 11.
    storeSceneState( OperatorState::True );
    storeBlockState( "blockId1", OperatorState::True );
    storeBlockState( "blockId2", OperatorState::False );
    storeBlockState( "blockId3", OperatorState::Disabled );
    m_statesRegistry->flushToPersistentMemory();

    m_statesRegistry->removeRecordsFromScene( m_testSceneId );
    m_statesRegistry->flushToPersistentMemory();

    compareTempAndPersistentTables();
}

TEST_F( StatesRegistryTest, stateOfUnknownBlock ) {  // 12.
    const auto record = m_statesRegistry->readBlockState( "some_scene_id", "some_block_id" );
    EXPECT_FALSE( record.isValid );
}

TEST_F( StatesRegistryTest, stateOfUnknownScene ) {  // 13.
    const auto record = m_statesRegistry->readSceneState( "some_scene_id" );
    EXPECT_FALSE( record.isValid );
}

TEST_F( StatesRegistryTest, initialStateNotIncludesTimestamp ) {  // 14.
    const auto initialState = AbstractOperator::OperatorState::False;
    const std::string testBlockId = "some_block_id";
    m_statesRegistry->storeInitialSceneState( m_testSceneId, initialState );
    m_statesRegistry->storeInitialBlockState( m_testSceneId, testBlockId, initialState );

    const StatesRegistry::Record expected( StatesRegistry::TimePoint(), initialState, true );

    const auto sceneRecord = m_statesRegistry->readSceneState( m_testSceneId );
    const auto blockRecord = m_statesRegistry->readBlockState( m_testSceneId, testBlockId );
    EXPECT_TRUE( sceneRecord == expected );
    EXPECT_TRUE( blockRecord == expected );
}

}  // namespace UT
}  // namespace SceneEngine
