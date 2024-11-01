#pragma once

#include "scenes_manager/scenes_engine/actions/action.h"

namespace HubPlatform {
namespace ScenesManager {

class ActionInterface {
public:
    using ActionExecutorPtr = SceneEngine::ActionExecutorPtr;
    using ActionArguments = SceneEngine::Action::Arguments;
    using Action = SceneEngine::Action;
    class InvalidActionArgs : public std::invalid_argument {
    public:
        using std::invalid_argument::invalid_argument;
    };

    class SendFinishCallback {
    public:
        SendFinishCallback( const Action& action, ActionExecutorPtr finishCallback )
        : m_action( action )
        , m_actionExecutorPtr( finishCallback ) {}

        ~SendFinishCallback() {
            std::string contextId = m_action.getContextId();
            if( m_actionExecutorPtr && m_actionExecutorPtr->setResult( { m_action.getId(),
                                                                         m_action.getExecutionPolicy(),
                                                                         m_result,
                                                                         m_message,
                                                                         m_action.getContextId() } ) ) {
                if( m_action.getExecutionPolicy() == SceneEngine::ExecutionPolicy::WaitResult ) {
                    m_actionExecutorPtr->runNextAction( contextId );
                }
                else if( m_result && m_action.getExecutionPolicy() == SceneEngine::ExecutionPolicy::CheckResult ) {
                    m_actionExecutorPtr->runNextAction( contextId );
                }
            }
        }

        void setResult( bool isSuccessful ) {
            m_result = isSuccessful;
        }

        void setResult( const Json& message ) {
            m_message = message;
        }

    private:
        bool m_result = false;
        Json m_message;
        Action m_action;
        ActionExecutorPtr m_actionExecutorPtr;
    };

    virtual ~ActionInterface() = default;

    virtual void run( const Action& action, ActionExecutorPtr actionExecutorPtr ) = 0;
};

}  // namespace ScenesManager
}  // namespace HubPlatform
