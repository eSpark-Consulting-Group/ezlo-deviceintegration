#include "modify_scene_on_create.h"
#include "cored/model/resources/cored_resources.h"
#include "log_utils/ezlo_log.h"
#include "json_utils/json_utils.h"
#include "common_utils/cpp14.h"

namespace HubPlatform {
namespace ScenesManager {

HandlerUPtr factoryModifySceneOnCreate( ItemsTransport& remote ) {
    return std::make_unique< ModifySceneOnCreate >( remote );
}

ModifySceneOnCreate::ModifySceneOnCreate( ItemsTransport& transport )
: m_transport( transport ) {}

std::pair< bool, Json > ModifySceneOnCreate::modifyScene( SceneEngine::SceneScaleModifier& scaleModifier ) {
    if( buildIndex() ) {
        updateItems( scaleModifier );
        return { true, scaleModifier.getScene() };
    }
    return { false, Json{} };
}

bool ModifySceneOnCreate::handleItemFound( const ModifySceneOnCreate::ItemFound& item ) {
    m_itemsFound.push_back( item );
    LOGGER_TRACE_RAW( cc::trace( "ScaleModifier ItemFoundHandler: " ).append( item.key.getSourceId() ) );

    return true;
}

bool ModifySceneOnCreate::checkItemCondition( const Json& itemArrayValue ) {
    return SceneEngine::SceneScaleModifier::isNumericItem( itemArrayValue[ "type" ] );
}

bool ModifySceneOnCreate::buildIndex() {
    auto itemFoundComparator = []( const ItemFound& v1, const ItemFound& v2 ) -> bool {
        return v1.key.getSourceId() < v2.key.getSourceId();
    };
    std::sort( begin( m_itemsFound ), end( m_itemsFound ), itemFoundComparator );

    // find info about them in core
    // fetches data about items from core
    auto fetchItems = [this]( const ItemsFound & /*itemsFound*/ ) -> std::vector< Json > {
        using namespace HubPlatform::CoreD::Model::Resources;
        if( m_itemsFound.empty() ) {
            return {};
        }
        JsonArray filterItems;
        for( const ItemFound& it : m_itemsFound ) {
            filterItems.push_back( JSON( ItemFields::IdFieldName << it.key.getSourceId() ) );
        }
        Json queryTemperatureItems = JSON( "$or" << filterItems << "type"
                                                 << "temperature" );
        return m_transport.getItemsBy( queryTemperatureItems );
    };

    // merges found items with their core metadata
    auto indexItems = [this, &itemFoundComparator]( const std::vector< Json >& coreItems ) {
        IndexedItems index;

        for( const auto& item : coreItems ) {
            auto itemId = json_get_str( item, "_id" );
            ItemFound toFind{ SceneEngine::ValueSourceKey{ "", itemId } };

            auto it = std::lower_bound( begin( m_itemsFound ), end( m_itemsFound ), toFind, itemFoundComparator );
            for( ; it != m_itemsFound.end(); it++ ) {
                ItemTypeInfo typeInfo{ *it, json_get_str( item, "type" ) };

                auto foundIt = index.find( itemId );
                if( foundIt == index.end() ) {
                    index.emplace( itemId, ItemTypeInfoList{ typeInfo } );
                }
                else {
                    foundIt->second.push_back( typeInfo );
                }
            }
        }
        return index;
    };

    auto fetchedItems = fetchItems( m_itemsFound );
    m_indexedItems = indexItems( fetchedItems );

    return !m_indexedItems.empty();
}

unsigned ModifySceneOnCreate::updateItems( SceneEngine::SceneScaleModifier& scaleModifier ) {
    unsigned itemsUpdated = 0;
    for( const auto& indexedItem : m_indexedItems ) {
        const auto& valuesFound = indexedItem.second;

        for( const auto& itemTypeInfo : valuesFound ) {
            const auto& itemFound = itemTypeInfo.itemFound.get();
            if( itemTypeInfo.type != "temperature" )
                continue;
            if( !json_get_str( itemFound.valueBlock, "scale" ).empty() )
                continue;

            // add default scale
            JsonPath pathToValue = itemFound.path;
            pathToValue.append( itemFound.valueBlockRelativePath );

            Json valueBlock = itemFound.valueBlock;
            valueBlock[ "scale" ] = scaleModifier.defaultScale();
            scaleModifier.updateItemValue( pathToValue, valueBlock );

            itemsUpdated++;
            LOGGER_TRACE( cc::trace( "Item's value scale updated: " ) + cc::note( pathToValue.toString() ) );
        }
    }
    return itemsUpdated;
}

}  // namespace ScenesManager
}  // namespace HubPlatform
