#include "scenes_manager/scenes_engine/actions/dispatcher.h"

#include <algorithm>

namespace SceneEngine {

Dispatcher& Dispatcher::instance() {
    static Dispatcher dispatcher;

    return dispatcher;
}

void Dispatcher::addHandler( Handler&& handler ) {
    m_handlers.push_back( std::move( handler ) );
}

void Dispatcher::clear() {
    m_handlers.clear();
}

void Dispatcher::fireAction( const std::string& name, const Action& action, ActionExecutorPtr actionExecutorPtr ) {
    std::for_each( m_handlers.begin(), m_handlers.end(), [&]( const Handler& handler ) {
        handler( name, action, actionExecutorPtr );
    } );
}

}  // namespace SceneEngine
