#pragma once

#include "scenes_manager/scenes_engine/scene.h"
#include "scenes_manager/scenes_engine/operators/scene_operator.h"

#include <string>
#include <chrono>

class IDbClient;

namespace SceneEngine {

class StatesRegistryException : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class StatesRegistry {
public:
    using TimePoint = std::chrono::system_clock::time_point;
    using TimeProvider = std::function< TimePoint() >;

    struct Record {
        Record() = default;
        Record( TimePoint lastChangeTime, AbstractOperator::OperatorState state, bool isValid );

        TimePoint lastChangeTime;
        AbstractOperator::OperatorState state = AbstractOperator::OperatorState::False;
        bool isValid = false;
    };

public:
    StatesRegistry( IDbClient& dbClient, TimeProvider timeProvider );

    void storeSceneState( const Scene::SceneID& id, AbstractOperator::OperatorState state );
    void storeBlockState( const Scene::SceneID& sceneId,
                          const std::string& blockId,
                          AbstractOperator::OperatorState state );
    void storeInitialSceneState( const Scene::SceneID& id, AbstractOperator::OperatorState state );
    void storeInitialBlockState( const Scene::SceneID& sceneId,
                                 const std::string& blockId,
                                 AbstractOperator::OperatorState state );

    Record readSceneState( const Scene::SceneID& id ) const;
    Record readBlockState( const Scene::SceneID& sceneId, const std::string& blockId ) const;

    void removeRecordsFromScene( const Scene::SceneID& id );

    void flushToPersistentMemory();

private:
    void updatePersistentTable( const std::string& source,
                                const std::string& destination,
                                const std::set< std::string >& changedIds );
    void readFromPersistentTable( const std::string& source, const std::string& destination );

    void storeSceneStateImpl( const Scene::SceneID& id,
                              AbstractOperator::OperatorState state,
                              TimePoint lastChangeTime );
    void storeBlockStateImpl( const Scene::SceneID& sceneId,
                              const std::string& blockId,
                              AbstractOperator::OperatorState state,
                              TimePoint lastChangeTime );

    static Record parseRecord( const Json& dbRecord );

private:
    IDbClient& m_dbClient;
    TimeProvider m_timeProvider;

    std::set< std::string > m_blocksChangedState;
    std::set< std::string > m_scenesChangedState;
    std::set< std::string > m_deletedSceneIds;
};

}  // namespace SceneEngine
