#pragma once
#include "scenes_manager/scenes_engine/actions/action_block.h"
#include "scenes_manager/scenes_engine/scene.h"

namespace SceneEngine {

class ISceneActionExecute {
public:
    virtual ~ISceneActionExecute() = default;

    virtual bool isEnabled() const = 0;

    /**
     * @brief
     * Run "then" block actions, usually invoked when full scene condition becomes TRUE
     * @param contextId - Scene context identifier, value is common for all calls of the same scene event handling and
     * actions execution.
     * @return true if run was successfully initiated, false in all other cases
     */
    virtual bool runThenActions( const std::string& contextId ) const = 0;

    /**
     * @brief
     * Run actions linked to the TRUE condition of a given "when" block (i.e. "thenGroups" actions)
     * Usually invoked when a corresponding scene operator is triggered and its state becomes TRUE
     * @param contextId - Scene context identifier, value is common for all calls of the same scene event handling and
     * actions execution.
     * @return true if run was successfully initiated, false in all other cases
     */
    virtual bool runThenActions( const std::string& whenBlockID, const std::string& contextId ) const = 0;

    /**
     * @brief
     * Run "else" block actions, usually invoked when full scene condition becomes FALSE
     * @param contextId - Scene context identifier, value is common for all calls of the same scene event handling and
     * actions execution.
     * @return true if run was successfully initiated, false in all other cases
     */
    virtual bool runElseActions( const std::string& contextId ) const = 0;

    /**
     * @brief
     * Run actions linked to the FALSE condition of a given "when" block (i.e. "elseGroups" actions)
     * Usually invoked when a corresponding scene operator is triggered and its state becomes TRUE
     * @param contextId - Scene context identifier, value is common for all calls of the same scene event handling and
     * actions execution.
     * @return true if run was successfully initiated, false in all other cases
     */
    virtual bool runElseActions( const std::string& whenBlockID, const std::string& contextId ) const = 0;

    virtual SceneSharedPtr getScene() const = 0;
    virtual Scene::SceneID getSceneId() const = 0;

    virtual ActionBlocks::Actions getActions( const std::string& name ) const = 0;
    virtual ActionBlocks::Actions getThenActions( const std::string& name ) const = 0;

    virtual bool hasThenActions() const = 0;
    virtual bool hasElseActions() const = 0;

    virtual void stopScene( const std::string& sceneId, const RunSceneActionBlockType blockType ) = 0;
};

using SceneActionExecutePtr = std::shared_ptr< ISceneActionExecute >;

}  // namespace SceneEngine