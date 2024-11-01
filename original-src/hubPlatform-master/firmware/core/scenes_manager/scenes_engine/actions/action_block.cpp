#include "scenes_manager/scenes_engine/actions/action_block.h"
#include "scenes_manager/scenes_engine/exceptions.h"

namespace SceneEngine {

void ActionBlocks::forEach( const std::function< void( ActionSharedPtr ) >& visitor ) const {
    for( auto actionsPtr : { &m_then, &m_else } ) {
        std::for_each( actionsPtr->begin(), actionsPtr->end(), visitor );
    }
    for( auto groupsPtr : { &m_thenGroups, &m_elseGroups } ) {
        for( auto& groupIter : *groupsPtr ) {
            std::for_each( groupIter.second.begin(), groupIter.second.end(), visitor );
        }
    }
}

bool ActionBlocks::empty() const {
    return m_then.empty() && m_else.empty() && m_thenGroups.empty() && m_elseGroups.empty();
}

void ActionBlocks::clear() {
    m_then.clear();
    m_else.clear();
    m_thenGroups.clear();
    m_elseGroups.clear();
}

}  // namespace SceneEngine
