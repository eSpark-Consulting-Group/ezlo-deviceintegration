#include "scenes_manager/scenes_engine/events/is_item_state_changed_event.h"
#include "scenes_manager/scenes_engine/exceptions.h"
#include "jsonrpc/error_codes.h"
#include "scenes_manager/scenes_engine/model/model_resources.h"

namespace SceneEngine {

ItemStateChangedEvent::ItemStateChangedEvent( ItemValueProviderPtr startValueProvider,
                                              ItemValueProviderPtr finishValueProvider )
: m_startValueProvider( std::move( startValueProvider ) )
, m_finishValueProvider( std::move( finishValueProvider ) ) {}

std::string ItemStateChangedEvent::getMethodName() const {
    return Model::Resources::MethodNames::isItemStateChanged;
}

EventBase::ValueKeys ItemStateChangedEvent::getValueKeys() const {
    EventBase::ValueKeys keys;
    keys.reserve( 2 );

    if( m_startValueProvider && m_startValueProvider->hasValueKey() )
        keys.emplace_back( m_startValueProvider->getValueKey() );
    if( m_finishValueProvider && m_finishValueProvider->hasValueKey() )
        keys.emplace_back( m_finishValueProvider->getValueKey() );
    return keys;
}

bool ItemStateChangedEvent::compare( const ItemValue& value ) const {
    if( m_oldValue.empty() ) {
        m_oldValue.setPayload( value.getPayload() );
        return false;
    }

    return checkChangeExpected( m_oldValue, value );
}

bool ItemStateChangedEvent::checkChangeExpected( const ItemValueWrapper& oldValue,
                                                 const ItemValueWrapper& newValue ) const {
    if( !m_startValueProvider && !m_finishValueProvider ) {  // any -> any
        return true;
    }

    const ItemValueWrapper* startValue = m_startValueProvider ? fetchValue( *m_startValueProvider ) : nullptr;
    const ItemValueWrapper* finishValue = m_finishValueProvider ? fetchValue( *m_finishValueProvider ) : nullptr;

    checkValuesComparability( startValue, finishValue, oldValue, newValue );

    if( m_startValueProvider && m_finishValueProvider ) {  // value -> value
        if( startValue && finishValue ) {
            if( ( m_startConditionSatisfied || *startValue == oldValue ) && *finishValue == newValue ) {
                m_startConditionSatisfied = false;
                return true;
            }

            m_startConditionSatisfied = m_startConditionSatisfied || *startValue == newValue;
            return false;
        }
    }
    else if( m_startValueProvider ) {  // value -> any
        return startValue && *startValue == oldValue;
    }
    else {  // any -> value
        return finishValue && *finishValue == newValue;
    }

    return false;
}

const ItemValueWrapper* ItemStateChangedEvent::fetchValue( ItemValueProviderInterface& valueProvider ) {
    if( valueProvider.fetchItemValue() ) {
        return &valueProvider.getCachedItemValue();
    }
    return nullptr;
}

void ItemStateChangedEvent::checkValuesComparability( const ItemValueWrapper* startValue,
                                                      const ItemValueWrapper* finishValue,
                                                      const ItemValueWrapper& oldValue,
                                                      const ItemValueWrapper& newValue ) {
    using namespace SceneEngine::Model::Resources;

    if( startValue ) {
        if( !startValue->isComparableWith( oldValue ) )
            throw createValuesIncomparableException(
                EventFields::Start, startValue->getValueType(), "old value", oldValue.getValueType() );

        if( !startValue->isComparableWith( newValue ) )
            throw createValuesIncomparableException(
                EventFields::Start, startValue->getValueType(), "new value", newValue.getValueType() );
    }

    if( finishValue ) {
        if( !finishValue->isComparableWith( newValue ) )
            throw createValuesIncomparableException(
                EventFields::Finish, finishValue->getValueType(), "new value", newValue.getValueType() );
    }
}

SceneException ItemStateChangedEvent::createValuesIncomparableException( const char* paramName,
                                                                         const std::string& paramValueType,
                                                                         const char* otherValueName,
                                                                         const std::string& otherValueType ) {
    std::string methodName = SceneEngine::Model::Resources::MethodNames::isItemStateChanged;
    std::string message = methodName + ": '" + paramName + "' is incomparable with " + otherValueName + " of the item ";
    std::string types = '(' + paramValueType + " - " + otherValueType + ')';

    return SceneException( Errors::SCENE_WHEN_TYPE_MISMATCH, message + types );
}

}  // namespace SceneEngine
