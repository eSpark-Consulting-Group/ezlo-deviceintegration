#pragma once

// Extracted from action.h
#include "scenes_manager/scenes_engine/actions/iaction_executor.h"
#include <vector>
#include <map>
#include <functional>
#include <string>

namespace SceneEngine {
enum class RunSceneActionBlockType { Then, Else, ThenGroups, ElseGroups, Empty };
struct ActionBlocks {
    using Actions = std::vector< ActionSharedPtr >;
    // key - ID of a corresponding "when" block, value - vector of action pointers
    using ActionGroups = std::map< std::string, Actions >;

    Actions m_then;
    Actions m_else;

    ActionGroups m_thenGroups;  // A map of action groups linked to the TRUE condition of a given "when" block ID
    ActionGroups m_elseGroups;  // A map of action groups linked to the FALSE condition of a given "when" block ID

    void forEach( const std::function< void( ActionSharedPtr ) >& ) const;

public:
    bool empty() const;
    void clear();
};

}  // namespace SceneEngine