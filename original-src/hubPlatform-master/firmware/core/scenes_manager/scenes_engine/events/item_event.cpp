#include "scenes_manager/scenes_engine/events/item_event.h"
#include "scenes_manager/scenes_engine/exceptions.h"
#include "common_utils/cpp14.h"
#include "scenes_manager/scenes_engine/model/model_resources.h"

namespace SceneEngine {

ItemEvent::ItemEvent( const ItemID& id, ItemValueProviderPtr valueProvider )
: m_valueProvider( std::move( valueProvider ) )
, m_item( id, {} )
, m_deviceArmed( ArmedState::EMPTY )
, m_eventType( EventType::ITEM ) {}

ItemEvent::ItemEvent( const ItemID& itemId, ItemValueProviderPtr valueProvider, ArmedState deviceArmed )
: m_valueProvider( std::move( valueProvider ) )
, m_item( itemId, {} )
, m_deviceArmed( deviceArmed )
, m_eventType( EventType::ALL ) {}

std::string ItemEvent::getMethodName() const {
    return Model::Resources::MethodNames::isItemState;
}

void ItemEvent::setId( const ItemID& id ) {
    m_item.first = id;
}

const ItemEvent::ItemID& ItemEvent::getId() const {
    return m_item.first;
}

const JsonObject& ItemEvent::getValue() const {
    if( m_valueProvider && m_valueProvider->fetchItemValue() ) {
        return m_valueProvider->getCachedItemValue().getValue();
    }
    return m_item.second.getValue();  // always empty
}

void ItemEvent::setEventType( ItemEvent::EventType event ) {
    m_eventType = event;
}

ItemEvent::EventType ItemEvent::getEventType() const {
    return m_eventType;
}

ItemEvent::ArmedState ItemEvent::toArmedState( bool armed ) {
    return ( armed ) ? ArmedState::TRUE : ArmedState::FALSE;
}

bool ItemEvent::compare( const ItemEvent& other ) const {
    return compare( other.m_item.first, other.getItemValue(), other.m_deviceArmed );
}

void ItemEvent::checkValue( const ItemValue& leftValue ) const {
    ItemValue rightValue( getValue() );
    if( !leftValue.isComparableWith( rightValue ) ) {
        throw SceneEngine::SceneException( Errors::SCENE_WRONG_WHEN_BLOCK,
                                           std::string{ Model::Resources::MethodNames::isItemState } +
                                               ": Value types of left and right parts differ" );
    }
}

bool ItemEvent::compare( const ItemEvent::ItemID& itemId,
                         const ItemValue& leftValue,
                         ItemEvent::ArmedState deviceArmed,
                         bool isGroup ) const {
    checkValue( leftValue );
    if( m_deviceArmed != ArmedState::EMPTY && deviceArmed != ArmedState::EMPTY ) {
        return ( isGroup || m_item.first == itemId ) && m_deviceArmed == deviceArmed && getItemValue() == leftValue;
    }
    else {
        return ( isGroup || m_item.first == itemId ) && getItemValue() == leftValue;
    }
}

bool ItemEvent::compare( const ItemValue& itemValue,
                         ItemEvent::ArmedState deviceArmed,
                         ItemEvent::EventType eventType ) const {
    if( m_eventType != ItemEvent::EventType::ALL && m_eventType != ItemEvent::EventType::EXPRESSION &&
        m_eventType != eventType ) {
        return false;
    }
    auto cachedValue = getItemValue();

    if( m_deviceArmed != ArmedState::EMPTY ) {
        auto result = m_deviceArmed == deviceArmed && cachedValue == itemValue;
        return result;
    }
    else {
        auto result = cachedValue == itemValue;
        return result;
    }
    return false;
}

bool ItemEvent::isDeviceUsed() {
    return m_deviceArmed != ArmedState::EMPTY;
}

const std::string& ItemEvent::getScale() const {
    return m_scale;
}

void ItemEvent::setScale( std::string scale ) {
    m_scale = std::move( scale );
}

const ItemValueWrapper& ItemEvent::getItemValue() const {
    if( m_valueProvider && m_valueProvider->fetchItemValue() ) {
        return m_valueProvider->getCachedItemValue();
    }
    return m_item.second;  // always empty
}

void ItemEvent::setValueKey( const ValueSourceKey& valueKey ) {
    m_valueKey = std::make_unique< ValueSourceKey >( valueKey );
}

EventBase::ValueKeys ItemEvent::getValueKeys() const {
    if( m_valueKey )
        return { *m_valueKey };
    return {};
}

}  // namespace SceneEngine
