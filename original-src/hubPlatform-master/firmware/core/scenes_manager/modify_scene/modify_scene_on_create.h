#pragma once

#include "scenes_manager/modify_scene/modify_scene_handler.h"
#include "scenes_manager/modify_scene/items_transport.h"

namespace HubPlatform {
namespace ScenesManager {

class ModifySceneOnCreate : public ModifySceneHandler {
public:
    using ItemFound = SceneEngine::SceneScaleModifier::ItemFound;
    explicit ModifySceneOnCreate( ItemsTransport& transport );

public:
    std::pair< bool, Json > modifyScene( SceneEngine::SceneScaleModifier& scaleModifier ) override;
    bool handleItemFound( const ItemFound& item ) override;
    bool checkItemCondition( const Json& itemArrayValue ) override;

private:
    struct ItemTypeInfo {
        std::reference_wrapper< ItemFound > itemFound;
        std::string type;
    };
    using ItemTypeInfoList = std::vector< ItemTypeInfo >;
    using IndexedItems = std::map< std::string, ItemTypeInfoList >;

    bool buildIndex();
    unsigned updateItems( SceneEngine::SceneScaleModifier& scaleModifier );

private:
    using ItemsFound = std::vector< ItemFound >;
    ItemsFound m_itemsFound;
    IndexedItems m_indexedItems;

    ItemsTransport& m_transport;
};

HandlerUPtr factoryModifySceneOnCreate( ItemsTransport& remote );

}  // namespace ScenesManager
}  // namespace HubPlatform