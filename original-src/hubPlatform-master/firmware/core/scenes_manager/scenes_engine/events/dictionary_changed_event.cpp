#include "scenes_manager/scenes_engine/events/dictionary_changed_event.h"
#include "log_utils/ezlo_log.h"
#include "scenes_manager/scenes_engine/model/model_resources.h"

#include <utility>

namespace SceneEngine {

DictionaryChangedEvent::DictionaryChangedEvent( ItemId itemId, KeyValue keyValue, Operation operation )
: m_itemId( std::move( itemId ) )
, m_keyValue( std::move( keyValue ) )
, m_operation( std::move( operation ) ) {}

std::string DictionaryChangedEvent::getMethodName() const {
    return Model::Resources::MethodNames::isDictionaryChanged;
}

bool DictionaryChangedEvent::compare( const KeyValue& keyValue, const Operation& operation ) const {
    return m_operation == operation && m_keyValue == keyValue;
}

const DictionaryChangedEvent::KeyValue& DictionaryChangedEvent::getKeyValue() const {
    return m_keyValue;
}

}  // namespace SceneEngine
