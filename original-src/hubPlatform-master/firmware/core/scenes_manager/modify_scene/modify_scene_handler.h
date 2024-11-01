#pragma once

#include "scene_scale_modifier.h"
#include <utility>
#include <memory>

namespace HubPlatform {
namespace ScenesManager {

class ModifySceneHandler {  // strategy base
public:
    virtual ~ModifySceneHandler() = default;

    using ItemFound = SceneEngine::SceneScaleModifier::ItemFound;

    virtual std::pair< bool, Json > modifyScene( SceneEngine::SceneScaleModifier& scaleModifier ) = 0;
    virtual bool handleItemFound( const ItemFound& item ) = 0;
    virtual bool checkItemCondition( const Json& arrayItemValue ) = 0;
};
using HandlerUPtr = std::unique_ptr< ModifySceneHandler >;

}  // namespace ScenesManager
}  // namespace HubPlatform