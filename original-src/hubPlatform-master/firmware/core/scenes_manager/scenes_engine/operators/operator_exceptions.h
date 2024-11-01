#pragma once

#include "scenes_manager/scenes_engine/exceptions.h"

namespace SceneEngine {
class LatchException : public SceneException {
public:
    explicit LatchException( const char* message );
    explicit LatchException( const std::string& message );
};

}  // namespace SceneEngine
