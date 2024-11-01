#include "scenes_manager/scenes_engine/events/dictionary_value_event.h"
#include "cored/model/resources/cored_resources.h"
#include "json_utils/json_utils.h"
#include "scenes_manager/scenes_engine/model/model_resources.h"

namespace SceneEngine {

DictionaryValueEvent::DictionaryValueEvent( const ElementValue& elementValue )
: m_elementValue( elementValue )
, m_operations( { "added", "updated" } ) {}

DictionaryValueEvent::DictionaryValueEvent( ElementValue&& elementValue )
: m_elementValue( std::move( elementValue ) )
, m_operations( { "added", "updated" } ) {}

std::string DictionaryValueEvent::getMethodName() const {
    return Model::Resources::MethodNames::hasAtLeastOneDictionaryValue;
}

bool DictionaryValueEvent::compare( const Operation& operation, const ElementValue& element ) const {
    using namespace HubPlatform::CoreD::Model::Resources;
    if( element.empty() ) {
        LOGGER_DEBUG( "empty element" );
        return false;
    }
    if( m_operations.find( operation ) == m_operations.end() ) {
        return false;
    }
    return element == m_elementValue;
}

const DictionaryValueEvent::ElementValue& DictionaryValueEvent::getElementValue() const {
    return m_elementValue;
}

}  // namespace SceneEngine
