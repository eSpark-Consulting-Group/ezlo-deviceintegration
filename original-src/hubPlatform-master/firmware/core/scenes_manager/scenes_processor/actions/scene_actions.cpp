#include "update_manager/common/um_hub_reset_type.h"
#include "scene_actions.h"
#include "cored/model/resources/cored_resources.h"
#include "scenes_manager/scenes_processor/resources.h"
#include "extensions_manager/remote_interface/em_remote_interface.h"
#include "scenes_manager/scenes_engine/exceptions.h"
#include "http/http_options.h"
#include "http/http_events.h"
#include "json_utils/json_utils.h"
#include "base_utils/ezlodb.h"
#include "jsonrpc/jsonrpc.h"
#include "database_client/dbid.h"
#include "common_utils/cpp14.h"
#include "cored/group_operations/group_db_reader.h"
#include "scenes_manager/scenes_processor/actions/group_toggle_value.h"
#include "scenes_manager/scenes_engine/model/model_resources.h"
#include "base_utils/utils.h"
#include <algorithm>
#include <utility>

namespace ActionResources = HubPlatform::ScenesManager::ActionFields;

namespace HubPlatform {
namespace ScenesManager {

constexpr static size_t ValueMaxLen = 14 * 1024;
constexpr static char SaveResultKey[] = "saveResult";

SetItemValue::SetItemValue( ControllerFactory& controllerFactory )
: ControllerCoreActions( controllerFactory ) {}

void SetItemValue::run( const Action& action, ActionExecutorPtr actionExecutorPtr ) {
    auto sendFinishCallbackPtr = std::make_shared< SendFinishCallback >( action, actionExecutorPtr );

    if( !action.isInitialized() ) {
        LOGGER_TRACE( "action is not initialized" );
        return;
    }

    auto arguments = action.getArgs();
    if( arguments[ "value" ].empty() ) {
        LOGGER_DEBUG( "invalid item value" );
        return;
    }

    auto itItem = arguments.find( "item" );
    if( itItem == arguments.end() || itItem->empty() ) {
        LOGGER_DEBUG( "invalid item id" );
        return;
    }

    auto responseCallback = [sendFinishCallbackPtr]( const JsonObject& message ) mutable {
        LOGGER_TRACE_RAW( "setItemValue response: " + cc::j( message ) );
        sendFinishCallbackPtr->setResult( !hasError( message ) && !message.is_null() );
        sendFinishCallbackPtr.reset();
    };

    auto controllerInterface = m_controllerFactory.getControllerInterface( action.getControllerId() );

    using namespace CoreD::Model::Resources;
    Json commandParams = { { ItemFields::ValueFieldName, arguments[ "value" ] } };
    const std::string scaleKey = "scale";
    std::string scale = json_get_str( arguments, scaleKey );
    if( !scale.empty() )
        commandParams[ scaleKey ] = scale;

    if( itItem->is_array() ) {
        commandParams[ "ids" ] = *itItem;
        // multicast
    }
    else {
        const std::string itemId = itItem->get< std::string >();
        commandParams[ ItemFields::IdFieldName ] = itemId;
    }

    // add source(type=scene, id=<sceneId>)
    commandParams[ RPCParams::SourceField ] = { { "type", ItemSource::Scene } };
    if( actionExecutorPtr.get() && !actionExecutorPtr->getSceneId().empty() ) {
        commandParams[ RPCParams::SourceField ][ "id" ] = actionExecutorPtr->getSceneId();
    }
    controllerInterface->setItemValue( commandParams, std::move( responseCallback ) );
}

GroupSetItemValue::GroupSetItemValue( ControllerFactory& controllerFactory, Services::Service& hostService )
: ControllerCoreActions( controllerFactory )
, m_hostService( hostService ) {}

void GroupSetItemValue::run( const Action& action, ActionExecutorPtr actionExecutorPtr ) {
    auto sendFinishCallbackPtr = std::make_shared< SendFinishCallback >( action, actionExecutorPtr );

    if( !action.isInitialized() ) {
        LOGGER_TRACE( cc::trace( "action is not initialized" ) );
        return;
    }
    auto arguments = action.getArgs();
    auto deviceGroupId = arguments[ "deviceGroup" ];

    if( deviceGroupId.empty() ) {
        LOGGER_ERROR( cc::error( "invalid device group Id" ) );
        return;
    }
    auto itemGroupId = arguments[ "itemGroup" ];
    if( itemGroupId.empty() ) {
        LOGGER_ERROR( cc::error( "invalid item group Id" ) );
        return;
    }

    auto value = arguments[ "value" ];
    auto scale = arguments[ "scale" ];

    auto responseCallback = [sendFinishCallbackPtr]( const JsonObject& message ) mutable {
        LOGGER_TRACE_RAW( cc::trace( "groupSetItemValue response: " ) + cc::j( message ) );
        sendFinishCallbackPtr->setResult( !hasError( message ) && !message.is_null() );
        sendFinishCallbackPtr.reset();
    };

    auto controllerInterface = m_controllerFactory.getControllerInterface( action.getControllerId() );

    using namespace CoreD::Model::Resources;
    Json commandParams = { { ItemFields::ValueFieldName, value } };
    if( !scale.empty() )
        commandParams[ "scale" ] = scale;

    using namespace CoreD::GroupOperations;
    JsonObject dbDeviceGroup = DbReader::fetchSingleDeviceGroup( m_hostService.getDBClient(),
                                                                 JSON( DeviceFields::IdFieldName << deviceGroupId ) );

    if( dbDeviceGroup.empty() ) {
        LOGGER_ERROR( cc::error( "device group is not valid" ) );
        return;
    }

    auto deviceIdsArray = expandDeviceGroup( dbDeviceGroup, action, m_controllerFactory );

    JsonObject dbItemGroup =
        DbReader::fetchSingleItemGroup( m_hostService.getDBClient(), JSON( ItemFields::IdFieldName << itemGroupId ) );

    if( dbItemGroup.empty() ) {
        LOGGER_ERROR( cc::error( "item group is not valid" ) );
        return;
    }

    auto itemsArray = expandItemGroup( dbItemGroup, deviceIdsArray, action, m_controllerFactory );

    if( itemsArray.size() > 1 ) {
        std::vector< std::string > itemIds;
        for( int i = 0; i < itemsArray.size(); i++ ) {
            itemIds.emplace_back( json_get_str( itemsArray.at( i ), ItemFields::IdFieldName ) );
        }
        commandParams[ "ids" ] = itemIds;
    }
    else if( itemsArray.size() == 1 ) {
        auto itemId = json_get_str( itemsArray.at( 0 ), ItemFields::IdFieldName );
        commandParams[ ItemFields::IdFieldName ] = itemId;
    }
    else {
        throw SceneEngine::SceneException( Errors::EMPTY_FIELD, "No items exists for this group" );
    }

    // add source(type=scene, id=<sceneId>)
    commandParams[ RPCParams::SourceField ] = { { "type", ItemSource::Scene } };
    if( actionExecutorPtr.get() && !actionExecutorPtr->getSceneId().empty() ) {
        commandParams[ RPCParams::SourceField ][ "id" ] = actionExecutorPtr->getSceneId();
    }

    controllerInterface->setItemValue( commandParams, std::move( responseCallback ) );
}

DeviceArmed::DeviceArmed( ControllerFactory& controllerFactory )
: ControllerCoreActions( controllerFactory ) {}

void DeviceArmed::run( const Action& action, ActionExecutorPtr actionExecutorPtr ) {
    SendFinishCallback sendFinishCallback( action, actionExecutorPtr );
    if( !action.isInitialized() ) {
        LOGGER_TRACE( "action is not initialized" );
        return;
    }
    std::string deviceId = json_get_str( action.getArgs(), "device" );
    if( deviceId.empty() ) {
        LOGGER_DEBUG( "invalid device Id" );
        return;
    }
    bool deviceFlag;
    if( !json_get( action.getArgs(), "deviceFlag", deviceFlag ) ) {
        LOGGER_DEBUG( "invalid device flag" );
        return;
    }
    auto controllerInterface = m_controllerFactory.getControllerInterface( action.getControllerId() );

    controllerInterface->setDeviceArmed( deviceId, deviceFlag );
    sendFinishCallback.setResult( /*isSuccessful*/ true );
}

GroupDeviceArmed::GroupDeviceArmed( ControllerFactory& controllerFactory, Services::Service& hostService )
: ControllerCoreActions( controllerFactory )
, m_hostService( hostService ) {}

void GroupDeviceArmed::run( const Action& action, ActionExecutorPtr actionExecutorPtr ) {
    SendFinishCallback sendFinishCallback( action, actionExecutorPtr );

    if( !action.isInitialized() ) {
        LOGGER_TRACE( cc::trace( "action is not initialized" ) );
        return;
    }
    std::string deviceGroupId = json_get_str( action.getArgs(), "deviceGroup" );
    if( deviceGroupId.empty() ) {
        LOGGER_ERROR( cc::error( "invalid device group Id" ) );
        return;
    }
    using namespace CoreD::Model::Resources;
    using namespace CoreD::GroupOperations;
    JsonObject dbDeviceGroup = DbReader::fetchSingleDeviceGroup( m_hostService.getDBClient(),
                                                                 JSON( DeviceFields::IdFieldName << deviceGroupId ) );

    if( dbDeviceGroup.empty() ) {
        LOGGER_ERROR( cc::error( "device group is not valid" ) );
        return;
    }

    auto deviceIdsArray = GroupSetItemValue::expandDeviceGroup( dbDeviceGroup, action, m_controllerFactory );

    bool deviceFlag;
    if( !json_get( action.getArgs(), "deviceFlag", deviceFlag ) ) {
        LOGGER_ERROR( cc::error( "invalid device flag" ) );
        return;
    }
    auto controllerInterface = m_controllerFactory.getControllerInterface( action.getControllerId() );

    for( auto& deviceId : deviceIdsArray ) {
        controllerInterface->setDeviceArmed( deviceId, deviceFlag );
    }
    sendFinishCallback.setResult( /*isSuccessful*/ true );
}

HostRequestAction::HostRequestAction( Services::Service& hostService )
: m_hostService( hostService ) {}

void HostRequestAction::run( const ActionInterface::Action& action,
                             ActionInterface::ActionExecutorPtr actionExecutorPtr ) {
    auto sendFinishCallbackPtr = std::make_shared< SendFinishCallback >( action, actionExecutorPtr );
    if( !action.isInitialized() ) {
        LOGGER_ERROR( cc::error( "Action is not initialized" ) );
        return;
    }
    try {
        auto requestData = getRequestData( action.getArgs() );

        LOGGER_TRACE_RAW( cc::trace( "HostRequestAction: requestBody: " ) + cc::j( requestData.m_requestBody ) );
        m_hostService.sendAsyncRequest(
            requestData.m_serviceName,
            requestData.m_requestBody,
            requestData.m_requestType,
            [sendFinishCallbackPtr]( const JsonObject& message ) mutable {
                if( hasError( message ) ) {
                    LOGGER_ERROR_RAW( cc::error( "HostRequestAction failed" ) );
                    LOGGER_DEBUG_RAW( cc::debug( "HostRequestAction: receiveCallback: " ) + cc::j( message ) );
                    sendFinishCallbackPtr->setResult( /*isSuccessful*/ false );
                }
                else {
                    sendFinishCallbackPtr->setResult( /*isSuccessful*/ true );
                }
                sendFinishCallbackPtr.reset();
            },
            [sendFinishCallbackPtr]() mutable {
                LOGGER_TRACE_RAW( cc::trace( "HostRequestAction: timeout" ) );
                sendFinishCallbackPtr->setResult( /*isSuccessful*/ false );
                sendFinishCallbackPtr.reset();
            },
            std::chrono::minutes( 1 ) );
    }
    catch( InvalidActionArgs& e ) {
        LOGGER_ERROR( cc::error( "Send host request error: + " ) + cc::attention( e.what() ) );
    }
}

HostRequestAction::RequestData SendCloudAbstractCommand::getRequestData( const Action::Arguments& arguments ) {
    JsonObject params;
    for( JsonObject::const_iterator it = arguments.begin(); it != arguments.end(); ++it ) {
        if( it.key() == "abstract" ) {
            params[ "uuid" ] = *it;
        }
        else {
            params[ it.key() ] = *it;
        }
    }
    JsonObject message = JSON_EMPTY_OBJ;
    message[ "method" ] = "cloud.controller_abstracts_set";
    message[ "api" ] = "1";
    message[ "params" ] = params;
    return { GroupsNames::UI_GROUP, CLOUD_MESSAGE, std::move( message ) };
}

void CloudAPICommand::run( const ActionInterface::Action& action,
                           ActionInterface::ActionExecutorPtr actionExecutorPtr ) {
    auto sendFinishCallbackPtr = std::make_shared< SendFinishCallback >( action, actionExecutorPtr );
    if( !action.isInitialized() ) {
        LOGGER_ERROR( cc::error( "Action is not initialized" ) );
        return;
    }
    try {
        auto requestData = getRequestData( action.getArgs() );

        const auto& args = action.getArgs();
        std::string name;
        std::string valueType;
        std::string method;
        GlobalVariable::parseSaveResult( args, name, valueType, method );

        std::shared_ptr< GlobalVariable > globalVariable;
        if( method == "setVariable" && !name.empty() ) {
            globalVariable = std::make_shared< GlobalVariable >( m_hostService, name, valueType );
        }

        m_hostService.sendAsyncRequest( requestData.m_serviceName,
                                        requestData.m_requestBody,
                                        requestData.m_requestType,
                                        [globalVariable, sendFinishCallbackPtr]( const JsonObject& message ) mutable {
                                            if( hasError( message ) ) {
                                                if( globalVariable.get() ) {
                                                    Json error = JSON_EMPTY_OBJ;
                                                    json_get( message, "error", error );
                                                    globalVariable->setCloudApiError( error, sendFinishCallbackPtr );
                                                }
                                                else {
                                                    sendFinishCallbackPtr->setResult( false );
                                                }
                                            }
                                            else {
                                                if( globalVariable.get() ) {
                                                    globalVariable->setCloudApiData( message, sendFinishCallbackPtr );
                                                }
                                                else {
                                                    sendFinishCallbackPtr->setResult( true );
                                                }
                                            }
                                            sendFinishCallbackPtr.reset();
                                        },
                                        [sendFinishCallbackPtr]() mutable {
                                            LOGGER_TRACE_RAW( cc::trace( "CloudAPICommand: timeout" ) );
                                            sendFinishCallbackPtr->setResult( false );
                                            sendFinishCallbackPtr.reset();
                                        },
                                        std::chrono::minutes( 1 ) );
    }
    catch( InvalidActionArgs& e ) {
        LOGGER_ERROR( cc::error( "Send host request error: + " ) + cc::attention( e.what() ) );
    }
}

HostRequestAction::RequestData CloudAPICommand::getRequestData( const Action::Arguments& arguments ) {
    JsonObject message = JSON_EMPTY_OBJ;
    message[ "id" ] = Services::Service::generateMessageUUID();
    message[ "method" ] = "cloud." + json_get_str( arguments, "cloudApiName" );
    message[ "api" ] = json_get_int( arguments, "version" );
    std::string paramsStr = json_get_str( arguments, "params" );

    std::string name;
    std::string valueType;
    std::string method;

    GlobalVariable::parseSaveResult( arguments, name, valueType, method );

    JsonObject params;
    try {
        params = Json::parse( paramsStr );
        message[ "params" ] = params;
        return { GroupsNames::UI_GROUP, CLOUD_MESSAGE, std::move( message ) };
    }
    catch( const std::exception& ex ) {
        LOGGER_ERROR( cc::error( "Parsing of cloudAPI parameters failed: " ) + cc::warn( ex.what() ) );
        throw ex;
    }
    return { GroupsNames::UI_GROUP, CLOUD_MESSAGE, std::move( message ) };
}

HostRequestAction::RequestData SetExpressionAction::getRequestData( const Action::Arguments& arguments ) {
    JsonObject message = JSON_EMPTY_OBJ;

    message[ "id" ] = Services::Service::generateMessageUUID();
    message[ "name" ] = json_get_str( arguments, "name" );
    message[ "type" ] = json_get_str( arguments, "type" );
    message[ "code" ] = json_get_str( arguments, "code" );
    message[ "valueType" ] = json_get_str( arguments, "valueType" );
    message[ "error" ] = json_get_str( arguments, "error" );
    message[ "save_previous_params" ] = true;
    JsonObject metadataObj;
    if( json_get_obj( arguments, "metadata", metadataObj ) ) {
        message[ "metadata" ] = metadataObj;
    }

    JsonObject pramsObj;
    if( json_get_obj( arguments, "params", pramsObj ) ) {
        message[ "params" ] = pramsObj;
    }

    return { GroupsNames::SCENE_MANAGER, SCENE_EXPRESSIONS_SET, std::move( message ) };
}

HostRequestAction::RequestData SetVariableAction::getRequestData( const Action::Arguments& arguments ) {
    JsonObject message = JSON_EMPTY_OBJ;

    message[ "name" ] = json_get_str( arguments, "name" );
    message[ "valueType" ] = json_get_str( arguments, "valueType" );

    JsonObject metadataObj;
    if( json_get( arguments, "metadata", metadataObj ) ) {
        message[ "metadata" ] = metadataObj;
    }

    JsonObject valueObj;
    if( json_get( arguments, "value", valueObj ) ) {
        message[ "value" ] = valueObj;
    }

    message[ "setVariable" ] = true;

    return { GroupsNames::SCENE_MANAGER, SCENE_EXPRESSIONS_SET, std::move( message ) };
}

ToggleValueAction::ToggleValueAction( ControllerFactory& controllerFactory,
                                      SceneEngine::Expressions::ExpressionManager& expressionManager,
                                      ScenesRequestHandlerInterface& scenesHandler )
: ControllerCoreActions( controllerFactory )
, m_expressions( expressionManager )
, m_scenesHandler( scenesHandler ) {}

void ToggleValueAction::run( const Action& action, ActionExecutorPtr actionExecutorPtr ) {
    auto sendFinishCallbackPtr = std::make_shared< SendFinishCallback >( action, actionExecutorPtr );

    using namespace HubPlatform::ScenesManager::ActionFields::ExpressionFields;
    if( !action.isInitialized() ) {
        LOGGER_TRACE( "action is not initialized" );
        return;
    }

    auto arguments = action.getArgs();

    if( arguments.find( Expression ) != arguments.end() )  // expression
    {
        actionToggleExpression( sendFinishCallbackPtr, arguments );
    }
    else if( arguments.find( Item ) != arguments.end() )  // item
    {
        actionToggleItem( action, sendFinishCallbackPtr, arguments );
    }
}

void ToggleValueAction::actionToggleItem( const Action& action,
                                          std::shared_ptr< ActionInterface::SendFinishCallback >& sendFinishCallbackPtr,
                                          const SceneEngine::Action::Arguments& arguments ) {
    auto responseCallback = [sendFinishCallbackPtr]( const JsonObject& message ) mutable {
        LOGGER_TRACE_RAW( "setItemValue response: " + cc::j( message ) );
        sendFinishCallbackPtr->setResult( !hasError( message ) && !message.is_null() );
        sendFinishCallbackPtr.reset();
    };

    const auto itemId = json_get_str( arguments, HubPlatform::ScenesManager::ActionFields::ExpressionFields::Item );
    auto controllerInterface = m_controllerFactory.getControllerInterface( action.getControllerId() );
    auto item = controllerInterface->getItem( itemId );
    JsonObject value;
    json_get( item, CoreD::Model::Resources::ItemFields::ValueFieldName, value );

    if( value.is_number_integer() ) {
        auto numberValue = value.get< int64_t >();

        JsonObject value = numberValue == 0 ? 1 : 0;
        controllerInterface->setItemValue(
            JSON( CoreD::Model::Resources::ItemFields::IdFieldName
                  << itemId << CoreD::Model::Resources::ItemFields::ValueFieldName << value ),
            std::move( responseCallback ) );
    }
    else if( value.is_boolean() ) {
        bool boolValue = value.get< bool >();
        JsonObject val = true;
        if( boolValue ) {
            val = false;
        }

        controllerInterface->setItemValue(
            JSON( CoreD::Model::Resources::ItemFields::IdFieldName
                  << itemId << CoreD::Model::Resources::ItemFields::ValueFieldName << val ),
            std::move( responseCallback ) );
    }
    else {
        LOGGER_ERROR( "Toggle value error: source field is failed" );
        sendFinishCallbackPtr->setResult( false );
        return;
    }
}

void ToggleValueAction::actionToggleExpression(
    std::shared_ptr< ActionInterface::SendFinishCallback >& sendFinishCallbackPtr,
    const SceneEngine::Action::Arguments& arguments ) {
    const auto name = json_get_str( arguments, ActionFields::ExpressionFields::Expression );
    auto expression = m_expressions.findByName( name );

    if( expression.empty() ) {
        LOGGER_ERROR( "Toggle value error: expression not found" );
        return;
    }

    const auto value = m_expressions.fetchValue( name ).value;
    JsonObject message;
    message[ "name" ] = name;
    message[ "setVariable" ] = true;
    if( value.is_number_integer() ) {
        auto numberValue = value.get< int64_t >();
        message[ "value" ] = numberValue == 0 ? 1 : 0;
        m_scenesHandler.expressionSet( message );
        sendFinishCallbackPtr->setResult( true );
    }
    else if( value.is_boolean() ) {
        message[ "value" ] = !value.get< bool >();
        m_scenesHandler.expressionSet( message );
        sendFinishCallbackPtr->setResult( true );
    }
    else {
        LOGGER_ERROR( "Toggle value error: type of expression mismatch" );
        sendFinishCallbackPtr->setResult( false );
        return;
    }
}

void UnknownAction::run( const Action& /*action*/, ActionExecutorPtr /*actionExecutorPtr*/ ) {}

SwitchHouseMode::SwitchHouseMode( ControllerFactory& controllerFactory )
: ControllerCoreActions( controllerFactory ) {}

void SwitchHouseMode::run( const ActionInterface::Action& action,
                           ActionInterface::ActionExecutorPtr actionExecutorPtr ) {
    SendFinishCallback sendFinishCallback( action, actionExecutorPtr );

    if( !action.isInitialized() ) {
        LOGGER_TRACE( "action is not initialized" );
        return;
    }
    std::string houseMode = json_get_str( action.getArgs(), "houseMode" );
    if( houseMode.empty() ) {
        LOGGER_DEBUG( "invalid house mode" );
        return;
    }
    auto controllerInterface = m_controllerFactory.getControllerInterface( action.getControllerId() );
    controllerInterface->setHouseMode( houseMode );
    sendFinishCallback.setResult( /*isSuccessful*/ true );
}

std::mutex SendHttpRequest::m_mutex;
SendHttpRequest::CallbacksMapping SendHttpRequest::m_callbacks;
SendHttpRequest::VariablesMapping SendHttpRequest::m_variables;
const int SendHttpRequest::ResponseTimeout;
const int SendHttpRequest::ConnectionTimeout;

SendHttpRequest::SendHttpRequest( Services::Service& hostService )
: m_hostService( hostService ) {}

void SendHttpRequest::run( const ActionInterface::Action& action,
                           ActionInterface::ActionExecutorPtr actionExecutorPtr ) {
    const auto& args = action.getArgs();

    CommonHttp::HttpOptions httpOptions;
    httpOptions.url = json_get_str( args, "url" );
    httpOptions.data = json_get_str( args, "content" );
    httpOptions.type = json_get_str( args, "request" );
    httpOptions.contentType = json_get_str( args, "contentType" );
    httpOptions.skipSecurity = json_get_bool( args, "skipSecurity" );
    httpOptions.handler = "empty";
    const auto& headers = args[ "headers" ];
    bool hasExpectHeader = false;
    bool hasTransferEncoding = false;
    bool hasContentLength = false;
    for( auto it = headers.begin(); it != headers.end(); ++it ) {
        httpOptions.headers.emplace( it.key(), json_get_str( it.value() ) );

        auto header = toLower( it.key() );
        if( header == "expect" )
            hasExpectHeader = true;
        if( header == "transfer-encoding" )
            hasTransferEncoding = true;
        if( header == "content-length" )
            hasContentLength = true;
    }

    if( !httpOptions.data.empty() ) {
        if( !hasExpectHeader ) {
            httpOptions.headers.emplace( "expect", "" );
        }
        if( !hasTransferEncoding && !hasContentLength ) {
            httpOptions.headers.emplace( "transfer-encoding", "chunked" );
        }
    }
    const auto& credential = args[ "credential" ];
    httpOptions.user = json_get_str( credential, "user" );
    httpOptions.password = json_get_str( credential, "password" );
    // TODO: EFWR-3291: make possible to set custom timeouts for tests. 30/60 seconds are very large timeouts for tests
    httpOptions.timeoutMs = ResponseTimeout;
    httpOptions.connectTimeoutMs = ConnectionTimeout;
    httpOptions.redirection = RedirectionNumber;

    std::string variableName;
    std::string valueType;
    std::string saveResultMethod;
    GlobalVariable::parseSaveResult( args, variableName, valueType, saveResultMethod );

    using namespace std::placeholders;
    auto responseCallback = std::bind( &SendHttpRequest::processResponse, _1, _2 );
    auto timeoutCallback = std::bind( &SendHttpRequest::processTimeout, _1, _2 );
    static HttpDispatcher httpDispatcher( responseCallback, timeoutCallback );

    try {
        auto sendFinishCallbackPtr = std::make_shared< SendFinishCallback >( action, actionExecutorPtr );
        auto requestId = httpDispatcher.request( httpOptions );

        std::lock_guard< std::mutex > lock( m_mutex );
        m_callbacks.emplace( requestId, sendFinishCallbackPtr );
        if( saveResultMethod == "setVariable" && !variableName.empty() ) {
            auto variablePtr = std::make_unique< GlobalVariable >( m_hostService, variableName, valueType );
            m_variables.emplace( requestId, std::move( variablePtr ) );
        }
    }
    catch( const std::exception& ex ) {
        LOGGER_ERROR( ex.what() );
    }
}

void SendHttpRequest::processResponse( const std::string& eventHandler, const CommonHttp::EventPtr& event ) {
    CommonHttp::DataReceivedEvent* theEvent = nullptr;
    if( !event.get() || !( theEvent = dynamic_cast< CommonHttp::DataReceivedEvent* >( event.get() ) ) ) {
        return;
    }

    auto callback = extractCallback( event->id() );
    if( !callback ) {
        LOGGER_ERROR( cc::error( "No handler found for HTTP response" ) );
        return;
    }

    auto code = theEvent->code();
    callback->setResult( JSON( "httpAnswerCode" << code ) );

    auto answer = event->toJson();
    JsonObject metadata;
    json_get_obj( answer, "headers", metadata );
    JsonObject value;
    json_get( answer, "data", value );

    metadata[ "code" ] = code;

    auto variablePtr = extractVariable( event->id() );
    if( variablePtr ) {
        if( code >= 200 && code < 400 ) {
            variablePtr->setHTTPData( metadata, value, callback );
        }
        else {
            Json reason;
            auto reasonIt = answer.find( "reason" );
            if( reasonIt != answer.end() && reasonIt->is_object() )
                reason = *reasonIt;
            variablePtr->setHttpError( reason, callback );
        }
    }
    else if( code == HttpCodes::Ok ) {
        callback->setResult( true );
    }
}

void SendHttpRequest::processTimeout( const std::string& eventHandler, const CommonHttp::EventPtr& event ) {
    CommonHttp::ConnectionClosedEvent* theEvent = nullptr;
    if( !event.get() || !( theEvent = dynamic_cast< CommonHttp::ConnectionClosedEvent* >( event.get() ) ) ) {
        return;
    }

    auto callback = extractCallback( event->id() );
    if( !callback ) {
        LOGGER_ERROR( cc::error( "No handler found for HTTP response" ) );
        return;
    }

    auto answer = event->toJson();
    LOGGER_TRACE_RAW( "Connection closed callback: " + cc::j( answer ) );

    /** FIXME: Http::Reason usually means a situation when there is a problem with a connection
     * or environment AND there was no response of the HTTP server. But, this code treats them as if the HTTP server
     * successfully received the request and handled it with the HTTP Status. Actually, a lot of these errors will
     * never reach the HTTP server. */
    const int reasonCode = theEvent->reasonCode();
    switch( static_cast< CommonHttp::Reason >( reasonCode ) ) {
    case CommonHttp::Reason::RESPONSE_TIMEOUT_REACHED:
        callback->setResult( JSON( "httpAnswerCode" << HttpCodes::RequestTimeout ) );
        break;
    case CommonHttp::Reason::CLOSED_BY_SERVER:
        callback->setResult( JSON( "httpAnswerCode" << HttpCodes::InternalServerError ) );
        break;
    case CommonHttp::Reason::CLOSED_BY_CLIENT:
        callback->setResult( JSON( "httpAnswerCode" << HttpCodes::ClientClosedRequest ) );
        break;
    case CommonHttp::Reason::DNS_NAME:
    case CommonHttp::Reason::UNKNOWN_ERROR:
    case CommonHttp::Reason::TOO_MANY_REDIRECTS:
        callback->setResult( JSON( "httpAnswerCode" << HttpCodes::NotFound ) );
        break;
    case CommonHttp::Reason::CONNECTION_ERROR:
        callback->setResult( JSON( "httpAnswerCode" << HttpCodes::ServiceUnavailable ) );
        break;
    case CommonHttp::Reason::SSL_CONNECTION_ERROR:
        callback->setResult( JSON( "httpAnswerCode" << HttpCodes::NetworkAuthenticationRequired ) );
        break;
    default:
        break;
    }

    Json reason;
    auto reasonIt = answer.find( "reason" );
    if( reasonIt != answer.end() && reasonIt->is_object() )
        reason = *reasonIt;

    auto variablePtr = extractVariable( event->id() );
    if( reasonCode != 0 && variablePtr ) {
        variablePtr->setHttpError( reason, callback );
    }
}

SendHttpRequest::Callback SendHttpRequest::extractCallback( const HttpDispatcher::RequestId& requestId ) {
    std::lock_guard< std::mutex > lock( m_mutex );

    Callback result;

    auto it = m_callbacks.find( requestId );
    if( it != m_callbacks.end() ) {
        result = std::move( it->second );
        m_callbacks.erase( it );
    }

    return result;
}

std::unique_ptr< GlobalVariable > SendHttpRequest::extractVariable( const HttpDispatcher::RequestId& requestId ) {
    std::lock_guard< std::mutex > lock( m_mutex );

    std::unique_ptr< GlobalVariable > result;

    auto it = m_variables.find( requestId );
    if( it != m_variables.end() ) {
        result = std::move( it->second );
        m_variables.erase( it );
    }

    return result;
}

HostRequestAction::RequestData RunLuaCodeAction::getRequestData( const Action::Arguments& args ) {
    const auto pluginId = json_get_str( args, "script" );
    if( pluginId.empty() ) {
        throw InvalidActionArgs( "invalid \"script\" field value" );
    }

    ExtensionsManager::RemoteInterface extensionManager( m_hostService );
    const auto scriptId = extensionManager.getAnonymousScriptIdByPluginId( pluginId );

    auto data = JSON_EMPTY_OBJ;
    data[ "script" ] = scriptId;
    data[ "operationId" ] = "";
    data[ "isUserRunScript" ] = false;

    LOGGER_DEBUG( cc::debug( "Send request to execute an associated anonymous script, scriptId: " ) +
                  cc::note( pluginId ) );

    // TODO: Run all plugin scripts only through extensions manager
    return { "addon.lua", EXECUTE_SCRIPT, std::move( data ) };
}

HostRequestAction::RequestData RunLuaPluginScriptAction::getRequestData( const Action::Arguments& args ) {
    const auto scriptId = json_get_str( args, "script" );
    Json scriptParams = JSON_EMPTY_OBJ;
    json_get( args, "params", scriptParams );

    if( scriptId.empty() ) {
        throw InvalidActionArgs( "invalid \"script\" field value" );
    }

    ExtensionsManager::RemoteInterface extensionManager( m_hostService );
    const auto pluginId = extensionManager.getPluginIdByScriptId( scriptId );
    if( pluginId.empty() ) {
        throw InvalidActionArgs( "invalid plugin identifier: '" + pluginId + "'" );
    }

    const std::string& operationId = db::DbId::genOID();
    auto pluginRunParams = JSON_EMPTY_OBJ;
    pluginRunParams[ "script" ] = scriptId;
    pluginRunParams[ "operationId" ] = operationId;
    const bool dontProduceProgressBroadcasts = false;
    pluginRunParams[ "isUserRunScript" ] = dontProduceProgressBroadcasts;
    if( !scriptParams.empty() ) {
        pluginRunParams[ "scriptParams" ] = std::move( scriptParams );
    }

    LOGGER_DEBUG( cc::debug( "Send request to execute a plugin script, scriptId: " ) + cc::note( scriptId ) +
                  cc::debug( ", operationId: " ) + cc::note( operationId ) );

    return { GroupsNames::EXTENSIONS_MANAGER, PLUGIN_RUN_REQUEST, std::move( pluginRunParams ) };
}

HostRequestAction::RequestData RunSceneAction::getRequestData( const Action::Arguments& args ) {
    const auto scene = json_get_str( args, "sceneId" );
    if( scene.empty() ) {
        throw InvalidActionArgs( "invalid \"sceneId\" field value" );
    }
    bool global = false;
    if( !json_get( args, "global", global ) ) {
        throw InvalidActionArgs( "invalid \"global\" field value" );
    }
    const auto block = json_get_str( args, ActionResources::Fields::Block );
    const auto group = json_get_str( args, ActionResources::Fields::Group );
    return { GroupsNames::SCENE_MANAGER,
             SCENE_RUN,
             JSON( "sceneId" << scene << "global" << global << ActionResources::Fields::Block << block
                             << ActionResources::Fields::Group << group ) };
}

HostRequestAction::RequestData StopSceneAction::getRequestData( const Action::Arguments& args ) {
    const auto scene = json_get_str( args, "sceneId" );
    if( scene.empty() ) {
        throw InvalidActionArgs( "stopScene: invalid \"sceneId\" field value" );
    }
    bool global = false;
    if( !json_get( args, "global", global ) ) {
        throw InvalidActionArgs( "stopScene: invalid \"global\" field value" );
    }
    const auto block = json_get_str( args, ActionResources::Fields::Block );
    const auto group = json_get_str( args, ActionResources::Fields::Group );
    return { GroupsNames::SCENE_MANAGER,
             SCENE_STOP,
             JSON( "sceneId" << scene << "global" << global << ActionResources::Fields::Block << block
                             << ActionResources::Fields::Group << group ) };
}

HostRequestAction::RequestData SetSceneStateAction::getRequestData( const Action::Arguments& args ) {
    const auto sceneId = json_get_str( args, "sceneId" );
    if( sceneId.empty() ) {
        throw InvalidActionArgs( "invalid \"sceneId\" field value" );
    }
    bool global = false;
    if( !json_get( args, "global", global ) ) {
        throw InvalidActionArgs( "invalid \"global\" field value" );
    }
    bool enabled = false;
    if( !json_get( args, "enabled", enabled ) ) {
        throw InvalidActionArgs( "invalid \"enabled\" field value" );
    }
    return {
        GroupsNames::SCENE_MANAGER, SCENE_ENABLE, JSON( "_id" << sceneId << "global" << global << "enabled" << enabled )
    };
}

HostRequestAction::RequestData ResetLatchAction::getRequestData( const Action::Arguments& args ) {
    const std::string sceneIdParam = ActionFields::Fields::SceneId;
    const std::string sceneId = json_get_str( args, sceneIdParam );
    if( sceneId.empty() ) {
        throw InvalidActionArgs( "invalid \"" + sceneIdParam + "\" field value" );
    }

    const std::string blockIdParam = ActionFields::Fields::BlockId;
    const std::string blockId = json_get_str( args, blockIdParam );
    if( blockId.empty() ) {
        throw InvalidActionArgs( "invalid \"" + blockIdParam + "\" field value" );
    }

    return {
        GroupsNames::SCENE_MANAGER, SCENE_BLOCK_STATUS_RESET, JSON( sceneIdParam << sceneId << blockIdParam << blockId )
    };
}

HostRequestAction::RequestData ResetSceneLatchesAction::getRequestData( const Action::Arguments& args ) {
    const std::string sceneIdParam = ActionFields::Fields::SceneId;
    const std::string sceneId = json_get_str( args, sceneIdParam );
    if( sceneId.empty() ) {
        throw InvalidActionArgs( "invalid \"" + sceneIdParam + "\" field value" );
    }

    return { GroupsNames::SCENE_MANAGER, SCENE_BLOCK_STATUS_RESET, JSON( sceneIdParam << sceneId ) };
}

HostRequestAction::RequestData RebootHub::getRequestData( const SceneEngine::Action::Arguments& args ) {
    return { GroupsNames::HARDWARE_MANAGER, HUB_REBOOT, JSON_EMPTY_OBJ };
}

ResetHubAction::ResetHubAction( Services::Service& hostService )
: m_hostService( hostService ) {}

void ResetHubAction::run( const ActionInterface::Action& action,
                          ActionInterface::ActionExecutorPtr actionExecutorPtr ) {
    auto sendFinishCallbackPtr = std::make_shared< SendFinishCallback >( action, actionExecutorPtr );
    if( !action.isInitialized() ) {
        LOGGER_ERROR( cc::error( "Action is not initialized" ) );
        return;
    }
    HubPlatform::UpdateManager::HubResetType resetType;
    const auto resetTypeStr = json_get_str( action.getArgs(), ActionResources::Fields::ResetType );
    if( resetTypeStr == ActionResources::Values::ResetTypeSoft ) {
        resetType = HubPlatform::UpdateManager::HubResetType::Soft;
    }
    else if( resetTypeStr == ActionResources::Values::ResetTypeFactory ) {
        resetType = HubPlatform::UpdateManager::HubResetType::Factory;
    }
    else {
        LOGGER_ERROR( cc::error( "Invalid reset type field value" ) );
        return;
    }
    sendFinishCallbackPtr->setResult( /*isSuccessful*/ true );
    m_hostService.sendMethodCall(
        GroupsNames::UPDATE_MANAGER, HUB_RESET_REQUEST, HubPlatform::UpdateManager::hubResetTypeToJson( resetType ) );
}

GlobalVariable::GlobalVariable( Services::Service& hostService, std::string name, std::string valueType )
: m_hostService( hostService )
, m_name( std::move( name ) )
, m_valueType( std::move( valueType ) ) {}

void GlobalVariable::parseSaveResult( const SceneEngine::Action::Arguments& args,
                                      std::string& name,
                                      std::string& valueType,
                                      std::string& method ) {
    if( args.count( "saveResult" ) ) {
        JsonObject saveResult;
        json_get_obj( args, "saveResult", saveResult );
        JsonObject saveResultArgs;
        json_get_obj( saveResult, "args", saveResultArgs );
        method = json_get_str( saveResult, "method" );
        name = json_get_str( saveResultArgs, "name" );
        valueType = json_get_str( saveResultArgs, "valueType" );
    }
}

void GlobalVariable::setHTTPData( Json& metadata, Json& value, SendHttpRequest::Callback callback ) {
    auto globalVariableMetadata = JSON( SaveResultKey << metadata );
    JsonObject message = JSON( "name" << m_name << "valueType"
                                      << "string"
                                      << "setVariable" << true << "metadata" << globalVariableMetadata );

    LOGGER_TRACE( "message without value: " + cc::j( message ) );

    if( value.is_string() ) {
        auto dataPtr = value.get_ptr< const Json::string_t* >();
        auto valueSize = dataPtr->size();
        auto restrictedLength = std::min( ValueMaxLen, valueSize );
        if( restrictedLength < valueSize )
            message[ "value" ] = std::string{ dataPtr->data(), restrictedLength };
        else
            message[ "value" ] = *dataPtr;
    }
    else {
        std::string dump = value.dump();
        auto restrictedLength = std::min( ValueMaxLen, dump.size() );
        if( restrictedLength < dump.size() )
            message[ "value" ] = std::string{ dump.data(), restrictedLength };
        else
            message[ "value" ] = dump;
    }

    m_hostService.sendAsyncRequest(
        GroupsNames::SCENE_MANAGER, message, SCENE_EXPRESSIONS_SET, [callback]( const Json& message ) {
            if( hasError( message ) ) {
                LOGGER_ERROR_RAW( cc::error( "HostRequestAction failed" ) );
                LOGGER_DEBUG_RAW( cc::debug( "HostRequestAction: receiveCallback: " ) + cc::j( message ) );
                callback->setResult( false );
            }
            else {
                callback->setResult( true );
            }
        } );
}

void GlobalVariable::setHttpError( Json& reason, SendHttpRequest::Callback callback ) {
    auto resultKey = JSON( SaveResultKey << reason );

    JsonObject message = JSON( "method"
                               << "setVariable"
                               << "name" << m_name << "valueType"
                               << "object"
                               << "metadata" << JSON_EMPTY_OBJ << "setVariable" << true << "error" << resultKey
                               << "value" << JSON_EMPTY_OBJ );

    m_hostService.sendAsyncRequest(
        GroupsNames::SCENE_MANAGER, std::move( message ), SCENE_EXPRESSIONS_SET, [callback]( const Json& message ) {
            callback->setResult( false );
        } );
}

void GlobalVariable::setCloudApiData( const Json& value, SendHttpRequest::Callback callback ) {
    JsonObject message = JSON( "name" << m_name << "method"
                                      << "setVariable"
                                      << "valueType"
                                      << "object"
                                      << "setVariable" << true << "metadata" << JSON_EMPTY_OBJ << "value" << value );

    m_hostService.sendAsyncRequest(
        GroupsNames::SCENE_MANAGER, std::move( message ), SCENE_EXPRESSIONS_SET, [callback]( const Json& message ) {
            if( hasError( message ) ) {
                callback->setResult( false );
            }
            else {
                callback->setResult( true );
            }
        } );
}

void GlobalVariable::setCloudApiError( const Json& error, SendHttpRequest::Callback callback ) {
    JsonObject message = JSON( "name" << m_name << "method"
                                      << "setVariable"
                                      << "metadata" << JSON_EMPTY_OBJ << "error" << error << "setVariable" << true
                                      << "value" << JSON_EMPTY_OBJ );

    m_hostService.sendAsyncRequest(
        GroupsNames::SCENE_MANAGER, std::move( message ), SCENE_EXPRESSIONS_SET, [callback]( const Json& message ) {
            { callback->setResult( false ); }
        } );
}
JsonArray GroupSetItemValue::expandDeviceGroup( const JsonObject& deviceGroup,
                                                const Action& action,
                                                ControllerFactory& controllerFactory ) {
    using namespace SceneEngine::Model::Resources;
    auto controllerInterface = controllerFactory.getControllerInterface( action.getControllerId() );
    JsonArray devices;
    json_get_arr( deviceGroup, DeviceGroup::Devices, devices );
    JsonArray exceptions;
    json_get_arr( deviceGroup, DeviceGroup::Exceptions, exceptions );

    JsonArray categoryArray;
    json_get_arr( deviceGroup, DeviceGroup::Categories, categoryArray );

    JsonArray deviceCategoryArray;
    JsonArray queryArray;
    using namespace CoreD::Model::Resources;
    for( int i = 0; i < categoryArray.size(); i++ ) {
        JsonArray deviceSubcategoryArray;
        deviceCategoryArray[ i ] = json_get_str( categoryArray[ i ], DeviceGroup::Category );
        json_get_arr( categoryArray[ i ], DeviceGroup::Subcategories, deviceSubcategoryArray );
        if( !exceptions.empty() ) {
            for( int x = 0; x < exceptions.size(); x++ ) {
                queryArray[ i ] =
                    JSON( DeviceGroup::Category << deviceCategoryArray[ i ] << DeviceGroup::Subcategory
                                                << JSON( "$in" << deviceSubcategoryArray ) << DeviceFields::IdFieldName
                                                << JSON( "$ne" << exceptions[ x ] ) );
            }
        }
        else {
            queryArray[ i ] = JSON( DeviceGroup::Category << deviceCategoryArray[ i ] << DeviceGroup::Subcategory
                                                          << JSON( "$in" << deviceSubcategoryArray ) );
        }
    }

    if( !devices.empty() ) {
        for( int i = 0; i < devices.size(); i++ ) {
            queryArray.push_back( JSON( DeviceFields::IdFieldName << devices[ i ] ) );
        }
    }

    auto devicesArray = controllerInterface->getDevicesBy( JSON( "$or" << queryArray ) );

    JsonArray deviceIdsArray;
    for( int i = 0; i < devicesArray.size(); i++ ) {
        deviceIdsArray[ i ] = json_get_str( devicesArray.at( i ), DeviceFields::IdFieldName );
    }

    return deviceIdsArray;
}

std::vector< JsonObject > GroupSetItemValue::expandItemGroup( const JsonObject& itemGroup,
                                                              const JsonArray& deviceIdsArray,
                                                              const Action& action,
                                                              ControllerFactory& controllerFactory ) {
    using namespace CoreD::Model::Resources;
    auto controllerInterface = controllerFactory.getControllerInterface( action.getControllerId() );
    using namespace SceneEngine::Model::Resources;
    JsonArray itemNamesArray;
    json_get_arr( itemGroup, ItemGroup::ItemNames, itemNamesArray );

    auto query = JSON( ItemFields::DeviceIdFieldName << JSON( "$in" << deviceIdsArray ) << ItemFields::NameFieldName
                                                     << JSON( "$in" << itemNamesArray ) << ItemFields::TypeFieldName
                                                     << json_get_str( itemGroup, ItemGroup::ValueType ) );

    auto itemsArray = controllerInterface->getItemsBy( query );
    return itemsArray;
}

}  // namespace ScenesManager
}  // namespace HubPlatform
