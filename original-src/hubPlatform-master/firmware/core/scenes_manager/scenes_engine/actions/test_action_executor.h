#pragma once
// Extracted from action.h/cpp
#include "scenes_manager/scenes_engine/actions/iaction_executor.h"

namespace SceneEngine {

class TestActionExecutor : public IActionExecutor {
public:
    using ResultHandler = std::function< void( const Json& response ) >;
    explicit TestActionExecutor( ResultHandler resultHandler );
    bool setResult( const ActionResult& result ) override;
    void setActions( const ActionList& actions ) override;
    void setActions( ActionList&& actions ) override;
    void setSceneId( std::string sceneId ) override;
    const std::string& getSceneId() const override;

    void runNextAction( std::string contextId ) override;

private:
    ActionList m_actions;
    ResultHandler m_resultHandler;
    std::string m_sceneId;
    std::string m_globalRestrictionId;
};

}  // namespace SceneEngine