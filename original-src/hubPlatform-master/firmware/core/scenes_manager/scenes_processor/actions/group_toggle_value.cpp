#include "extensions_manager/remote_interface/em_remote_interface.h"
#include "scenes_manager/scenes_engine/exceptions.h"
#include "common_utils/cpp14.h"
#include "scenes_manager/scenes_processor/actions/group_toggle_value.h"
#include "cored/group_operations/group_db_reader.h"
#include "cored/model/resources/cored_resources.h"

namespace HubPlatform {
namespace ScenesManager {

GroupToggleValueAction::GroupToggleValueAction( ControllerFactory& controllerFactory, Services::Service& hostService )
: ControllerCoreActions( controllerFactory )
, m_hostService( hostService ) {}

class NextStepFactory final : public std::enable_shared_from_this< NextStepFactory > {
public:
    using SendFinishCallbackPtr = std::shared_ptr< ActionInterface::SendFinishCallback >;
    using ControllerInterfacePtr = std::shared_ptr< SceneEngine::ControllerInterface >;
    using Args = std::vector< JsonObject >;

public:
    NextStepFactory( SendFinishCallbackPtr sendFinishCallbackPtr, ControllerInterfacePtr controllerInterface )
    : m_sendFinishCallbackPtr( std::move( sendFinishCallbackPtr ) )
    , m_controllerInterface( std::move( controllerInterface ) ) {}

public:
    using SetItemValueFinished = std::function< void( const Json& ) >;
    SetItemValueFinished makeNextStepCallback( const Args& args, int index ) {
        auto factory = shared_from_this();

        return [factory, args, index]( const Json& response ) mutable -> void {
            if( index == args.size() - 1 ) {
                auto sendFinishCallbackPtr = factory->m_sendFinishCallbackPtr;
                if( sendFinishCallbackPtr != nullptr ) {
                    sendFinishCallbackPtr->setResult( true );
                    sendFinishCallbackPtr.reset();
                }
                return;
            }
            else {
                auto nextStep = index + 1;
                const auto& commandParams = args[ nextStep ];
                auto nextStepCallback = factory->makeNextStepCallback( args, nextStep );
                auto controllerInterface = factory->m_controllerInterface;

                controllerInterface->setItemValue( commandParams, std::move( nextStepCallback ) );
            }
        };
    }

private:
    SendFinishCallbackPtr m_sendFinishCallbackPtr;
    ControllerInterfacePtr m_controllerInterface;
};

void GroupToggleValueAction::run( const Action& action, ActionExecutorPtr actionExecutorPtr ) {
    auto sendFinishCallbackPtr = std::make_shared< SendFinishCallback >( action, actionExecutorPtr );

    if( !action.isInitialized() ) {
        LOGGER_TRACE( cc::trace( "action is not initialized" ) );
        return;
    }
    auto arguments = action.getArgs();
    auto deviceGroupId = arguments[ "deviceGroup" ];

    if( deviceGroupId.empty() ) {
        LOGGER_ERROR( cc::error( "invalid device group Id" ) );
        sendFinishCallbackPtr->setResult( false );
        sendFinishCallbackPtr.reset();
        return;
    }
    auto itemGroupId = arguments[ "itemGroup" ];
    if( itemGroupId.empty() ) {
        LOGGER_ERROR( cc::error( "invalid item group Id" ) );
        sendFinishCallbackPtr->setResult( false );
        sendFinishCallbackPtr.reset();
        return;
    }

    auto controllerInterface = m_controllerFactory.getControllerInterface( action.getControllerId() );

    using namespace CoreD::Model::Resources;
    using namespace CoreD::GroupOperations;
    JsonObject dbDeviceGroup = DbReader::fetchSingleDeviceGroup( m_hostService.getDBClient(),
                                                                 JSON( DeviceFields::IdFieldName << deviceGroupId ) );

    if( dbDeviceGroup.empty() ) {
        LOGGER_ERROR( cc::error( "device group is not valid" ) );
        sendFinishCallbackPtr->setResult( false );
        sendFinishCallbackPtr.reset();
        return;
    }

    auto deviceIdsArray = GroupSetItemValue::expandDeviceGroup( dbDeviceGroup, action, m_controllerFactory );

    JsonObject dbItemGroup =
        DbReader::fetchSingleItemGroup( m_hostService.getDBClient(), JSON( ItemFields::IdFieldName << itemGroupId ) );

    if( dbItemGroup.empty() ) {
        LOGGER_ERROR( cc::error( "item group is not valid" ) );
        return;
    }
    else if( !( dbItemGroup[ "valueType" ] == "bool" ) ) {
        sendFinishCallbackPtr->setResult( false );
        sendFinishCallbackPtr.reset();
        throw SceneEngine::SceneException( Errors::WRONG_PARAMS, "Not a boolean type item group" );
    }

    auto itemsArray = GroupSetItemValue::expandItemGroup( dbItemGroup, deviceIdsArray, action, m_controllerFactory );

    std::vector< std::string > itemIds0, itemIds1, itemIdsFalse, itemIdsTrue;
    for( auto& singleItem : itemsArray ) {
        JsonObject value;
        json_get( singleItem, CoreD::Model::Resources::ItemFields::ValueFieldName, value );

        auto itemId = json_get_str( singleItem, CoreD::Model::Resources::ItemFields::IdFieldName );

        if( value.is_number_integer() ) {
            auto numberValue = value.get< int64_t >();
            if( numberValue == 0 ) {
                itemIds1.push_back( itemId );
            }
            else {
                itemIds0.push_back( itemId );
            }
        }
        else if( value.is_boolean() ) {
            bool boolValue = value.get< bool >();
            if( boolValue ) {
                itemIdsFalse.push_back( itemId );
            }
            else {
                itemIdsTrue.push_back( itemId );
            }
        }
        else {
            LOGGER_ERROR( cc::error( "Toggle value error: source field is failed" ) );
            sendFinishCallbackPtr->setResult( false );
            sendFinishCallbackPtr.reset();
            return;
        }
    }

    auto composeStepParams =
        []( const std::vector< std::string >& itemIds, Json value, const std::string& sceneId ) -> JsonObject {
        Json commandParams = JSON_EMPTY_OBJ;
        commandParams[ "ids" ] = itemIds;
        commandParams[ "value" ] = std::move( value );

        commandParams[ RPCParams::SourceField ] = { { "type", ItemSource::Scene } };
        if( !sceneId.empty() ) {
            commandParams[ RPCParams::SourceField ][ "id" ] = sceneId;
        }
        return commandParams;
    };

    std::vector< JsonObject > commandParamsArray;
    auto sceneId = actionExecutorPtr->getSceneId();
    if( !itemIds0.empty() ) {
        auto commandParams = composeStepParams( itemIds0, 0, sceneId );
        commandParamsArray.push_back( std::move( commandParams ) );
    }
    if( !itemIds1.empty() ) {
        auto commandParams = composeStepParams( itemIds1, 1, sceneId );
        commandParamsArray.push_back( std::move( commandParams ) );
    }
    if( !itemIdsFalse.empty() ) {
        auto commandParams = composeStepParams( itemIdsFalse, false, sceneId );
        commandParamsArray.push_back( std::move( commandParams ) );
    }
    if( !itemIdsTrue.empty() ) {
        auto commandParams = composeStepParams( itemIdsTrue, true, sceneId );
        commandParamsArray.push_back( std::move( commandParams ) );
    }

    if( commandParamsArray.empty() ) {
        LOGGER_ERROR( cc::error( "No items to toggle" ) );
        sendFinishCallbackPtr->setResult( false );
        sendFinishCallbackPtr.reset();
    }
    else {
        int index = 0;

        auto nextStepFactory = std::make_shared< NextStepFactory >( sendFinishCallbackPtr, controllerInterface );
        auto stepOneCallback = nextStepFactory->makeNextStepCallback( commandParamsArray, index );
        const auto& commandParams = commandParamsArray[ index ];

        controllerInterface->setItemValue( commandParams, std::move( stepOneCallback ) );
    }
}

}  // namespace ScenesManager
}  // namespace HubPlatform