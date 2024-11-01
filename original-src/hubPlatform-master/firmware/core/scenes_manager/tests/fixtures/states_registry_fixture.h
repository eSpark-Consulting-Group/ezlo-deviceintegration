#include "scenes_manager/scenes_engine/states_registry.h"
#include "database_client/fake_db/fake_db_client.h"

#include <gtest/gtest.h>

namespace SceneEngine {
namespace UT {

class StatesRegistryFixture : public ::testing::Test {
public:
    void SetUp() override;

    void setClockTime( std::chrono::system_clock::duration time );

    void storeSceneState( AbstractOperator::OperatorState state ) const;
    void storeBlockState( const std::string& blockId, AbstractOperator::OperatorState state ) const;

    StatesRegistry::Record readSceneState() const;
    StatesRegistry::Record readBlockState( const std::string& blockId ) const;

    IDbClient& dbClient();

    void recreateStatesRegistry();

public:
    std::string m_testSceneId;
    std::unique_ptr< StatesRegistry > m_statesRegistry;

private:
    std::chrono::system_clock::duration m_clockTime;
    HubPlatform::UT::FakeDbClient m_fakeDbClient;
};

}  // namespace UT
}  // namespace SceneEngine
