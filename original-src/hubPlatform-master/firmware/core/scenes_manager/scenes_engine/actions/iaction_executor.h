#pragma once
// Extracted from action.h
#include "json_utils/json_fwd.h"
#include <memory>
#include <deque>
#include <vector>

namespace SceneEngine {

enum class ExecutionPolicy { CheckResult, IgnoreResult, WaitResult, Unknown };

class Action;
using ActionSharedPtr = std::shared_ptr< Action >;

using BroadcastCallback = std::function< void( const JsonObject& message ) >;

class IActionExecutor : public std::enable_shared_from_this< IActionExecutor > {
public:
    using ActionList = std::deque< ActionSharedPtr >;
    using ActionId = Json;

    struct ActionResult {
        ActionId id;
        ExecutionPolicy executionPolicy;
        bool result;
        Json message;
        std::string contextId;
    };
    using ResultList = std::vector< ActionResult >;

    virtual ~IActionExecutor() = default;
    virtual bool setResult( const ActionResult& result ) = 0;
    virtual void setActions( const ActionList& actions ) = 0;
    virtual void setActions( ActionList&& actions ) = 0;
    virtual void setSceneId( std::string sceneId ) = 0;
    virtual const std::string& getSceneId() const = 0;
    virtual void runNextAction( std::string contextId ) = 0;
};

using ActionExecutorPtr = std::shared_ptr< IActionExecutor >;

}  // namespace SceneEngine
