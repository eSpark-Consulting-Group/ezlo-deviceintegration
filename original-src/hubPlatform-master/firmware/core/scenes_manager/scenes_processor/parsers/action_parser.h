#include "scenes_manager/scenes_processor/id_utils.h"
#include "scenes_manager/scenes_engine/actions/action.h"
#include "scenes_manager/scenes_engine/actions/action_block.h"
#include "scenes_manager/scenes_engine/events/item_value_provider.h"
#include "cored/remote_interface/cored_interface.h"
#include "extensions_manager/remote_interface/em_remote_interface.h"
#include "json_utils/json_utils.h"

namespace HubPlatform {
namespace ScenesManager {

class ActionParserInterface {
public:
    enum class BlockType { Then, Else };

    virtual ~ActionParserInterface();
    virtual SceneEngine::ActionBlocks::Actions parse( Json& sceneJsonObject,
                                                      BlockType blockType,
                                                      IdUtils::PlaceholdersMapping& placeholdersMapping ) = 0;
    virtual SceneEngine::ActionBlocks::ActionGroups
    parseGroups( Json& sceneJsonObject, BlockType blockType, IdUtils::PlaceholdersMapping& placeholdersMapping ) = 0;
};

class ActionParser : public ActionParserInterface {
public:
    using ItemId = std::string;
    using ItemValue = JsonObject;
    using HouseMode = std::string;
    using HouseModes = std::set< HouseMode >;
    using ControllerId = std::string;

    using MakeExpressionValueProvider =
        std::function< SceneEngine::ItemValueProviderPtr( const std::string& expressionName ) >;

    ActionParser( ControllerId controllerId,
                  CoreDRemoteInterface::CoreDInterface& core,
                  ExtensionsManager::RemoteInterface& extensionManager,
                  MakeExpressionValueProvider makeExpressionValueProvider );

    SceneEngine::ActionBlocks::Actions parse( Json& sceneJsonObject,
                                              BlockType blockType,
                                              IdUtils::PlaceholdersMapping& placeholdersMapping ) override;
    SceneEngine::ActionBlocks::ActionGroups parseGroups( Json& sceneJsonObject,
                                                         BlockType blockType,
                                                         IdUtils::PlaceholdersMapping& placeholdersMapping ) override;

private:
    SceneEngine::ActionBlocks::Actions parseArrayOfActions( const std::string& sceneId,
                                                            SceneEngine::ExecutionPolicy policy,
                                                            Json& arrayOfBlocks );
    JsonArray& getBlock( JsonObject& sceneJsonObject, const std::string& blockName, Json::value_t elementType );
    Errors::Error getWrongBlockError( const std::string& blockName );

    static SceneEngine::ExecutionPolicy getExecutionPolicy( const Json& json,
                                                            SceneEngine::ExecutionPolicy defaultValue );
    static std::chrono::seconds parseDelay( const JsonObject& then_block );

    void parseControlFlow( JsonObject& block,
                           const std::string& sceneId,
                           SceneEngine::Action::InitData& actionInitData );
    SceneEngine::ActionExceptionData parseExceptionData( JsonObject& exceptionDataObj, const std::string& sceneId );

    SceneEngine::Action::Arguments parseArguments( const JsonObject& args,
                                                   const JsonObject& block,
                                                   const std::string& methodName,
                                                   const HouseModes& houseModes ) const;

    HouseModes getControllerHouseModes();
    void addActionId( Json& thenBlock ) const;

    void validateField( const std::string& method,
                        const std::string& field,
                        const Json& value,
                        const HouseModes& houseModes ) const;
    static void validateSaveResult( const JsonObject& saveResultObj );
    bool isActionOfSynchronousType( const std::string& actionName );
    void fetchGlobalRestrictionID( Json& sceneJsonObject );
    struct CheckActionFields {
        static void runPluginScript( const std::string& field_name, const Json& value );
        static void resetLatch( const std::string& field_name, const Json& value );
        static void resetSceneLatches( const std::string& field_name, const Json& value );
        static void cloudAPI( const std::string& field_name, const Json& value );
        static void setExpression( const std::string& field_name, const Json& value );
        static void setVariable( const std::string& field_name, const Json& value );
        static void toggleValue( const std::string& field_name,
                                 const Json& value,
                                 CoreDRemoteInterface::CoreDInterface&,
                                 MakeExpressionValueProvider expressionValueProvider );
        static void hubBackupRun( const std::string& field_name, const Json& value );
        static void forwardNotifications( const std::string& field_name, const Json& value );
    };

private:
    ControllerId m_ownControllerId;
    CoreDRemoteInterface::CoreDInterface& m_core;
    ExtensionsManager::RemoteInterface& m_extensionManager;
    MakeExpressionValueProvider m_expressionValueProvider;

    IdUtils::PlaceholdersMapping* m_placeholdersMapping = nullptr;
    std::string m_globalRestrictionId;
};

}  // namespace ScenesManager
}  // namespace HubPlatform
