#pragma once

#include "scenes_manager/scenes_engine/events/event_base.h"

#include <vector>

namespace SceneEngine {

class TransactionalEventsUpdater {
public:
    using Events = std::vector< EventBasePtr >;
    using PostFlushCallback = std::function< void() >;

    explicit TransactionalEventsUpdater( PostFlushCallback postFlushCallback = nullptr );
    explicit TransactionalEventsUpdater( PostFlushCallback postFlushCallback, const std::string& contextId );

    void requestTrigger( EventBasePtr event );
    void requestReset( EventBasePtr event );
    void requestResetSilent( EventBasePtr event );
    void requestTrigger( const Events& events );
    void requestReset( const Events& events );
    void requestResetSilent( const Events& events );
    void flush();

private:
    std::string dumpRequests() const;

    Events m_toTrigger;
    Events m_toReset;
    Events m_toResetSilent;

    PostFlushCallback m_postFlushCallback;

    std::string m_sceneContextId;
};

}  // namespace SceneEngine
