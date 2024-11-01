#include "scenes_manager/tests/fixtures/states_registry_fixture.h"

#include "common_utils/cpp14.h"

namespace SceneEngine {
namespace UT {

void StatesRegistryFixture::SetUp() {
    recreateStatesRegistry();
    m_testSceneId = "sceneId";
    m_statesRegistry->storeInitialSceneState( m_testSceneId, AbstractOperator::OperatorState::False );
}

void StatesRegistryFixture::setClockTime( std::chrono::system_clock::duration time ) {
    m_clockTime = time;
}

void StatesRegistryFixture::storeSceneState( AbstractOperator::OperatorState state ) const {
    m_statesRegistry->storeSceneState( m_testSceneId, state );
}

void StatesRegistryFixture::storeBlockState( const std::string& blockId, AbstractOperator::OperatorState state ) const {
    m_statesRegistry->storeBlockState( m_testSceneId, blockId, state );
}

StatesRegistry::Record StatesRegistryFixture::readSceneState() const {
    return m_statesRegistry->readSceneState( m_testSceneId );
}

StatesRegistry::Record StatesRegistryFixture::readBlockState( const std::string& blockId ) const {
    return m_statesRegistry->readBlockState( m_testSceneId, blockId );
}

IDbClient& StatesRegistryFixture::dbClient() {
    return m_fakeDbClient;
}

void StatesRegistryFixture::recreateStatesRegistry() {
    auto timeProvider = [this]() { return std::chrono::system_clock::time_point( m_clockTime ); };
    m_statesRegistry = std::make_unique< StatesRegistry >( dbClient(), std::move( timeProvider ) );
}

}  // namespace UT
}  // namespace SceneEngine
