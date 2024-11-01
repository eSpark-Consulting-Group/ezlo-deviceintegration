#pragma once

#include "scene_scale_modifier.h"
#include "modify_scene_handler.h"
#include <utility>

namespace HubPlatform {
namespace ScenesManager {

class ModifyScene {
public:
    using ItemFound = SceneEngine::SceneScaleModifier::ItemFound;

public:
    explicit ModifyScene( HandlerUPtr handler );
    void setHandler( HandlerUPtr handler );
    std::pair< bool, Json > processScene( Json scene, std::string defaultScale );

private:
    HandlerUPtr m_handler;
};

}  // namespace ScenesManager
}  // namespace HubPlatform