#include "scenes_manager/scenes_engine/actions/run_custom_script_action.h"
#include "scenes_manager/scenes_processor/parsers/action_parser.h"
#include "scenes_manager/scenes_engine/exceptions.h"
#include "scenes_manager/scenes_engine/helpers/json_helper.h"
#include "base_utils/ezlodb.h"
#include "scenes_manager/scenes_processor/resources.h"
#include <utility>

namespace HubPlatform {
namespace ScenesManager {
namespace BlockFields {
constexpr char Exceptions[] = "exceptions";
constexpr char BlockType[] = "blockType";
constexpr char GlobalRestriction[] = "globalRestriction";
constexpr char Id[] = "_id";
}  // namespace BlockFields
namespace ActionFields {
constexpr char Arguments[] = "args";
constexpr char BlockOptions[] = "blockOptions";
constexpr char SaveResult[] = "saveResult";
constexpr char Method[] = "method";
constexpr char Name[] = "name";
constexpr char ControllerId[] = "controllerId";
constexpr char ExecutionPolicy[] = "exec_policy";
constexpr char Delay[] = "delay";
constexpr char Control[] = "control";
constexpr char Exception[] = "exception";
constexpr char Id[] = "_id";

namespace Fields {
constexpr char Item[] = "item";
constexpr char Value[] = "value";
constexpr char HouseMode[] = "houseMode";
constexpr char Script[] = "script";
constexpr char Params[] = "params";
constexpr char Enabled[] = "enabled";
}  // namespace Fields
}  // namespace ActionFields

static const std::set< std::string > SupportedResetTypes = { "soft", "factory" };
static const std::set< std::string > SupportedBlockTypes = { ActionBlockTypes::Then,
                                                             ActionBlockTypes::Else,
                                                             ActionBlockTypes::ThenGroups,
                                                             ActionBlockTypes::ElseGroups };

using SceneEngine::ActionBlocks;
using SceneEngine::ExecutionPolicy;

ActionParserInterface::~ActionParserInterface() = default;

ActionParser::ActionParser( ControllerId controllerId,
                            CoreDRemoteInterface::CoreDInterface& core,
                            ExtensionsManager::RemoteInterface& extensionManager,
                            MakeExpressionValueProvider makeExpressionValueProvider )
: m_ownControllerId( std::move( controllerId ) )
, m_core( core )
, m_extensionManager( extensionManager )
, m_expressionValueProvider( std::move( makeExpressionValueProvider ) ) {}

SceneEngine::ActionBlocks::Actions ActionParser::parse( Json& sceneJsonObject,
                                                        BlockType blockType,
                                                        IdUtils::PlaceholdersMapping& placeholdersMapping ) {
    const auto sceneId = json_get_str( sceneJsonObject, ActionFields::Id );
    const auto scenePolicy = getExecutionPolicy( sceneJsonObject, ExecutionPolicy::IgnoreResult );
    const auto blockName = blockType == BlockType::Then ? ActionBlockTypes::Then : ActionBlockTypes::Else;
    fetchGlobalRestrictionID( sceneJsonObject );

    m_placeholdersMapping = &placeholdersMapping;
    auto result =
        parseArrayOfActions( sceneId, scenePolicy, getBlock( sceneJsonObject, blockName, Json::value_t::array ) );
    m_placeholdersMapping = nullptr;

    return result;
}

SceneEngine::ActionBlocks::ActionGroups ActionParser::parseGroups( Json& sceneJsonObject,
                                                                   BlockType blockType,
                                                                   IdUtils::PlaceholdersMapping& placeholdersMapping ) {
    const auto sceneId = json_get_str( sceneJsonObject, ActionFields::Id );
    const auto scenePolicy = getExecutionPolicy( sceneJsonObject, ExecutionPolicy::IgnoreResult );
    const auto blockName = blockType == BlockType::Then ? ActionBlockTypes::ThenGroups : ActionBlockTypes::ElseGroups;
    fetchGlobalRestrictionID( sceneJsonObject );
    m_placeholdersMapping = &placeholdersMapping;
    auto& block = getBlock( sceneJsonObject, blockName, Json::value_t::object );

    ActionBlocks::ActionGroups groups;
    for( auto it = block.begin(); it != block.end(); ++it ) {
        groups[ it.key() ] = parseArrayOfActions( sceneId, scenePolicy, *it );
    }
    m_placeholdersMapping = nullptr;

    return groups;
}

SceneEngine::ActionBlocks::Actions ActionParser::parseArrayOfActions( const std::string& sceneId,
                                                                      ExecutionPolicy sceneExecutionPolicy,
                                                                      Json& blockArray ) {
    using namespace SceneEngine;

    using ItemIdList = std::set< ItemId >;
    struct MulticastData {
        SceneEngine::ActionSharedPtr action;
        ItemIdList itemIds;
        Action::Id firstElementId;
    };
    using MulticastActions = std::map< ItemValue, MulticastData >;
    using DelayGroupedActions = std::map< Action::Delay, ActionBlocks::Actions >;

    Action::Delay summaryDelay( 0 );
    DelayGroupedActions delayGroupedActions;

    auto controllerHouseModes = getControllerHouseModes();

    int id = 0;
    for( auto it = blockArray.begin(); it != blockArray.end(); ++it, ++id ) {
        JsonObject& block = *it;

        JsonObject block_options;
        if( !json_get_obj( block, ActionFields::BlockOptions, block_options ) ) {
            // TODO: make specific object with description error. For that need new type of
            // error description. Invalid param (what ... )
            throw SceneEngine::SceneException( Errors::WRONG_FIELD );
        }

        JsonObject method_object;
        if( !json_get_obj( block_options, ActionFields::Method, method_object ) ) {
            // TODO: make specific object with description error. For that need new type of
            // error description. Invalid param (what ... )
            throw SceneEngine::SceneException( Errors::WRONG_FIELD );
        }

        std::string method_name;
        json_get( method_object, ActionFields::Name, method_name );

        JsonObject args;
        if( !json_get_obj( method_object, ActionFields::Arguments, args ) ) {
            // TODO: make specific object with description error. For that need new type of
            // error description. Invalid param (what ... )
            throw SceneEngine::SceneException( Errors::WRONG_FIELD );
        }

        ExecutionPolicy actionExecutionPolicy = getExecutionPolicy( block, sceneExecutionPolicy );

        auto controllerId = json_get_str( block, ActionFields::ControllerId );
        if( controllerId.empty() ) {
            controllerId = m_ownControllerId;
        }

        addActionId( block );

        auto delay = parseDelay( block );
        summaryDelay += delay;

        Action::InitData actionInitData;
        actionInitData.name = method_name;
        actionInitData.id = id;
        actionInitData.arguments = parseArguments( args, block, method_name, controllerHouseModes );
        actionInitData.executionPolicy = actionExecutionPolicy;
        actionInitData.controllerId = controllerId;
        actionInitData.delay = delay;

        if( actionInitData.executionPolicy != sceneExecutionPolicy ) {
            sceneExecutionPolicy = actionInitData.executionPolicy;
        }
        parseControlFlow( block, sceneId, actionInitData );

        ActionSharedPtr newAction;
        if( method_name == ActionFields::ActionNames::RunCustomScript )
            newAction = std::make_shared< RunCustomScriptAction >( m_extensionManager, std::move( actionInitData ) );
        else if( method_name == ActionFields::ActionNames::RunPluginScript )
            newAction = std::make_shared< RunPluginScriptAction >( m_extensionManager, std::move( actionInitData ) );
        else
            newAction = std::make_shared< Action >( std::move( actionInitData ) );

        if( method_name == ActionFields::ActionNames::SetItemValue ) {
            newAction->setSubscriptionState( SubscriptionState::Item );
            newAction->setInitialized( false );
        }
        else if( method_name == ActionFields::ActionNames::SetDeviceArmed ) {
            newAction->setSubscriptionState( SubscriptionState::Device );
            newAction->setInitialized( false );
        }
        delayGroupedActions[ summaryDelay ].emplace_back( std::move( newAction ) );
    }
    int anotherActionPresent = 0;
    auto addMulticastAction = [&anotherActionPresent]( MulticastActions& multicastActions,
                                                       const ActionSharedPtr& action ) {
        bool isFirstItem = false;
        const auto& arguments = action->getArgs();
        auto itValue = arguments.find( ActionFields::Fields::Value );
        auto itItem = arguments.find( ActionFields::Fields::Item );
        auto itScale = arguments.find( "scale" );
        if( itValue != arguments.end() && itItem != arguments.end() && itScale == arguments.end() ) {
            JsonObject key;
            if( action->getExecutionPolicy() != ExecutionPolicy::IgnoreResult ) {
                key[ itValue->dump() ] = anotherActionPresent;
            }
            else {
                key[ itValue->dump() ];
            }
            MulticastData& multicastData = multicastActions[ key ];
            if( !multicastData.action ) {
                multicastData.action = action;
                multicastData.firstElementId = action->getId();
                isFirstItem = true;
            }
            std::string itemId = json_get_str( action->getArgs(), ActionFields::Fields::Item );
            if( itemId.empty() ) {
                throw SceneEngine::SceneException( Errors::WRONG_FIELD, "setItemValueAction: itemId is empty" );
            }
            multicastData.itemIds.emplace( std::move( itemId ) );
        }
        else if( itValue != arguments.end() && itItem != arguments.end() && itScale != arguments.end() ) {
            JsonObject key, valuePair;
            if( action->getExecutionPolicy() != ExecutionPolicy::IgnoreResult ) {
                valuePair[ itScale->dump() ] = anotherActionPresent;
                key[ itValue->dump() ] = valuePair;
            }
            else {
                key[ itValue->dump() ] = *itScale;
            }

            MulticastData& multicastData = multicastActions[ key ];
            if( !multicastData.action ) {
                multicastData.action = action;
                multicastData.firstElementId = action->getId();
                isFirstItem = true;
            }
            std::string itemId = json_get_str( action->getArgs(), ActionFields::Fields::Item );
            if( itemId.empty() ) {
                throw SceneEngine::SceneException( Errors::WRONG_FIELD, "setItemValueAction: itemId is empty" );
            }
            multicastData.itemIds.emplace( std::move( itemId ) );
        }
        return isFirstItem;
    };

    SceneEngine::ActionBlocks::Actions actionResults;
    for( auto& element : delayGroupedActions ) {
        MulticastActions multicastActions;
        auto& actions = element.second;
        if( actions.size() == 1 ) {
            actionResults.emplace_back( std::move( actions[ 0 ] ) );
        }
        else if( actions.size() > 1 ) {
            ActionBlocks::Actions tmpActions;
            for( const auto& action : actions ) {
                if( action->getName() == ActionFields::ActionNames::SetItemValue ) {
                    if( addMulticastAction( multicastActions, action ) ) {
                        tmpActions.emplace_back( std::move( action ) );
                    }
                }
                else {
                    auto actionName = action->getName();
                    if( isActionOfSynchronousType( actionName ) ) {
                        if( action->getExecutionPolicy() != ExecutionPolicy::IgnoreResult ) {
                            anotherActionPresent++;
                        }
                    }
                    tmpActions.emplace_back( std::move( action ) );
                }
            }
            for( auto& element : multicastActions ) {
                MulticastData& multicastData = element.second;
                if( multicastData.itemIds.size() > 1 ) {
                    Action::Arguments arguments = multicastData.action->getArgs();
                    arguments[ ActionFields::Fields::Item ] = multicastData.itemIds;
                    multicastData.action->setArgs( std::move( arguments ) );
                }
                for( auto& action : tmpActions ) {
                    if( action->getId() == multicastData.firstElementId ) {
                        action = std::move( multicastData.action );
                        break;
                    }
                }
            }
            auto size = actionResults.size();
            actionResults.resize( actionResults.size() + tmpActions.size() );
            std::move( tmpActions.begin(), tmpActions.end(), actionResults.begin() + size );
        }
    }
    return actionResults;
}

JsonArray& ActionParser::getBlock( JsonObject& sceneJsonObject,
                                   const std::string& blockName,
                                   Json::value_t elementType ) {
    auto itBlock = sceneJsonObject.find( blockName );
    if( itBlock != sceneJsonObject.end() && itBlock->type() == elementType ) {
        return *itBlock;
    }
    else {
        throw SceneEngine::SceneException( getWrongBlockError( blockName ) );
    }
}

Errors::Error ActionParser::getWrongBlockError( const std::string& blockName ) {
    static std::unordered_map< std::string, Errors::Error > errors = {
        { ActionBlockTypes::Then, Errors::SCENE_WRONG_THEN_BLOCK },
        { ActionBlockTypes::Else, Errors::SCENE_WRONG_ELSE_BLOCK },
        { ActionBlockTypes::ThenGroups, Errors::SCENE_WRONG_THEN_GROUPS_BLOCK },
        { ActionBlockTypes::ElseGroups, Errors::SCENE_WRONG_ELSE_GROUPS_BLOCK },
    };
    auto iter = errors.find( blockName );
    return iter != errors.end() ? iter->second : Errors::SCENE_UNSUPPORTED_ACTION_BLOCK;
}

ExecutionPolicy ActionParser::getExecutionPolicy( const Json& sceneJsonObject, ExecutionPolicy defaultValue ) {
    using namespace SceneEngine;
    std::string executionPolicyStr = json_get_str( sceneJsonObject, ActionFields::ExecutionPolicy );
    if( executionPolicyStr == "check_result" ) {
        return ExecutionPolicy::CheckResult;
    }
    else if( executionPolicyStr == "ignore_result" ) {
        return ExecutionPolicy::IgnoreResult;
    }
    else if( executionPolicyStr == "wait_result" ) {
        return ExecutionPolicy::WaitResult;
    }
    else {
        return defaultValue;
    }
}

std::chrono::seconds ActionParser::parseDelay( const JsonObject& then_block ) {
    JsonObject delayJson;
    int delay = 0;
    if( json_get_obj( then_block, ActionFields::Delay, delayJson ) ) {
        delay += json_get_int( delayJson, "seconds", 0 );
        delay += json_get_int( delayJson, "minutes", 0 ) * 60;
        delay += json_get_int( delayJson, "hours", 0 ) * 3600;
        delay += json_get_int( delayJson, "days", 0 ) * 24 * 3600;
    }
    return std::chrono::seconds( delay );
}

void ActionParser::parseControlFlow( JsonObject& block,
                                     const std::string& sceneId,
                                     SceneEngine::Action::InitData& actionInitData ) {
    using namespace SceneEngine;

    auto controlIt = block.find( ActionFields::Control );
    if( controlIt != block.end() ) {
        if( !controlIt->is_object() )
            throw SceneException( Errors::WRONG_FIELD, ActionFields::Control, "Should be an object" );

        Json& controlObj = *controlIt;
        auto exceptionIt = controlObj.find( ActionFields::Exception );
        if( exceptionIt != controlObj.end() ) {
            if( !exceptionIt->is_object() )
                throw SceneException( Errors::WRONG_FIELD, ActionFields::Exception, "Should be an object" );

            Json& exceptionObj = *exceptionIt;
            actionInitData.exceptionData = parseExceptionData( exceptionObj, sceneId );
        }
    }
}

SceneEngine::ActionExceptionData ActionParser::parseExceptionData( JsonObject& exceptionDataObj,
                                                                   const std::string& sceneId ) {
    using namespace SceneEngine;
    using namespace ActionFields;

    std::string type = json_get_str( exceptionDataObj, Fields::Type );
    if( type.empty() )
        throw SceneException( Errors::WRONG_FIELD, Fields::Type, "Should be non-empty string" );

    if( type == Values::BlockReference ) {
        auto blockIt = exceptionDataObj.find( Fields::BlockId );
        if( blockIt == exceptionDataObj.end() || !blockIt->is_string() )
            throw SceneException( Errors::WRONG_FIELD, Fields::BlockId );
        if( !m_globalRestrictionId.empty() ) {
            const auto id = blockIt->get< std::string >();
            if( id == m_globalRestrictionId ) {
                throw SceneException( Errors::WRONG_FIELD,
                                      Fields::BlockId,
                                      "Should not be allowed to reference globalRestrictions block in any action" );
            }
        }

        if( m_placeholdersMapping && IdUtils::isPlaceholder( *blockIt ) )
            *blockIt = IdUtils::replacePlaceholder( *blockIt, *m_placeholdersMapping );

        std::string referenceSceneId = json_get_str( exceptionDataObj, Fields::SceneId );
        if( referenceSceneId.empty() )
            referenceSceneId = sceneId;

        ActionExceptionData::BlockReference result{ std::move( referenceSceneId ), *blockIt };
        return std::move( result );
    }
    else if( type == Values::ExpressionReference ) {
        auto nameIt = exceptionDataObj.find( Fields::Name );
        if( nameIt == exceptionDataObj.end() || !nameIt->is_string() )
            throw SceneException( Errors::WRONG_FIELD, Fields::Name );

        ActionExceptionData::ExpressionReference result{ *nameIt };
        return std::move( result );
    }

    throw SceneException( Errors::WRONG_FIELD, Fields::Type, "Unknown value action exception type" );
}

SceneEngine::Action::Arguments ActionParser::parseArguments( const JsonObject& args,
                                                             const JsonObject& block,
                                                             const std::string& methodName,
                                                             const HouseModes& houseModes ) const {
    SceneEngine::Action::Arguments result = JSON_EMPTY_OBJ;

    JsonObject saveResultObj;
    if( json_get_obj( block, ActionFields::SaveResult, saveResultObj ) ) {
        validateSaveResult( saveResultObj );
        result[ ActionFields::SaveResult ] = std::move( saveResultObj );
    }

    for( const auto& argument : args ) {
        const std::string fieldName = json_get_str( argument );
        if( fieldName.empty() )
            throw SceneEngine::SceneException( Errors::WRONG_FIELD );

        JsonObject value;
        json_helper::get_field_from_array( fieldName, block, value );
        result[ fieldName ] = value;

        JsonObject scale;
        if( "value" == fieldName && json_helper::get_field_from_array( fieldName, block, scale, "scale" ) ) {
            result[ "scale" ] = scale;  // Append artificial field "scale"
        }

        validateField( methodName, fieldName, value, houseModes );
    }

    return result;
}

ActionParser::HouseModes ActionParser::getControllerHouseModes() {
    HouseModes houseModesIds;
    auto houseModes = m_core.getHouseModes();
    for( const auto& houseMode : houseModes ) {
        houseModesIds.insert( houseMode.m_id );
    }
    return houseModesIds;
}

void ActionParser::addActionId( Json& thenBlock ) const {
    auto it = thenBlock.find( ActionFields::Id );
    if( it == thenBlock.end() ) {
        thenBlock[ ActionFields::Id ] = Db::genOid();
    }
}

void ActionParser::validateSaveResult( const JsonObject& saveResultObj ) {
    JsonObject saveResultMethodObject;
    if( !json_get( saveResultObj, ActionFields::Method, saveResultMethodObject ) )
        throw SceneEngine::SceneException( Errors::WRONG_FIELD );

    JsonObject saveResultArgs;
    if( !json_get_obj( saveResultObj, ActionFields::Arguments, saveResultArgs ) )
        throw SceneEngine::SceneException( Errors::WRONG_FIELD );

    std::string saveResultMethodName;
    if( !json_get( saveResultArgs, ActionFields::Name, saveResultMethodName ) )
        throw SceneEngine::SceneException( Errors::WRONG_FIELD );
}

void ActionParser::validateField( const std::string& method,
                                  const std::string& field,
                                  const Json& value,
                                  const HouseModes& houseModes ) const {
    if( method == ActionFields::ActionNames::SwitchHouseMode && field == ActionFields::Fields::HouseMode ) {
        auto houseMode = json_get_str( value );
        if( houseMode.empty() || houseModes.find( houseMode ) == houseModes.end() )
            throw SceneEngine::SceneException( Errors::HOUSE_MODE_DOES_NOT_EXIST );
    }
    else if( method == ActionFields::ActionNames::RunCustomScript && field == ActionFields::Fields::Script ) {
        if( !value.is_string() )
            throw SceneEngine::SceneException( Errors::WRONG_FIELD );
    }
    else if( method == ActionFields::ActionNames::RunScene && field == ActionFields::Fields::Block ) {
        if( !SupportedBlockTypes.count( value ) )
            throw SceneEngine::SceneException(
                Errors::WRONG_FIELD, ActionFields::Fields::Block, "Unsupported block type" );
    }
    else if( method == ActionFields::ActionNames::StopScene && field == ActionFields::Fields::Block ) {
        if( !SupportedBlockTypes.count( value ) )
            throw SceneEngine::SceneException(
                Errors::WRONG_FIELD, ActionFields::Fields::Block, "Unsupported block type" );
    }
    else if( method == ActionFields::ActionNames::SetSceneState && field == ActionFields::Fields::Enabled ) {
        if( !value.is_boolean() )
            throw SceneEngine::SceneException( Errors::WRONG_FIELD );
    }
    else if( method == ActionFields::ActionNames::ResetHub && field == ActionFields::Fields::ResetType ) {
        if( !SupportedResetTypes.count( value ) )
            throw SceneEngine::SceneException( Errors::WRONG_FIELD );
    }
    else if( method == ActionFields::ActionNames::RunPluginScript ) {
        CheckActionFields::runPluginScript( field, value );
    }
    else if( method == ActionFields::ActionNames::ResetLatch ) {
        CheckActionFields::resetLatch( field, value );
    }
    else if( method == ActionFields::ActionNames::ResetSceneLatches ) {
        CheckActionFields::resetSceneLatches( field, value );
    }
    else if( method == ActionFields::ActionNames::CloudAPI ) {
        CheckActionFields::cloudAPI( field, value );
    }
    else if( method == ActionFields::ActionNames::SetExpression ) {
        CheckActionFields::setExpression( field, value );
    }
    else if( method == ActionFields::ActionNames::SetVariable ) {
        if( field == "name" ) {
            auto provider = m_expressionValueProvider( value );
            if( provider && provider->fetchItemValue() ) {
                CheckActionFields::setVariable( field, value );
            }
            else {
                LOGGER_ERROR( cc::error( "Expression does not exist: " ) + cc::j( value ) );
                throw SceneEngine::SceneException( Errors::SCENES_EXPRESSION_DOES_NOT_EXIST,
                                                   "Expression does not exist" );
            }
        }
    }
    else if( method == ActionFields::ActionNames::ToggleValue ) {
        CheckActionFields::toggleValue( field, value, m_core, m_expressionValueProvider );
    }
    else if( method == ActionFields::ActionNames::HubBackupAction ) {
        CheckActionFields::hubBackupRun( field, value );
    }
    else if( method == ActionFields::ActionNames::ForwardNotifications ) {
        CheckActionFields::forwardNotifications( field, value );
    }
}

void ActionParser::CheckActionFields::toggleValue( const std::string& field,
                                                   const Json& value,
                                                   CoreDRemoteInterface::CoreDInterface& core,
                                                   MakeExpressionValueProvider provider ) {
    if( !value.is_string() ) {
        throw SceneEngine::SceneException( Errors::WRONG_FIELD, field );
    }

    if( field == ActionFields::ExpressionFields::Item ) {
        auto item = core.getItem( value );
        if( item.empty() )
            throw SceneEngine::SceneException( Errors::SCENES_ITEM_DOES_NOT_EXIST, "Scenes item doesn't exist" );

        auto valueType = json_get_str( item, ActionFields::ExpressionFields::Type );

        if( !( valueType == "int" || valueType == "bool" ) ) {
            throw SceneEngine::SceneException( Errors::SCENE_ACTION_TYPE_MISMATCH,
                                               "Scenes action type mismatch: int or bool expected" );
        }
    }
    else if( field == ActionFields::ExpressionFields::Expression ) {
        auto aProvider = provider( value );
        if( !( aProvider && aProvider->fetchItemValue() ) ) {
            throw SceneEngine::SceneException( Errors::SCENES_EXPRESSION_DOES_NOT_EXIST, "Expression does not exist" );
        }

        auto valueType = aProvider->getCachedItemValue().getValueType();
        if( !( valueType == "int" || valueType == "bool" ) ) {
            throw SceneEngine::SceneException( Errors::SCENE_ACTION_TYPE_MISMATCH,
                                               "Scenes action type mismatch: int or bool expected" );
        }
    }
    else {
        throw SceneEngine::SceneException( Errors::WRONG_FIELD, field );
    }
}

void ActionParser::CheckActionFields::runPluginScript( const std::string& field_name, const Json& value ) {
    if( field_name == ActionFields::Fields::Script ) {
        if( !value.is_string() ) {
            throw SceneEngine::SceneException( Errors::WRONG_FIELD, field_name );
        }
    }
    else if( field_name == ActionFields::Fields::Params ) {
        if( !value.empty() && !value.is_object() ) {
            throw SceneEngine::SceneException( Errors::WRONG_FIELD, field_name );
        }
    }
}

void ActionParser::CheckActionFields::resetLatch( const std::string& field_name, const Json& value ) {
    const std::string valueStr = json_get_str( value );

    std::string sceneIdStr = ActionFields::Fields::SceneId;
    if( field_name == sceneIdStr && valueStr.empty() ) {
        std::string error = sceneIdStr + " should be a non-empty string";
        throw SceneEngine::SceneException( Errors::WRONG_FIELD, std::move( sceneIdStr ), error.c_str() );
    }

    std::string blockIdStr = ActionFields::Fields::BlockId;
    if( field_name == blockIdStr && valueStr.empty() ) {
        std::string error = blockIdStr + " should be a non-empty string";
        throw SceneEngine::SceneException( Errors::WRONG_FIELD, std::move( blockIdStr ), error.c_str() );
    }
}

void ActionParser::CheckActionFields::resetSceneLatches( const std::string& field_name, const Json& value ) {
    const std::string valueStr = json_get_str( value );

    std::string sceneIdStr = ActionFields::Fields::SceneId;
    if( field_name == sceneIdStr && valueStr.empty() ) {
        std::string error = sceneIdStr + " should be a non-empty string";
        throw SceneEngine::SceneException( Errors::WRONG_FIELD, std::move( sceneIdStr ), error.c_str() );
    }
}

void ActionParser::CheckActionFields::cloudAPI( const std::string& field_name, const Json& value ) {
    if( field_name == "cloudApiName" ) {
        if( !value.is_string() ) {
            throw SceneEngine::SceneException( Errors::WRONG_FIELD, "cloudApiName", "" );
        }
    }
    else if( field_name == "version" ) {
        if( !value.is_number_integer() ) {
            throw SceneEngine::SceneException( Errors::WRONG_FIELD,
                                               "version, "
                                               "" );
        }
    }
    else if( field_name == "params" ) {
        if( !value.is_string() ) {
            throw SceneEngine::SceneException( Errors::WRONG_FIELD, "params", "" );
        }
        else {
            try {
                Json::parse( value );
            }
            catch( const std::exception& ex ) {
                throw SceneEngine::SceneException( Errors::WRONG_FIELD, "params", ex.what() );
            }
        }
    }
}

void ActionParser::CheckActionFields::setExpression( const std::string& field_name, const Json& value ) {
    auto validateStringField = []( const JsonObject& value, const char* field ) {
        if( !value.is_string() )
            throw SceneEngine::SceneException( Errors::WRONG_FIELD, field, "Unsupported block type" );
    };

    if( field_name == ActionFields::ExpressionFields::Name ) {
        validateStringField( value, ActionFields::ExpressionFields::Name );
    }
    else if( field_name == ActionFields::ExpressionFields::Type ) {
        validateStringField( value, ActionFields::ExpressionFields::Type );
    }
    else if( field_name == ActionFields::ExpressionFields::Code ) {
        validateStringField( value, ActionFields::ExpressionFields::Code );
    }
    else if( field_name == ActionFields::ExpressionFields::ValueType ) {
        validateStringField( value, ActionFields::ExpressionFields::ValueType );
    }
    else if( field_name == ActionFields::ExpressionFields::Metadata ) {
        if( !value.is_object() ) {
            throw SceneEngine::SceneException(
                Errors::WRONG_FIELD, ActionFields::ExpressionFields::Metadata, "Unsupported block type" );
        }
    }
    else if( field_name == ActionFields::ExpressionFields::Error ) {
        validateStringField( value, ActionFields::ExpressionFields::Error );
    }
    else if( field_name == ActionFields::ExpressionFields::Params ) {
        if( !value.is_object() ) {
            throw SceneEngine::SceneException(
                Errors::WRONG_FIELD, ActionFields::ExpressionFields::Metadata, "Unsupported block type" );
        }
    }
}

void ActionParser::CheckActionFields::setVariable( const std::string& field_name, const Json& value ) {
    auto validateStringField = []( const JsonObject& value, const char* field ) {
        if( !value.is_string() )
            throw SceneEngine::SceneException( Errors::WRONG_FIELD, field, "Unsupported block type" );
    };

    if( field_name == ActionFields::ExpressionFields::Name ) {
        validateStringField( value, ActionFields::ExpressionFields::Name );
    }
    else if( field_name == ActionFields::ExpressionFields::ValueType ) {
        validateStringField( value, ActionFields::ExpressionFields::ValueType );
    }
}

void ActionParser::CheckActionFields::forwardNotifications( const std::string& field_name, const Json& value ) {
    auto validateStringField = []( const JsonObject& value, const char* field ) {
        if( !value.is_string() || value.empty() )
            throw SceneEngine::SceneException( Errors::WRONG_FIELD, field, "Unsupported block type" );
    };
    auto validateJsonField = []( const JsonObject& value, const char* field ) {
        if( !value.is_object() || value.empty() )
            throw SceneEngine::SceneException( Errors::WRONG_FIELD, field, "Unsupported block type" );
    };
    auto validateArrayField = []( const JsonObject& value, const char* field ) {
        if( !value.is_array() || value.empty() )
            throw SceneEngine::SceneException( Errors::WRONG_FIELD, field, "Unsupported block type" );
    };

    if( field_name == ActionFields::ForwardNotificationFields::NotificationTemplate ) {
        validateStringField( value, ActionFields::ForwardNotificationFields::NotificationTemplate );
    }
    else if( field_name == ActionFields::ForwardNotificationFields::Targets ) {
        validateArrayField( value, ActionFields::ForwardNotificationFields::Targets );
    }
    else if( field_name == ActionFields::ForwardNotificationFields::Device ) {
        validateStringField( value, ActionFields::ForwardNotificationFields::Device );
    }
    else if( field_name == ActionFields::ForwardNotificationFields::Item ) {
        validateStringField( value, ActionFields::ForwardNotificationFields::Item );
    }
    else if( field_name == ActionFields::ForwardNotificationFields::NotificationBlock ) {
        validateJsonField( value, ActionFields::ForwardNotificationFields::NotificationBlock );
    }
}

void ActionParser::CheckActionFields::hubBackupRun( const std::string& field_name, const Json& value ) {
    auto validateStringField = []( const JsonObject& value, const char* field ) {
        if( !value.is_string() || value.empty() )
            throw SceneEngine::SceneException( Errors::WRONG_FIELD, field, "Unsupported block type" );
    };
    auto validateJsonField = []( const JsonObject& value, const char* field ) {
        if( !value.is_object() || value.empty() )
            throw SceneEngine::SceneException( Errors::WRONG_FIELD, field, "Unsupported block type" );
    };
    auto validateKeyField = []( const JsonObject& value, const char* field ) {
        if( value.empty() )
            throw SceneEngine::SceneException( Errors::EMPTY_FIELD, field, "value should not be empty" );
    };

    if( field_name == ActionFields::HubBackupFields::Action ) {
        validateStringField( value, ActionFields::HubBackupFields::Action );
    }
    else if( field_name == ActionFields::HubBackupFields::BackupId ) {
        validateJsonField( value, ActionFields::HubBackupFields::BackupId );
        validateKeyField( json_get_str( value, ActionFields::HubBackupFields::Key ),
                          ActionFields::HubBackupFields::Key );
        validateKeyField( json_get_str( value, ActionFields::HubBackupFields::Uuid ),
                          ActionFields::HubBackupFields::Uuid );
    }
}

void ActionParser::fetchGlobalRestrictionID( Json& sceneJsonObject ) {
    Json::iterator itExceptions = sceneJsonObject.find( BlockFields::Exceptions );
    if( itExceptions != sceneJsonObject.end() ) {
        for( Json& exceptionBlock : *itExceptions ) {
            if( json_get_str( exceptionBlock, BlockFields::BlockType ) == BlockFields::GlobalRestriction ) {
                m_globalRestrictionId = json_get_str( exceptionBlock, BlockFields::Id );
                break;
            }
        }
    }
}

bool ActionParser::isActionOfSynchronousType( const std::string& actionName ) {
    if( actionName == ActionFields::ActionNames::SendCloudAbstractCommand ||
        actionName == ActionFields::ActionNames::SendHttpRequest ||
        actionName == ActionFields::ActionNames::RunPluginScript ||
        actionName == ActionFields::ActionNames::SetVariable ||
        actionName == ActionFields::ActionNames::SetExpression ) {
        return true;
    }
    return false;
}

}  // namespace ScenesManager
}  // namespace HubPlatform
