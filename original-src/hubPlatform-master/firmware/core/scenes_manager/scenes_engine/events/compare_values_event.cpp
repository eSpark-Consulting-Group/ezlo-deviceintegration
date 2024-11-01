#include "scenes_manager/scenes_engine/events/compare_values_event.h"
#include "scenes_manager/scenes_engine/helpers/convert_comparator.h"
#include "cored/model/values/value_type_traits.h"
#include "tiny/tiny_consolecolors.h"
#include "cored/model/values/value_type.h"
#include "scenes_manager/scenes_engine/model/model_resources.h"
#include <utility>

namespace SceneEngine {

CompareValuesEvent::CompareValuesEvent( ItemValue itemValue, std::string comparator, std::string valueType )
: m_itemValue( std::move( itemValue ) )
, m_comparator( std::move( comparator ) )
, m_valueType( std::move( valueType ) ) {
    checkEmptyValueType();
    if( m_comparator.empty() )
        m_comparator = Model::Resources::ValueComparators::Equal;
    checkComparator();
}

CompareValuesEvent::CompareValuesEvent( ItemValueProviderPtr valueProvider,
                                        std::string comparator,
                                        std::string valueType )
: m_valueProvider( std::move( valueProvider ) )
, m_comparator( std::move( comparator ) )
, m_valueType( std::move( valueType ) ) {
    checkEmptyValueType();
    if( m_comparator.empty() )
        m_comparator = Model::Resources::ValueComparators::Equal;
    checkComparator();
}

std::string CompareValuesEvent::getMethodName() const {
    return Model::Resources::MethodNames::compareValues;
}

void CompareValuesEvent::checkValue( const ItemValue& leftValue ) const {
    const ItemValue& rightValue = getValue();
    if( leftValue.getValueType() != rightValue.getValueType() ) {
        throw SceneEngine::SceneException( Errors::SCENE_WHEN_TYPE_MISMATCH,
                                           std::string{ Model::Resources::MethodNames::compareValues } +
                                               ": Value types of left and right parts differ" );
    }
}

bool CompareValuesEvent::compare( const ItemValue& leftValue ) const {
    const ItemValue& rightValue = getValue();

    checkValue( leftValue );
    if( leftValue.getValueType() != m_valueType ) {
        throw SceneEngine::SceneException(
            Errors::SCENE_WRONG_WHEN_BLOCK,
            std::string{ Model::Resources::MethodNames::compareValues } + ": event value type restricted to " +
                m_valueType + ", but left value has different value type: " + leftValue.getValueType() );
    }

    if( m_comparatorMethod == nullptr ) {
        checkEmptyValueType();

        using namespace HubPlatform;
        auto valueType = CoreD::Model::ValueType::fromString( leftValue.getValueType() );
        bool isLess = CoreD::Model::isValueTypeValid( valueType ) && CoreD::Model::hasIsLess( valueType );

        try {
            m_comparatorMethod = SceneEngine::convertComparator( m_comparator, isLess );
        }
        catch( const SceneEngine::ConvertComparatorException& ex ) {
            throw SceneEngine::SceneException( Errors::SCENE_WRONG_WHEN_BLOCK,
                                               std::string{ Model::Resources::MethodNames::compareValues } +
                                                   ": Invalid comparator passed: " + m_comparator );
        }
    }
    bool compareResult = m_comparatorMethod( leftValue, rightValue );

    if( m_keepState ) {
        bool result = compareResult && !m_isTriggered;
        m_isTriggered = compareResult;
        return result;
    }

    return compareResult;
}

void CompareValuesEvent::setKeepState( bool value ) {
    m_keepState = value;
}

const ItemEventBase::ItemValue& CompareValuesEvent::getValue() const {
    if( m_valueProvider != nullptr && m_valueProvider->fetchItemValue() ) {
        return m_valueProvider->getCachedItemValue();
    }
    return m_itemValue;
}

EventBase::ValueKeys CompareValuesEvent::getValueKeys() const {
    if( m_valueProvider && m_valueProvider->hasValueKey() )
        return { m_valueProvider->getValueKey() };
    return {};
}

const std::string& CompareValuesEvent::getValueType() const {
    return m_valueType;
}

void CompareValuesEvent::checkEmptyValueType() const {
    if( m_valueType.empty() ) {
        throw SceneEngine::SceneException(
            Errors::SCENE_WRONG_WHEN_BLOCK,
            std::string{ Model::Resources::MethodNames::compareValues } + ": empty valueType member" );
    }
}

void CompareValuesEvent::checkComparator() const {
    try {
        using namespace HubPlatform::CoreD::Model;
        auto valueType = ValueType::fromString( m_valueType );
        bool isLess = isValueTypeValid( valueType ) && hasIsLess( valueType );

        SceneEngine::convertComparator( m_comparator, isLess );
    }
    catch( const SceneEngine::ConvertComparatorException& ex ) {
        throw SceneEngine::SceneException( Errors::SCENE_WRONG_WHEN_BLOCK,
                                           std::string{ Model::Resources::MethodNames::compareValues } +
                                               ": Invalid comparator passed: " + m_comparator );
    }
}

}  // namespace SceneEngine
