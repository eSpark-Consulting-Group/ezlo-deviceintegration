#include "vidoo4hub/dictionary_manager/dictionary_manager.h"

#include "vidoo4hub/core_manager/core_manager_interface.h"

#include "cored/model/resources/cored_resources.h"
#include "cored/model/values/value_factory.h"

#include <thread>
#include <chrono>

namespace HubPlatform {
namespace Vidoo4Hub {

DictionaryManager::DictionaryManager( ICoreManager& coreManager )
: m_coreManager( coreManager ) {}

Json DictionaryManager::readDictionary( const std::string& dictionaryId ) {
    auto itemJson = m_coreManager.core().getItem( dictionaryId );
    if( itemJson.empty() )
        return {};

    auto valueKey = HubPlatform::CoreD::Model::Resources::ItemFields::ValueFieldName;
    auto value = JSON_EMPTY_OBJ;
    json_get_obj( itemJson, valueKey, value );
    return value;
}

void DictionaryManager::clearDictionary( const std::string& dictionaryId ) {
    m_coreManager.core().updateItemValue( dictionaryId, []( CoreD::Model::ValueType type ) {
        return HubPlatform::CoreD::Model::ValueFactory::createDefaultItemValue( type );
    } );
    m_coreManager.sync();
}

void DictionaryManager::updateDictionary( const std::string& dictionaryId, const Json& dictionaryJson ) {
    auto valueLoader = [&dictionaryJson]( HubPlatform::CoreD::Model::ValueType valueType ) {
        return HubPlatform::CoreD::Model::ValueFactory::createItemValue( valueType, dictionaryJson );
    };
    m_coreManager.core().updateItemValue( dictionaryId, std::move( valueLoader ) );
    m_coreManager.sync();
}

}  // namespace Vidoo4Hub
}  // namespace HubPlatform
