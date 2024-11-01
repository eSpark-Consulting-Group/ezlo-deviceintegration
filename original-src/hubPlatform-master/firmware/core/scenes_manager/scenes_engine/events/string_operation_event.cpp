#include "scenes_manager/scenes_engine/events/string_operation_event.h"
#include "scenes_manager/scenes_engine/exceptions.h"
#include "scenes_manager/scenes_engine/events/comparable_value.h"
#include "scenes_manager/scenes_engine/model/model_resources.h"

namespace SceneEngine {

StringOperationEvent::StringOperationEvent( ItemValue itemValue, std::string operation )
: m_itemValue( std::move( itemValue ) )
, m_operation( std::move( operation ) ) {
    validateOperation( m_operation );
    validateValue( m_operation, m_itemValue );
}

StringOperationEvent::StringOperationEvent( ItemValueProviderPtr valueProvider, std::string operation )
: m_valueProvider( std::move( valueProvider ) )
, m_operation( std::move( operation ) ) {
    validateOperation( m_operation );
}

std::string StringOperationEvent::getMethodName() const {
    return Model::Resources::MethodNames::stringOperation;
}

EventBase::ValueKeys StringOperationEvent::getValueKeys() const {
    if( m_valueProvider && m_valueProvider->hasValueKey() )
        return { m_valueProvider->getValueKey() };
    return {};
}

bool StringOperationEvent::compare( const ItemValue& leftValue ) const {
    const auto& rightValue = getValue();

    using namespace Model::Resources::StringOperations;
    checkValue( leftValue );

    const bool compareResult = checkOperation( leftValue, rightValue, m_operation );
    if( m_keepState ) {
        const bool result = compareResult && !m_isTriggered;
        m_isTriggered = compareResult;
        return result;
    }

    return compareResult;
}

void StringOperationEvent::setKeepState( bool value ) {
    m_keepState = value;
}

void StringOperationEvent::checkValue( const ItemValue& leftValue ) const {
    namespace StringOperations = Model::Resources::StringOperations;
    const ItemValue& rightValue = getValue();
    if( m_operation == StringOperations::Length || m_operation == StringOperations::NotLength ) {
        const Json& rightValueJson = rightValue.getValue();
        if( !rightValueJson.is_number_integer() ) {
            std::string message =
                getMethodName() + ": '" + m_operation + "' operation requires right operand to be a number";
            throw SceneException( Errors::SCENE_WRONG_WHEN_BLOCK, message.c_str() );
        }
    }
    else if( !leftValue.isComparableWith( rightValue ) ) {
        std::string message = getMethodName() + ": incomparable values used for '" + m_operation + "' operation";
        throw SceneException( Errors::SCENE_WRONG_WHEN_BLOCK, message.c_str() );
    }
}

const ItemEventBase::ItemValue& StringOperationEvent::getValue() const {
    if( m_valueProvider && m_valueProvider->fetchItemValue() ) {
        return m_valueProvider->getCachedItemValue();
    }
    return m_itemValue;
}

bool StringOperationEvent::checkOperation( const ItemValue& left,
                                           const ItemValue& right,
                                           const std::string& operation ) {
    namespace StringOperations = Model::Resources::StringOperations;

    const Json& leftValue = left.getValue();
    if( !leftValue.is_string() )
        return false;

    const std::string leftString = leftValue.get< std::string >();
    const std::size_t leftSize = leftString.size();

    const Json& rightValue = right.getValue();
    if( rightValue.is_number() ) {
        const int rightInt = rightValue.get< int >();
        if( operation == StringOperations::Length )
            return static_cast< int >( leftSize ) == rightInt;
        if( operation == StringOperations::NotLength )
            return static_cast< int >( leftSize ) != rightInt;
    }

    if( !rightValue.is_string() )
        return false;

    const std::string rightString = right.getValue().get< std::string >();
    const std::size_t rightSize = rightString.size();

    if( operation == StringOperations::Begin )
        return leftSize >= rightSize && leftString.compare( 0, rightSize, rightString ) == 0;
    if( operation == StringOperations::End )
        return leftSize >= rightSize && leftString.compare( leftSize - rightSize, rightSize, rightString ) == 0;
    if( operation == StringOperations::Contain )
        return leftString.find( rightString ) != std::string::npos;

    if( operation == StringOperations::NotBegin )
        return leftSize < rightSize || leftString.compare( 0, rightSize, rightString ) != 0;
    if( operation == StringOperations::NotEnd )
        return leftSize < rightSize || leftString.compare( leftSize - rightSize, rightSize, rightString ) != 0;
    if( operation == StringOperations::NotContain )
        return leftString.find( rightString ) == std::string::npos;

    return false;
}

void StringOperationEvent::validateOperation( const std::string& operation ) {
    namespace StringOperations = Model::Resources::StringOperations;

    const bool isOperationValid = operation == StringOperations::Begin || operation == StringOperations::End ||
                                  operation == StringOperations::Contain || operation == StringOperations::Length ||
                                  operation == StringOperations::NotBegin || operation == StringOperations::NotEnd ||
                                  operation == StringOperations::NotContain || operation == StringOperations::NotLength;

    if( !isOperationValid ) {
        const std::string message = "stringOperation: unknown operation: " + operation;
        throw SceneException( Errors::SCENE_WRONG_WHEN_BLOCK, message.c_str() );
    }
}

void StringOperationEvent::validateValue( const std::string& operation, const ItemValue& itemValue ) {
    namespace StringOperations = Model::Resources::StringOperations;

    const Json& value = itemValue.getValue();
    if( operation == StringOperations::Length || operation == StringOperations::NotLength ) {
        if( !value.is_number() ) {
            std::string message = "stringOperation: '" + operation + "' operation requires operand to be a number";
            throw SceneException( Errors::SCENE_WRONG_WHEN_BLOCK, message.c_str() );
        }
    }
    else if( !value.is_string() ) {
        std::string message =
            "stringOperation: '" + operation + "' operation requires operand to be a non-empty string";
        throw SceneException( Errors::SCENE_WRONG_WHEN_BLOCK, message.c_str() );
    }
    else {
        const std::string valueStr = value.get< std::string >();
        if( valueStr.empty() ) {
            std::string message =
                "stringOperation: '" + operation + "' operation requires operand to be a non-empty string";
            throw SceneException( Errors::SCENE_WRONG_WHEN_BLOCK, message.c_str() );
        }
    }
}

}  // namespace SceneEngine
