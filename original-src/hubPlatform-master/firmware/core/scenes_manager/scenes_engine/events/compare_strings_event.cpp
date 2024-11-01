#include "scenes_manager/scenes_engine/events/compare_strings_event.h"
#include "scenes_manager/scenes_engine/exceptions.h"
#include "scenes_manager/scenes_engine/model/model_resources.h"
#include <utility>

namespace SceneEngine {

CompareStringsEvent::CompareStringsEvent( ItemValue itemValue, Comparator comparator )
: m_itemValue( std::move( itemValue ) )
, m_comparator( std::move( comparator ) ) {}

CompareStringsEvent::CompareStringsEvent( ItemValueProviderPtr valueProvider, Comparator comparator )
: m_valueProvider( std::move( valueProvider ) )
, m_comparator( std::move( comparator ) ) {}

std::string CompareStringsEvent::getMethodName() const {
    return Model::Resources::MethodNames::compareStrings;
}

void CompareStringsEvent::checkValue( const ItemValue& leftValue ) const {
    const ItemValue& rightValue = getValue();
    if( !leftValue.isComparableWith( rightValue ) ) {
        throw SceneEngine::SceneException( Errors::SCENE_WRONG_WHEN_BLOCK,
                                           getMethodName() + ": Value types of left and right parts differ" );
    }
}

bool CompareStringsEvent::compare( const ItemValue& leftValue ) const {
    const ItemValue& rightValue = getValue();
    checkValue( leftValue );
    const bool compareResult = m_comparator( leftValue, rightValue );
    if( m_keepState ) {
        const bool result = compareResult && !m_isTriggered;
        m_isTriggered = compareResult;
        return result;
    }

    return compareResult;
}

void CompareStringsEvent::setKeepState( bool value ) {
    m_keepState = value;
}

const ItemEventBase::ItemValue& CompareStringsEvent::getValue() const {
    if( m_valueProvider != nullptr && m_valueProvider->fetchItemValue() ) {
        return m_valueProvider->getCachedItemValue();
    }
    return m_itemValue;
}

EventBase::ValueKeys CompareStringsEvent::getValueKeys() const {
    if( m_valueProvider && m_valueProvider->hasValueKey() )
        return { m_valueProvider->getValueKey() };
    return {};
}

}  // namespace SceneEngine
