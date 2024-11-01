#include "scenes_manager/scenes_engine/event_container.h"

namespace SceneEngine {

void EventContainer::addItemEvent( const ValueSourceKey& key, ItemEventPtr eventPtr ) {
    addSearchableEvent( m_itemEvents, key, std::move( eventPtr ) );
}

void EventContainer::addItemStateChangedEvent( const ValueSourceKey& key, ItemStateChangedEventPtr eventPtr ) {
    addSearchableEvent( m_itemStateChangedEvents, key, std::move( eventPtr ) );
}

void EventContainer::addItemStateDeviceItemGroupsEvent( const ValueSourceKey& key, ItemEventPtr eventPtr ) {
    addSearchableEvent( m_itemGroupEvents, key, std::move( eventPtr ) );
}

void EventContainer::addExpressionItem( const ValueSourceKey& key, EventBasePtr eventPtr ) {
    m_expressionItems.insert( { key, std::move( eventPtr ) } );
}

void EventContainer::addCompareNumbersEvent( const ValueSourceKey& key, ItemEventBasePtr eventPtr ) {
    addSearchableEvent( m_compareNumbersEvents, key, std::move( eventPtr ) );
}

void EventContainer::addDeviceItemGroupsEvent( const ValueSourceKey& key, ItemEventBasePtr eventPtr ) {
    addSearchableEvent( m_deviceItemGroupsEvents, key, std::move( eventPtr ) );
}

void EventContainer::addCompareNumbersRangeEvent( const ValueSourceKey& key, ItemEventBasePtr eventPtr ) {
    addSearchableEvent( m_compareNumbersRangeEvents, key, std::move( eventPtr ) );
}

void EventContainer::addCompareStringsEvent( const ValueSourceKey& key, ItemEventBasePtr eventPtr ) {
    addSearchableEvent( m_compareStringsEvents, key, std::move( eventPtr ) );
}

void EventContainer::addStringOperationEvent( const ValueSourceKey& key, ItemEventBasePtr eventPtr ) {
    addSearchableEvent( m_stringOperationEvents, key, std::move( eventPtr ) );
}

void EventContainer::addInArrayEvent( const ValueSourceKey& key, ItemEventBasePtr eventPtr ) {
    addSearchableEvent( m_inArrayEvents, key, std::move( eventPtr ) );
}

template < class Event, class EventPtr >
void EventContainer::addSearchableEvent( std::vector< Event >& container,
                                         const ValueSourceKey& key,
                                         EventPtr eventPtr ) {
    if( eventPtr ) {
        auto eventKeys = eventPtr->getValueKeys();
        for( const ValueSourceKey& eventKey : eventKeys ) {
            if( eventKey.isExpression() ) {
                addExpressionItem( key, eventPtr );
            }
        }
        container.emplace_back( key, std::move( eventPtr ) );
    }
}

const EventContainer::ItemEvents& EventContainer::getItemEvents() const {
    return m_itemEvents;
}

const EventContainer::ItemStateChangedEvents& EventContainer::getItemStateChangedEvents() const {
    return m_itemStateChangedEvents;
}

const EventContainer::ItemEvents& EventContainer::getItemGroupStateEvents() const {
    return m_itemGroupEvents;
}

const EventContainer::ExpressionItems& EventContainer::getExpressionItems() const {
    return m_expressionItems;
}

const EventContainer::CompareNumbersEvents& EventContainer::getCompareNumbersEvents() const {
    return m_compareNumbersEvents;
}

const EventContainer::CompareNumbersRangeEvents& EventContainer::getCompareNumbersRangeEvents() const {
    return m_compareNumbersRangeEvents;
}

const EventContainer::CompareStringsEvents& EventContainer::getCompareStringsEvents() const {
    return m_compareStringsEvents;
}

const EventContainer::FeatureStateEvents& EventContainer::getFeatureStateEvents() const {
    return m_featureStateEvents;
}

const EventContainer::StringOperationEvents& EventContainer::getStringOperationEvents() const {
    return m_stringOperationEvents;
}

const EventContainer::InArrayEvents& EventContainer::getInArrayEvents() const {
    return m_inArrayEvents;
}

const EventContainer::DeviceStateEvents& EventContainer::getIsDeviceStateEvents() const {
    return deviceStateEvents;
}

const EventContainer::DeviceStateEvents& EventContainer::getIsDeviceStateGroupEvents() const {
    return deviceStateGroupEvents;
}

const EventContainer::DeviceItemGroupsEvents& EventContainer::getDeviceItemGroupsEvents() const {
    return m_deviceItemGroupsEvents;
}

void EventContainer::addCompareValuesEvent( const ValueSourceKey& key, ItemEventBasePtr eventPtr ) {
    addSearchableEvent( m_compareValuesEvents, key, std::move( eventPtr ) );
}

const EventContainer::CompareValuesEvents& EventContainer::getCompareValuesEvents() const {
    return m_compareValuesEvents;
}

void EventContainer::clear() {
    *this = EventContainer{};
}

bool EventContainer::empty() const {
    return events.empty() && operators.empty();
}

AbstractOperator::SharedOperator EventContainer::findBlockOperator( const BlockId& blockId ) const {
    auto itEvent = events.find( blockId );
    if( itEvent != events.end() ) {
        const EventBasePtr& event = itEvent->second;
        return event->getOperator();
    }

    auto itOperator = operators.find( blockId );
    if( itOperator != operators.end() ) {
        return itOperator->second;
    }

    return nullptr;
}

}  // namespace SceneEngine
