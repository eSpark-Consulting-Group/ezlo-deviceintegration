#include "scenes_manager/scenes_engine/operators/operator_exceptions.h"

namespace SceneEngine {

LatchException::LatchException( const char* message )
: SceneException( Errors::SCENE_WRONG_WHEN_BLOCK, message ) {}

LatchException::LatchException( const std::string& message )
: SceneException( Errors::SCENE_WRONG_WHEN_BLOCK, message ) {}

}  // namespace SceneEngine
