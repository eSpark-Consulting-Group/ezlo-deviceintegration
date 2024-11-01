#include "scenes_manager/scenes_engine/events/transactional_events_updater.h"

#include "log_utils/ezlo_log.h"
#include "tiny/tiny_consolecolors.h"

namespace SceneEngine {

TransactionalEventsUpdater::TransactionalEventsUpdater( PostFlushCallback postFlushCallback )
: m_postFlushCallback( std::move( postFlushCallback ) ) {}

TransactionalEventsUpdater::TransactionalEventsUpdater( PostFlushCallback postFlushCallback,
                                                        const std::string& sceneContextId )
: m_postFlushCallback( std::move( postFlushCallback ) )
, m_sceneContextId( sceneContextId ) {}

void TransactionalEventsUpdater::requestTrigger( EventBasePtr event ) {
    m_toTrigger.push_back( std::move( event ) );
}

void TransactionalEventsUpdater::requestReset( EventBasePtr event ) {
    m_toReset.push_back( std::move( event ) );
}

void TransactionalEventsUpdater::requestResetSilent( EventBasePtr event ) {
    m_toResetSilent.push_back( std::move( event ) );
}

void TransactionalEventsUpdater::requestTrigger( const Events& events ) {
    for( auto& event : events )
        requestTrigger( event );
}

void TransactionalEventsUpdater::requestReset( const Events& events ) {
    for( auto& event : events )
        requestReset( event );
}

void TransactionalEventsUpdater::requestResetSilent( const Events& events ) {
    for( auto& event : events )
        requestResetSilent( event );
}

void TransactionalEventsUpdater::flush() {
    if( Log::getLogLevel() >= Log::DEBUG ) {
        std::string dump = dumpRequests();
        if( !dump.empty() )
            LOGGER_DEBUG( cc::debug( dump ) );
    }

    auto setState = []( Events& events, bool state ) {
        for( auto& event : events )
            event->setOperatorState( state );
    };
    auto notifyChanges = []( Events& events ) {
        for( auto& event : events )
            event->notifyOperatorState();
    };
    auto evaluateSubtree = []( Events& events ) {
        for( auto& event : events )
            event->evaluateSubtree();
    };

    // disregards "enabled" status of the operators, as this should be handled in implementation of operators
    setState( m_toTrigger, true );
    setState( m_toReset, false );
    setState( m_toResetSilent, false );

    notifyChanges( m_toTrigger );
    notifyChanges( m_toReset );
    notifyChanges( m_toResetSilent );

    evaluateSubtree( m_toTrigger );
    evaluateSubtree( m_toReset );

    m_toTrigger.clear();
    m_toReset.clear();
    m_toResetSilent.clear();

    if( m_postFlushCallback )
        m_postFlushCallback();
}

std::string TransactionalEventsUpdater::dumpRequests() const {
    auto dumpArray = []( const Events& events, const std::string& title ) {
        std::string result;

        if( !events.empty() ) {
            result.reserve( 4 * 1024 );

            result += title;
            result += '\n';
            for( const auto& event : events ) {
                result += '\t';
                result += event->getEventId();
                result += '\n';
            }
        }

        return result;
    };

    std::string dump;
    dump += dumpArray( m_toTrigger, "Requested trigger:" );
    dump += dumpArray( m_toReset, "Requested reset:" );
    dump += dumpArray( m_toResetSilent, "Requested silent reset:" );
    return dump;
}

}  // namespace SceneEngine
