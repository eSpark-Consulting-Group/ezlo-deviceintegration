#include "scenes_manager/tests/fakes/fake_controller_factory.h"
#include "scenes_manager/tests/fakes/test_scenes_service.h"

#include "scenes_manager/scenes_engine/actions/action_exception.h"
#include "scenes_manager/scenes_engine/states_registry.h"
#include "scenes_manager/scenes_engine/item_cache/item_cache.h"
#include "scenes_manager/scenes_engine/expressions/expression_manager.h"
#include "scenes_manager/scenes_engine/helpers/clock.h"
#include "scenes_manager/modify_scene/resources.h"

#include "cored/remote_interface/cored_interface_factory.h"

#include <gtest/gtest.h>

namespace SceneEngine {
namespace UT {

/*

TEST PLAN

DONE    1. Referencing blocks
DONE        1.1 Condition satisfied
DONE        1.2 Condition dissatisied
DONE        1.3 Reference to non-existing block
DONE    2. Exception references
DONE        2.1 Condition satisfied
DONE        2.2 Condition dissatisied
DONE        2.3 Reference to non-existing expression
DONE        2.4 Reference to expression of non-boolean type

*/

class ActionExceptionsTest : public ::testing::Test {
public:
    using BlockReference = ActionExceptionData::BlockReference;
    using ExpressionReference = ActionExceptionData::ExpressionReference;

    void SetUp() override;

    void addVariableExpression( const std::string& name, std::string valueType, Json value );

public:
    std::unique_ptr< StatesRegistry > m_statesRegistry;
    std::unique_ptr< Expressions::ExpressionManager > m_expressionsManager;

private:
    HubPlatform::CoreDRemoteInterface::CoreDInterfaceUPtr m_coreInterface;
    std::unique_ptr< HubPlatform::ScenesManager::TestService > m_service;  // any other service might be used instead

    std::unique_ptr< FakeControllerFactory > m_controllerFactory;
    std::unique_ptr< ItemCacheBase > m_itemCache;
    std::unique_ptr< Clock > m_clock;
};

void ActionExceptionsTest::SetUp() {
    m_service = std::make_unique< HubPlatform::ScenesManager::TestService >();
    m_coreInterface = HubPlatform::CoreDRemoteInterface::makeInstance( *m_service );

    auto makeController = [this]( const FakeControllerFactory::ControllerId& ) {
        return std::make_shared< HubPlatform::ScenesManager::InternalController >( *m_service );
    };
    m_controllerFactory = std::make_unique< FakeControllerFactory >( makeController );

    m_itemCache = std::make_unique< ItemCache >( *m_controllerFactory, m_service->getDBClient() );
    m_itemCache->setUp();

    m_clock = std::make_unique< Clock >();
    m_statesRegistry = std::make_unique< StatesRegistry >( m_service->getDBClient(),
                                                           [this]() { return m_clock->greenwichMeanTimePoint(); } );

    m_expressionsManager = std::make_unique< Expressions::ExpressionManager >(
        *m_coreInterface, m_service->getDBClient(), *m_itemCache, "0000000" );
    m_expressionsManager->setUp();
}

void ActionExceptionsTest::addVariableExpression( const std::string& name, std::string valueType, Json value ) {
    auto expression = std::make_unique< Expressions::TypedVariableExpression >(
        *m_expressionsManager, name, value, Json(), std::move( valueType ), Json(), true, Json() );
    expression->value( std::move( value ) );
    m_expressionsManager->setVariableExpression( name, std::move( expression ) );
}

TEST_F( ActionExceptionsTest, blockRef_conditionSatifsied ) {  // 1.1
    const std::string sceneId = "scene";
    const std::string blockId = "block";
    m_statesRegistry->storeBlockState( sceneId, blockId, AbstractOperator::True );

    ActionException exception( BlockReference{ sceneId, blockId }, *m_statesRegistry, *m_expressionsManager );
    EXPECT_EQ( exception.evaluate(), ActionException::State::Satisfied );
}

TEST_F( ActionExceptionsTest, blockRef_conditionDissatifsied ) {  // 1.2
    const std::string sceneId = "scene";
    const std::string blockId = "block";
    m_statesRegistry->storeBlockState( sceneId, blockId, AbstractOperator::False );

    ActionException exception( BlockReference{ sceneId, blockId }, *m_statesRegistry, *m_expressionsManager );
    EXPECT_EQ( exception.evaluate(), ActionException::State::Dissatisfied );
}

TEST_F( ActionExceptionsTest, blockRef_referenceToDeletedBlock ) {  // 1.3
    ActionException exception( BlockReference{ "scene", "block" }, *m_statesRegistry, *m_expressionsManager );
    EXPECT_THROW( exception.evaluate(), ActionExceptionError );
}

TEST_F( ActionExceptionsTest, expressionRef_conditionSatifsied ) {  // 2.1
    const std::string name = "expr";
    addVariableExpression( name, Types::BoolType, true );

    ActionException exception( ExpressionReference{ name }, *m_statesRegistry, *m_expressionsManager );
    EXPECT_EQ( exception.evaluate(), ActionException::State::Satisfied );
}

TEST_F( ActionExceptionsTest, expressionRef_conditionDissatifsied ) {  // 2.2
    const std::string name = "expr";
    addVariableExpression( name, Types::BoolType, false );

    ActionException exception( ExpressionReference{ name }, *m_statesRegistry, *m_expressionsManager );
    EXPECT_EQ( exception.evaluate(), ActionException::State::Dissatisfied );
}

TEST_F( ActionExceptionsTest, expressionRef_invalidReference ) {  // 2.3
    ActionException exception( ExpressionReference{ "expr" }, *m_statesRegistry, *m_expressionsManager );
    EXPECT_THROW( exception.evaluate(), ActionExceptionError );
}

TEST_F( ActionExceptionsTest, expressionRef_referenceToExpressionWithWrongType ) {  // 2.4
    const std::string name = "expr";
    addVariableExpression( name, Types::IntType, 42 );

    ActionException exception( ExpressionReference{ name }, *m_statesRegistry, *m_expressionsManager );
    EXPECT_THROW( exception.evaluate(), ActionExceptionError );
}

}  // namespace UT
}  // namespace SceneEngine
