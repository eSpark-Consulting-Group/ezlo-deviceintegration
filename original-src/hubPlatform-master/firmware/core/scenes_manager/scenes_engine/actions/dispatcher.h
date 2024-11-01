#pragma once

#include "scenes_manager/scenes_engine/actions/iaction_executor.h"  // uses ActionExecutorPtr

#include <vector>
#include <functional>

namespace SceneEngine {

class Action;

class Dispatcher {
public:
    static Dispatcher& instance();

    using Handler = std::function< void( const std::string& name, const Action&, ActionExecutorPtr ) >;

    void addHandler( Handler&& handler );
    void clear();
    void fireAction( const std::string& name, const Action& action, ActionExecutorPtr actionExecutorPtr );

private:
    std::vector< Handler > m_handlers;
};

}  // namespace SceneEngine
