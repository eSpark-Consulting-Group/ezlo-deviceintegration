#include "scenes_manager/scenes_engine/events/in_array_event.h"
#include "scenes_manager/scenes_engine/exceptions.h"
#include "scenes_manager/scenes_engine/model/model_resources.h"

namespace SceneEngine {

InArrayEvent::InArrayEvent( ValuesArray array, const std::string& operation )
: m_array( std::move( array ) ) {
    if( operation.empty() ) {
        m_operation = Model::Resources::ArrayOperations::In;
    }
    else {
        validateOperation( operation );
        m_operation = operation;
    }

    validateArray( m_array );
}

std::string InArrayEvent::getMethodName() const {
    return Model::Resources::MethodNames::inArray;
}

void InArrayEvent::checkValue( const ItemValue& leftValue ) const {
    const ItemValue& rightValue = m_array.front();
    if( !leftValue.isComparableWith( rightValue ) ) {
        throw SceneEngine::SceneException(
            Errors::SCENE_WRONG_WHEN_BLOCK,
            std::string{ Model::Resources::MethodNames::inArray } + ": Value types of left and right parts differ" );
    }
}

bool InArrayEvent::compare( const ItemValue& leftValue ) const {
    checkValue( leftValue );
    const bool compareResult = checkOperation( leftValue, m_array, m_operation );
    if( m_keepState ) {
        const bool result = compareResult && !m_isTriggered;
        m_isTriggered = compareResult;
        return result;
    }

    return compareResult;
}

void InArrayEvent::setKeepState( bool value ) {
    m_keepState = value;
}

bool InArrayEvent::checkOperation( const ItemValue& value, const ValuesArray& array, const std::string& operation ) {
    const ItemValue& sample = array.front();
    if( !sample.isComparableWith( value ) )
        return false;

    auto it = std::find( array.begin(), array.end(), value );
    if( operation == Model::Resources::ArrayOperations::In ) {
        return it != array.end();
    }
    return it == array.end();
}

void InArrayEvent::validateOperation( const std::string& operation ) {
    namespace ArrayOperations = Model::Resources::ArrayOperations;

    const bool isOperationValid = operation == ArrayOperations::In || operation == ArrayOperations::NotIn;
    if( !isOperationValid ) {
        const std::string message = "inArray: unknown operation: " + operation;
        throw SceneException( Errors::SCENE_WRONG_WHEN_BLOCK, message.c_str() );
    }
}

void InArrayEvent::validateArray( const ValuesArray& array ) {
    constexpr std::size_t minArraySize = 2;
    if( array.size() < minArraySize ) {
        const std::string message = "inArray: array should have at least 2 values, for comparing against a single "
                                    "value use 'isItemState' method";
        throw SceneException( Errors::SCENE_WRONG_WHEN_BLOCK, message.c_str() );
    }
}

}  // namespace SceneEngine
