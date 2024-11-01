#pragma once

#include "modify_scene_handler.h"

namespace HubPlatform {
namespace ScenesManager {

class ModifySceneOnFetch : public ModifySceneHandler {
    using ItemFound = SceneEngine::SceneScaleModifier::ItemFound;
    using SceneScaleModifier = SceneEngine::SceneScaleModifier;

public:
    std::pair< bool, Json > modifyScene( SceneEngine::SceneScaleModifier& scaleModifier ) override;
    bool handleItemFound( const ItemFound& item ) override;
    bool checkItemCondition( const Json& itemArrayValue ) override;

private:
    unsigned updateItems( SceneEngine::SceneScaleModifier& scaleModifier );
    static double convertToScale( double valueBefore, const std::string& fromScale, const std::string& toScale );

private:
    using ItemsFound = std::vector< ItemFound >;
    ItemsFound m_itemsFound;
};

HandlerUPtr factoryModifySceneOnFetch();

}  // namespace ScenesManager
}  // namespace HubPlatform