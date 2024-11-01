#include "modify_scene.h"
#include "cored/model/resources/cored_resources.h"
#include "json_utils/json_utils.h"
#include "json_utils/json_wrapper.h"

namespace HubPlatform {
namespace ScenesManager {

ModifyScene::ModifyScene( HandlerUPtr handler )
: m_handler( std::move( handler ) ) {}

void ModifyScene::setHandler( HandlerUPtr handler ) {
    m_handler = std::move( handler );
}

std::pair< bool, Json > ModifyScene::processScene( Json scene, std::string defaultScale ) {
    LOGGER_TRACE(
        cc::trace( "Default scale: " ).append( defaultScale ).append( ", scene: " ).append( cc::j( scene ) ) );

    if( m_handler ) {
        SceneEngine::SceneScaleModifier scaleModifier( std::move( scene ), std::move( defaultScale ) );
        scaleModifier.onItemFound( [this]( const ItemFound& item ) { m_handler->handleItemFound( item ); } );
        scaleModifier.onCheckItemCondition(
            [this]( const Json& itemArrayValue ) { return m_handler->checkItemCondition( itemArrayValue ); } );

        if( scaleModifier.process() ) {
            return m_handler->modifyScene( scaleModifier );
        }
    }
    return std::pair< bool, Json >( false, nullptr );
}

}  // namespace ScenesManager
}  // namespace HubPlatform
