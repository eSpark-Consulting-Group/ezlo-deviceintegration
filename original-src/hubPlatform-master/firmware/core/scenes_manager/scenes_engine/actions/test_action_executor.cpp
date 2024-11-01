// Extracted from action.h/cpp
#include "scenes_manager/scenes_engine/actions/test_action_executor.h"
#include "scenes_manager/scenes_engine/actions/action.h"
#include <utility>

namespace SceneEngine {

bool TestActionExecutor::setResult( const IActionExecutor::ActionResult& result ) {
    m_resultHandler( result.message );
    return true;
}

void TestActionExecutor::setActions( const IActionExecutor::ActionList& actions ) {
    m_actions = actions;
}

void TestActionExecutor::setActions( IActionExecutor::ActionList&& actions ) {
    m_actions = std::move( actions );
}

void TestActionExecutor::runNextAction( std::string contextId ) {
    while( !m_actions.empty() ) {
        auto action = m_actions.front();
        auto actionExecutor = shared_from_this();
        action->requestRun( actionExecutor );
        m_actions.pop_front();
    }
}

TestActionExecutor::TestActionExecutor( TestActionExecutor::ResultHandler resultHandler )
: m_resultHandler( std::move( resultHandler ) ) {}

void TestActionExecutor::setSceneId( std::string sceneId ) {
    m_sceneId = std::move( sceneId );
}

const std::string& TestActionExecutor::getSceneId() const {
    return m_sceneId;
}

}  // namespace SceneEngine