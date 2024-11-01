#include "resources.h"
#include "jsonrpc/jsonrpc.h"

namespace HubPlatform {
namespace Resources {
namespace LuaBindingsError {
// Error messages:
constexpr auto SceneDoesNotExist = "The scene doesn't exist";
constexpr auto BlockDoesNotExist = "The block doesn't exist";

const std::string getMessageWithErrorCode( const JsonObject& error ) {
    std::string errorMessage;
    const auto ErrorDataField = "data";
    if( error.count( ErrorDataField ) == 1 ) {
        const auto& data = json_get_str( error, ErrorDataField );
        // Add error mapping here:
        if( data == Errors::getData( Errors::SCENE_DOES_NOT_EXIST ) ) {
            errorMessage = SceneDoesNotExist;
        }
        else if( data == Errors::getData( Errors::SCENE_BLOCK_DOES_NOT_EXIST ) ) {
            errorMessage = BlockDoesNotExist;
        }
    }
    return errorMessage;
}

}  // namespace LuaBindingsError
}  // namespace Resources
}  // namespace HubPlatform