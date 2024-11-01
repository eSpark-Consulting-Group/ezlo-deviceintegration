#pragma once

#include "scenes_manager/scenes_engine/scene.h"
#include "variant/variant.hpp"
#include <string>

namespace SceneEngine {

struct ActionExceptionData {
    struct BlockReference {
        Scene::SceneID sceneId;
        std::string blockId;
    };
    struct ExpressionReference {
        std::string name;
    };

    using Storage = mpark::variant< mpark::monostate, BlockReference, ExpressionReference >;

    ActionExceptionData() = default;
    ActionExceptionData( BlockReference blockReference )
    : storage( std::move( blockReference ) ) {}
    ActionExceptionData( ExpressionReference expressionReference )
    : storage( std::move( expressionReference ) ) {}

public:
    Storage storage;
};

}  // namespace SceneEngine
