#include "modify_scene_on_fetch.h"
#include "cored/model/resources/cored_resources.h"
#include "cored/model/values/value_type.h"
#include "json_utils/json_utils.h"
#include "json_utils/json_wrapper.h"
#include "common_utils/cpp14.h"

namespace HubPlatform {
namespace ScenesManager {

HandlerUPtr factoryModifySceneOnFetch() {
    return std::make_unique< ModifySceneOnFetch >();
}

std::pair< bool, Json > ModifySceneOnFetch::modifyScene( SceneEngine::SceneScaleModifier& scaleModifier ) {
    if( !m_itemsFound.empty() ) {
        updateItems( scaleModifier );
        return { true, scaleModifier.getScene() };
    }
    return { false, Json{} };
}

bool ModifySceneOnFetch::handleItemFound( const ModifySceneOnFetch::ItemFound& item ) {
    m_itemsFound.push_back( item );
    LOGGER_TRACE_RAW( cc::trace( "ScaleModifier ItemFoundHandler: " ).append( item.key.getSourceId() ) );

    return true;
}

bool ModifySceneOnFetch::checkItemCondition( const Json& itemArrayValue ) {
    if( SceneScaleModifier::isNumericItem( itemArrayValue[ "type" ] ) ) {
        const auto& scale = json_get_str( itemArrayValue, "scale" );

        return ( scale == "celsius" || scale == "fahrenheit" );  // TODO: those values should not be hardcoded
    }
    return false;
}

unsigned ModifySceneOnFetch::updateItems( SceneEngine::SceneScaleModifier& scaleModifier ) {
    unsigned itemsUpdated = 0;
    for( const auto& itemFound : m_itemsFound ) {
        auto const& scaleBefore = json_get_str( itemFound.valueBlock, "scale" );

        if( scaleBefore == scaleModifier.defaultScale() )
            continue;

        // scales are differ, convert to default scale
        JsonPath pathToValue = itemFound.path;
        pathToValue.append( itemFound.valueBlockRelativePath );

        Json valueBlock = itemFound.valueBlock;
        valueBlock[ "scale" ] = scaleModifier.defaultScale();
        auto tempBefore = json_get_float( valueBlock, "value", -101 );
        auto tempAfter = convertToScale( tempBefore, scaleBefore, scaleModifier.defaultScale() );
        valueBlock[ "value" ] = tempAfter;

        LOGGER_TRACE( cc::trace( "valueBlock modified: from " ) + cc::j( itemFound.valueBlock ) + cc::trace( " to " ) +
                      cc::j( valueBlock ) );

        scaleModifier.updateItemValue( pathToValue, valueBlock );
        itemsUpdated++;
        LOGGER_TRACE( cc::trace( "Item's scale updated: " + itemFound.key.getSourceId() ) );
    }
    return itemsUpdated;
}

double ModifySceneOnFetch::convertToScale( double valueBefore,
                                           const std::string& fromScale,
                                           const std::string& toScale ) {
    CoreD::Model::ValueType valueType( CoreD::Model::ValueType::Enum::Temperature );
    auto converted = valueType.getConverter()( valueBefore, fromScale.c_str(), toScale.c_str() );

    return converted;
}

}  // namespace ScenesManager
}  // namespace HubPlatform
