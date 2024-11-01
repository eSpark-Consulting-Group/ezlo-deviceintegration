#include "scenes_manager/scenes_engine/states_registry.h"
#include "scenes_manager/scenes_engine/states_registry_resources.h"

#include "database_client/idbclient.h"
#include "database_client/db_tables_names.h"
#include "json_utils/json_utils.h"

namespace SceneEngine {

StatesRegistry::StatesRegistry( IDbClient& dbClient, TimeProvider timeProvider )
: m_dbClient( dbClient )
, m_timeProvider( std::move( timeProvider ) ) {
    if( !m_timeProvider )
        throw StatesRegistryException( "Invalid time provider was set for StatesRegistry" );

    readFromPersistentTable( HaSceneStatesDBName, HaTempSceneStatesDBName );
    readFromPersistentTable( HaSceneBlockStatesDBName, HaTempSceneBlockStatesDBName );
}

void StatesRegistry::storeSceneState( const Scene::SceneID& id, AbstractOperator::OperatorState state ) {
    storeSceneStateImpl( id, state, m_timeProvider() );
}

void StatesRegistry::storeBlockState( const Scene::SceneID& sceneId,
                                      const std::string& blockId,
                                      AbstractOperator::OperatorState state ) {
    storeBlockStateImpl( sceneId, blockId, state, m_timeProvider() );
}

void StatesRegistry::storeInitialSceneState( const Scene::SceneID& id, AbstractOperator::OperatorState state ) {
    bool recordsExists = m_dbClient.has( HaSceneBlockStatesDBName, JSON( StateRegistryDbFields::Id << id ) );
    if( !recordsExists ) {
        storeSceneStateImpl( id, state, TimePoint() );
    }
}

void StatesRegistry::storeInitialBlockState( const Scene::SceneID& sceneId,
                                             const std::string& blockId,
                                             AbstractOperator::OperatorState state ) {
    bool recordsExists = m_dbClient.has( HaTempSceneBlockStatesDBName, JSON( StateRegistryDbFields::Id << blockId ) );
    if( !recordsExists ) {
        storeBlockStateImpl( sceneId, blockId, state, TimePoint() );
    }
}

StatesRegistry::Record StatesRegistry::readSceneState( const Scene::SceneID& id ) const {
    const Json dbRecord =
        m_dbClient.findOne( HaTempSceneStatesDBName, JSON( StateRegistryDbFields::Id << id ), nullptr );
    return parseRecord( dbRecord );
}

StatesRegistry::Record StatesRegistry::readBlockState( const Scene::SceneID& sceneId,
                                                       const std::string& blockId ) const {
    const Json query = JSON( StateRegistryDbFields::Id << blockId );
    const Json dbRecord = m_dbClient.findOne( HaTempSceneBlockStatesDBName, query, nullptr );
    return parseRecord( dbRecord );
}

void StatesRegistry::removeRecordsFromScene( const Scene::SceneID& id ) {
    m_dbClient.remove( HaTempSceneStatesDBName, JSON( StateRegistryDbFields::Id << id ) );
    m_dbClient.remove( HaTempSceneBlockStatesDBName, JSON( StateRegistryDbFields::SceneId << id ) );
    m_deletedSceneIds.insert( id );
}

void StatesRegistry::flushToPersistentMemory() {
    LOGGER_INFO( cc::info( "Flushing states registry to persistent memory" ) );

    if( !m_scenesChangedState.empty() ) {
        updatePersistentTable( HaTempSceneStatesDBName, HaSceneStatesDBName, m_scenesChangedState );
        m_scenesChangedState.clear();
    }
    if( !m_blocksChangedState.empty() ) {
        updatePersistentTable( HaTempSceneBlockStatesDBName, HaSceneBlockStatesDBName, m_blocksChangedState );
        m_blocksChangedState.clear();
    }

    for( const std::string& id : m_deletedSceneIds ) {
        m_dbClient.remove( HaSceneStatesDBName, JSON( StateRegistryDbFields::Id << id ) );
        m_dbClient.remove( HaSceneBlockStatesDBName, JSON( StateRegistryDbFields::SceneId << id ) );
    }
    m_deletedSceneIds.clear();

    LOGGER_INFO( cc::info( "Finished flushing states registry" ) );
}

void StatesRegistry::updatePersistentTable( const std::string& source,
                                            const std::string& destination,
                                            const std::set< std::string >& changedIds ) {
    for( const std::string& id : changedIds ) {
        const auto record = m_dbClient.findOne( source, JSON( StateRegistryDbFields::Id << id ), nullptr );
        m_dbClient.insert( destination, record );
    }
    LOGGER_TRACE( cc::trace( "Updated " ) + cc::note( HaSceneStatesDBName ) );
}

void StatesRegistry::readFromPersistentTable( const std::string& source, const std::string& destination ) {
    LOGGER_DEBUG( cc::debug( "Reading from " ) + cc::note( source ) + cc::debug( " to " ) + cc::note( destination ) );

    m_dbClient.clear( destination );
    LOGGER_TRACE( cc::trace( "Cleared the destination table" ) );

    const auto allRecords = m_dbClient.findAll( source, JSON_EMPTY_OBJ, nullptr );
    for( const auto& record : allRecords )
        m_dbClient.insert( destination, record );
    LOGGER_TRACE( cc::trace( "Inserted all records into a destination table" ) );
}

StatesRegistry::Record StatesRegistry::parseRecord( const Json& dbRecord ) {
    Record result;

    auto stateIt = dbRecord.find( StateRegistryDbFields::State );
    if( stateIt != dbRecord.end() ) {
        const std::string stateStr = json_get_str( *stateIt );
        result.isValid = operatorStateFromString( stateStr, result.state );
        if( !result.isValid )
            LOGGER_ERROR( cc::error( "Cannot convert string to OperatorState, string: " ) + cc::note( stateStr ) );
    }

    if( result.isValid ) {
        auto it = dbRecord.find( StateRegistryDbFields::LastChangeTime );
        if( it != dbRecord.end() && it->is_number_integer() ) {
            const auto timestamp = it->get< int64_t >();
            const auto timeSinceEpoch = TimePoint::clock::duration( timestamp );
            result.lastChangeTime = TimePoint( timeSinceEpoch );
        }
    }

    return result;
}

void StatesRegistry::storeSceneStateImpl( const Scene::SceneID& id,
                                          AbstractOperator::OperatorState state,
                                          TimePoint lastChangeTime ) {
    using namespace StateRegistryDbFields;

    const int64_t timestamp = lastChangeTime.time_since_epoch().count();
    const std::string stateStr = operatorStateToString( state );

    const Json dbRecord = JSON( Id << id << State << stateStr << LastChangeTime << timestamp );
    LOGGER_TRACE( cc::trace( dbRecord.dump() ) );
    m_dbClient.insert( HaTempSceneStatesDBName, dbRecord );

    m_scenesChangedState.insert( id );
}

void StatesRegistry::storeBlockStateImpl( const Scene::SceneID& sceneId,
                                          const std::string& blockId,
                                          AbstractOperator::OperatorState state,
                                          TimePoint lastChangeTime ) {
    using namespace StateRegistryDbFields;

    const int64_t timestamp = lastChangeTime.time_since_epoch().count();
    const std::string stateStr = operatorStateToString( state );

    Json dbRecord = JSON( Id << blockId << SceneId << sceneId << State << stateStr << LastChangeTime << timestamp );
    LOGGER_TRACE( cc::trace( dbRecord.dump() ) );
    m_dbClient.insert( HaTempSceneBlockStatesDBName, dbRecord );

    m_blocksChangedState.insert( blockId );
}

StatesRegistry::Record::Record( TimePoint lastChangeTime, AbstractOperator::OperatorState state, bool isValid )
: lastChangeTime( lastChangeTime )
, state( state )
, isValid( isValid ) {}

}  // namespace SceneEngine
