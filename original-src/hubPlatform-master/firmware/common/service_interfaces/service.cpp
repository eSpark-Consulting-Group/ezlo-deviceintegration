#include "service_interfaces/service.h"
#include "service_interfaces/bus_wrapper.h"
#include "service_interfaces/script_event.h"
#include "hub_settings/hub_settings_reader.h"

#include "database_client/dbid.h"
#include "common_utils/cpp14.h"
#include "json_utils/json_utils.h"
#include "jsonrpc/error_codes.h"
#include "jsonrpc/jsonrpc.h"
#include "base_utils/timeutils.h"

#include <chrono>
#include <cassert>
#include <future>
#include <utility>

namespace HubPlatform {
namespace Services {

Service::Service( const std::string& name )
: Service( name, nullptr, nullptr ) {}

void Service::setReporter( std::unique_ptr< ReporterInterface > reporterInterfacePtr ) {
    if( m_reportLoop && m_reportLoop->isActive() ) {
        m_reportLoop->stop();
    }

    m_reporter = std::move( reporterInterfacePtr );
    if( m_reporter ) {
        if( !m_reportLoop ) {
            m_reportLoop = createPostponedTask( [this]( Services::PostponedTask& ) {
                if( m_reporter ) {
                    m_reporter->sendReportData();
                }
                else {
                    m_reportLoop->stop();
                }
            } );
        }

        m_reportLoop->start( std::chrono::seconds( m_reporter->postInterval() ), true );
    }
}

void Service::addReporter( std::unique_ptr< ReporterInterface > reporterInterfacePtr ) {
    if( m_addReporterLoop && m_addReporterLoop->isActive() ) {
        m_addReporterLoop->stop();
    }
    m_reporterArray.push_back( std::move( reporterInterfacePtr ) );
    if( !m_reporterArray.empty() ) {
        if( !m_addReporterLoop ) {
            m_addReporterLoop = createPostponedTask( [this]( Services::PostponedTask& ) {
                if( !m_reporterArray.empty() ) {
                    for( auto& reporter : m_reporterArray ) {
                        reporter->sendReportData();
                    }
                }
                else {
                    m_reportLoop->stop();
                }
            } );
        }

        m_addReporterLoop->start( std::chrono::seconds( m_reporterArray[ 0 ]->postInterval() ), true );
    }
}

Service::Service( std::string name,
                  std::unique_ptr< BusWrapper > busWrapper,
                  std::unique_ptr< IDbClient > dbClient,
                  size_t taskThreadCount )
: TaskLoop( taskThreadCount )
, m_name( std::move( name ) )
, m_busWrapper( std::move( busWrapper ) )
, m_dbClient( std::move( dbClient ) )
, m_isRunning( false ) {}

Service::~Service() = default;

void Service::defineDaemonParameters( tiny::command_line::parser& /*parser*/ ) {}

BaseBus* Service::getBus() {
    return m_busWrapper ? &( m_busWrapper->getBus() ) : nullptr;
}

const std::string& Service::getBinaryName() {
    return m_binaryName;
}

const Json& Service::unhandledRequestAnswer() const {
    static const Json answer = JSON( "unhandled" << true );
    return answer;
}

void Service::setUp() {
    if( !m_busWrapper ) {
        try {
            m_busWrapper = std::make_unique< Services::BusWrapper >();
        }
        catch( const std::runtime_error& error ) {
            Log::error( cc::error( "Failed to create bus: " ) + cc::note( error.what() ) );
            ::exit( 0 );
        }
    }

    createDatabase();

    m_busWrapper->setUp( getQualifiedName(), [this]( const bus_rx_data_t* rxData ) {
        if( rxData && getBus()->isRegularMessage( rxData->service_type ) ) {
            assert( rxData->num_groups > 0 );

            auto data = JsonObject::parse( std::string( rxData->msg, rxData->msg_size ) );
            auto msgClass = rxData->msg_type;
            auto msgSubClass = msgSubclassToKey( msgClass, data[ "msg_subclass" ] );
            auto busSender = std::string( rxData->sender );
            auto group = std::string( rxData->target_groups[ 0U ] );

            Message message = { std::move( group ), msgClass, msgSubClass, std::move( data[ "result" ] ) };

            switch( message.msgClass ) {
            case RESPONSE:
                handleResponseFromBus( message );
                break;

            case REQUEST:
                addTask( [this, message]() {
                    const auto& messageBody = message.data;
                    const auto& request = messageBody[ "requestBody" ];
                    std::string uuid = messageBody[ "uuid" ];
                    std::string sender = messageBody[ "sender" ];
                    const auto requestType = static_cast< MessageSubClasses >( message.msgSubClass );

                    if( !handleAsyncRequestFromBus( request, requestType, uuid, sender ) ) {
                        auto response = handleRequestFromBus( request, requestType );
                        if( response != unhandledRequestAnswer() ) {
                            Message responseMessage;
                            responseMessage.data = JSON_EMPTY_OBJ;
                            responseMessage.data[ "responseBody" ] = std::move( response );
                            responseMessage.data[ "uuid" ] = std::move( uuid );
                            responseMessage.group = std::move( sender );
                            responseMessage.msgClass = RESPONSE;
                            responseMessage.msgSubClass = RESPONSE_RESULT;
                            send( responseMessage );
                        }
                    }
                } );

                break;

            case JSON_RPC:
                if( message.msgSubClass == CALL_METHOD ) {
                    addTask( [this, message, busSender]() { handleLegacyMethodCall( message.data, busSender ); } );
                }
                else {
                    addTask( [this, message, busSender]() {
                        const auto requestType = static_cast< MessageSubClasses >( message.msgSubClass );
                        handleMethodCall( message.data, requestType, busSender );
                    } );
                }
                break;

            default:
                addTask( [this, message, busSender]() { handleMessageFromBus( message, busSender ); } );
                break;
            }
        }
    } );

    initLogOptions();

    // Send event about the service was started.
    JsonObject serviceId = JSON( "id" << getQualifiedName() );
    Services::Message message = { GroupsNames::BROADCAST_GROUP, EVENT, SERVICE_STARTED, serviceId };
    send( message );

    if( !m_reporter )
        setReporter( std::make_unique< Reporter >( getName() ) );

    printHubUptime();
    Log::info( cc::info( "Service Starting: " + getQualifiedName() + " ......." ) );
    Log::printLogCounter();
    m_serviceStartupTime = std::chrono::steady_clock::now();
    createPostponedTask( [this]( Services::PostponedTask& ) {
        printServiceStartupTime();
        Log::printLogCounter();
        auto serviceInfo = getServiceInfo();
        if( !serviceInfo.empty() ) {
            Log::info( "Service info: " + cc::j( serviceInfo ) );
        }
    } )
        ->start( ServiceIntervalTime( 60 ), true );
}

void Service::handleMessageFromBus( const Services::Message& message, const std::string& sender ) {
    if( message.msgClass == LOG_CMD ) {
        handleLogCommand( message, sender );
    }
    else if( message.msgClass == EVENT ) {
        if( message.msgSubClass == HUB_SYSTEM_DATETIME_CHANGED ) {
            handleSystemDateTimeChanged();
        }
        else if( message.msgSubClass == HUB_LOGS_ROTATED ) {
            handleRotateLogsBroadcast();
        }
    }
}

void Service::handleResponseFromBus( const Message& message ) {
    const auto uuid = json_get_str( message.data, "uuid" );

    {
        std::unique_lock< std::mutex > lock( m_responsesMapMutex );
        auto responseIt = m_responsesMap.find( uuid );
        if( responseIt != m_responsesMap.end() ) {
            auto promise = std::move( responseIt->second );
            m_responsesMap.erase( responseIt );
            lock.unlock();

            promise.set_value( message.data[ "responseBody" ] );
        }
    }

    handleAsyncResponseFromBus( uuid, message.data[ "responseBody" ] );
}

void Service::handleAsyncResponseFromBus( const std::string& uuid, const Json& response ) {
    std::unique_lock< std::mutex > lock( m_asyncResponsesMapMutex );
    auto it = m_asyncResponsesMap.find( uuid );
    if( it != m_asyncResponsesMap.end() ) {
        auto data = std::move( it->second );
        m_asyncResponsesMap.erase( it );
        lock.unlock();
        data.timer->stop();
        const auto& callback = data.receiveCallback;
        addTask( [callback, response]() mutable { callback( response ); } );
    }
}

void Service::handleLegacyMethodCall( const JsonObject& json, const std::string& /*sender*/ ) {
    const auto method = json_get_str( json[ "method" ] );
    if( method == UnsubscribeMethod ) {
        unsubscribe( PluginEventsSubscriber( json[ "params" ] ) );
    }
}

void Service::handleMethodCall( const JsonObject& /*json*/,
                                MessageSubClasses /*messageSubClasses*/,
                                const std::string& /*sender*/ ) {
    // Do nothing unless overridden
}

JsonObject Service::handleRequestFromBus( const JsonObject& request, MessageSubClasses requestType ) {
    switch( requestType ) {
    case SUBSCRIBE_FOR_PLUGIN_EVENTS: {
        return subscribe( PluginEventsSubscriber( request ) );
    }

    case CHANGE_SUBSCRIBER_EVENT_FILTERS: {
        const auto filterRules = PluginEventsSubscriber::createFilterRulesFromJson( request.at( "rules" ) );
        return changeSubscriberFilterRules( request.at( "moduleName" ), request.at( "subscriberId" ), filterRules );
    }

    case SERVICE_INFO_GET: {
        return handleServiceInfoGet( request );
    }

    default:
        return unhandledRequestAnswer();
    }
}

bool Service::handleAsyncRequestFromBus( const JsonObject& /*request*/,
                                         MessageSubClasses /*requestType*/,
                                         const std::string& /*uuid*/,
                                         const std::string& /*sender*/ ) {
    // By default handle requests synchronously
    return false;
}

bool Service::isRunning() const noexcept {
    return m_isRunning;
}

void Service::emit( const ScriptEvent& event ) {
    // order subscribers get notified is not specified
    m_subscribers.process( [this, &event]( SubscribersByModuleName& subscribers ) {
        auto subscribersIt = subscribers.find( event.getModuleName() );
        if( subscribersIt == subscribers.end() )
            return;

        for( const auto& pluginSubscribers : subscribersIt->second ) {
            for( const auto& subscriber : pluginSubscribers.second ) {
                subscriber.second.notify( event, [this]( const Message& notifyMessage ) { send( notifyMessage ); } );
            }
        }
    } );
}

void Service::emit( const std::string& pluginId, const ScriptEvent& event ) {
    m_subscribers.process( [this, &pluginId, &event]( SubscribersByModuleName& subscribers ) {
        auto subscribersIt = subscribers.find( event.getModuleName() );
        if( subscribersIt == subscribers.end() )
            return;

        for( const auto& pluginSubscribers : subscribersIt->second ) {
            if( pluginId != pluginSubscribers.first ) {
                continue;
            }
            for( const auto& subscriber : pluginSubscribers.second ) {
                subscriber.second.notify( event, [this]( const Message& notifyMessage ) { send( notifyMessage ); } );
            }
        }
    } );
}

void Service::createDatabase() {
    if( !m_dbClient ) {
        m_dbClient = createDbClient();
    }
}

Json Service::subscribe( PluginEventsSubscriber subscriber ) {
    bool subscriberAdded = false;
    const PluginId pluginId = subscriber.getPluginId();

    Json result = m_subscribers.process( [&subscriber, &subscriberAdded]( SubscribersByModuleName& subscribers ) {
        const auto subscriberId = subscriber.getId();
        LOGGER_DEBUG_RAW( cc::debug( "Register a " ) + cc::note( subscriberId ) + cc::debug( " subscriber for the " ) +
                          cc::note( subscriber.getModuleName() ) + cc::debug( " module" ) );
        LOGGER_DEBUG_RAW( cc::trace( "Event filter rules: " ) +
                          cc::note( subscriber.getEventsFilterRulesAsJson().dump( 2 ) ) );

        PluginSubscribers& pluginSubscribers = subscribers[ subscriber.getModuleName() ][ subscriber.getPluginId() ];
        std::tie( std::ignore, subscriberAdded ) = pluginSubscribers.emplace( subscriberId, std::move( subscriber ) );

        return JSON( "subscriberId" << subscriberId );
    } );

    if( subscriberAdded ) {
        onSubscriberAdded( pluginId );
    }

    return result;
}

void Service::unsubscribe( const PluginEventsSubscriber& subscriber ) {
    m_subscribers.process( [&subscriber]( SubscribersByModuleName& subscribers ) {
        subscribers[ subscriber.getModuleName() ][ subscriber.getPluginId() ].erase( subscriber.getId() );
        LOGGER_DEBUG_RAW( cc::debug( "Unregister a " ) + cc::note( subscriber.getId() ) +
                          cc::debug( " subscriber from the " ) + cc::note( subscriber.getModuleName() ) +
                          cc::debug( " module" ) );
    } );
}

Json Service::changeSubscriberFilterRules( const PluginEventsSubscriber::ModuleName& moduleName,
                                           const PluginEventsSubscriber::PluginEventsSubsriberId& subscriberId,
                                           const PluginEventsSubscriber::FilterRules& filterRules ) {
    return m_subscribers.process( [&moduleName, &subscriberId, &filterRules]( SubscribersByModuleName& subscribers ) {
        bool validSubscriberId = false;
        Json answer = JSON_EMPTY_OBJ;

        auto subscribersIt = subscribers.find( moduleName );
        if( subscribersIt == subscribers.end() ) {
            setError( answer, Errors::WRONG_PARAMS );
            return answer;
        }

        for( auto& pluginSubscribers : subscribersIt->second ) {
            auto it = pluginSubscribers.second.find( subscriberId );
            if( it != pluginSubscribers.second.end() ) {
                auto& subscriber = it->second;
                auto& subscriberFilterRules = subscriber.getEventsFilterRules();

                for( const auto& filterRulePair : filterRules ) {
                    const auto& filterRuleId = filterRulePair.first;
                    const auto& filterRule = filterRulePair.second;

                    if( filterRule.m_remove ) {
                        subscriberFilterRules.erase( filterRuleId );
                    }
                    else {
                        subscriberFilterRules[ filterRuleId ] = filterRule;
                    }

                    LOGGER_DEBUG_RAW( cc::debug( "Change event filter rules for a " ) + cc::note( subscriber.getId() ) +
                                      cc::debug( " subscriber for the " ) + cc::note( subscriber.getModuleName() ) +
                                      cc::debug( " module" ) );
                    LOGGER_DEBUG_RAW( cc::trace( "Event filter rules: " ) +
                                      cc::note( subscriber.getEventsFilterRulesAsJson().dump( 2 ) ) );
                }

                validSubscriberId = true;
                break;
            }
        }

        if( !validSubscriberId ) {
            setError( answer, Errors::WRONG_PARAMS );
        }

        return answer;
    } );
}

const std::string& Service::getHostChannel() {
    return getQualifiedName();
}

void Service::tearDown() {
    printHubUptime();
    printServiceStartupTime();
    Log::printLogCounter();
    // Send event about the service is stopping.
    JsonObject serviceId = JSON( "id" << getName() );
    Services::Message message = { GroupsNames::BROADCAST_GROUP, EVENT, SERVICE_STOPPED, serviceId };
    send( message );

    setReporter( nullptr );
    if( !m_reporterArray.empty() ) {
        m_reporterArray.clear();
    }

    if( m_reportLoop ) {
        m_reportLoop = nullptr;
    }
    if( m_addReporterLoop ) {
        m_addReporterLoop = nullptr;
    }

    m_busWrapper->tearDown();
}

void Service::run() {
    m_isRunning = true;
    setUp();
    setUpFinished();

    auto tearDown = std::bind( &Service::tearDown, std::placeholders::_1 );
    std::unique_ptr< Service, decltype( tearDown ) > guard( this, tearDown );

    TaskLoop::run();
}

void Service::stop( StoppingMode mode ) {
    assert( m_isRunning );
    m_isRunning = false;
    TaskLoop::stop( mode );
}

void Service::join( const std::string& group ) {
    m_busWrapper->getBus().join( group.c_str() );
}

bool Service::send( const Message& message ) {
    try {
        sendUnsafe( message );
    }
    catch( const MaxMessageLengthError& ex ) {
        LOGGER_ERROR( "Send message error: " + ex.what() );
        return false;
    }
    catch( std::exception& ex ) {
        LOGGER_ERROR( "Send message error: " + ex.what() );
        return false;
    }
    catch( ... ) {
        LOGGER_ERROR( "Send message error " );
        return false;
    }

    return true;
}

void Service::sendUnsafe( const Message& message ) {
    m_busWrapper->getBus().send( message.group.c_str(), message.msgClass, message.msgSubClass, message.data );
}

JsonObject Service::sendRequest( const std::string& serviceName,
                                 const JsonObject& request,
                                 MessageSubClasses requestType,
                                 std::chrono::seconds timeout ) {
    std::promise< JsonObject > promise;
    auto future = promise.get_future();

    const auto uuid = generateMessageUUID();

    {
        std::lock_guard< std::mutex > lock( m_responsesMapMutex );
        m_responsesMap.insert( std::make_pair( uuid, std::move( promise ) ) );
    }

    Message messageToSend;
    messageToSend.data = JSON_EMPTY_OBJ;
    messageToSend.data[ "requestBody" ] = request;
    messageToSend.data[ "uuid" ] = uuid;
    messageToSend.data[ "sender" ] = getQualifiedName();
    messageToSend.msgClass = REQUEST;
    messageToSend.msgSubClass = requestType;
    messageToSend.group = serviceName;

    JsonObject response = JSON_EMPTY_OBJ;

    try {
        sendUnsafe( messageToSend );
    }
    catch( const MaxMessageLengthError& ex ) {
        const auto errorObject = Errors::get( Errors::BUS_MESSAGE_TOO_LONG );
        setErrorObject( response, errorObject );
        return response;
    }
    catch( const std::exception& ex ) {
        Json errorObject;
        errorObject[ "error" ] = Errors::get( Errors::BUS_INTERNAL_ERROR );
        setErrorObject( response, errorObject );
        return response;
    }

    const auto status = future.wait_for( timeout );
    switch( status ) {
    case std::future_status::ready:
        response = future.get();
        break;

    case std::future_status::deferred:
        assert( false );
        break;

    case std::future_status::timeout: {
        LOGGER_TRACE( "Timeout of task was reached. " );
        {
            std::lock_guard< std::mutex > lock( m_responsesMapMutex );
            try {
                auto itFind = m_responsesMap.find( uuid );
                auto removingPromise = std::move( itFind->second );
                m_responsesMap.erase( itFind );
            }
            catch( const std::future_error& error ) {
                LOGGER_TRACE( "std::promise has thrown exception during destroying: " + error.what() );
            }
        }

        const auto errorObject = Errors::get( Errors::BUS_REQUEST_TIMEOUT );
        setErrorObject( response, errorObject );
    } break;
    }

    return response;
}

Service::RequestUuid Service::sendAsyncRequest( const std::string& serviceName,
                                                const JsonObject& request,
                                                MessageSubClasses requestType,
                                                ReceiveCallback receiveCallback,
                                                TimeoutCallback timeoutCallback,
                                                std::chrono::seconds timeout ) {
    auto uuid = generateMessageUUID();
    auto timer = createPostponedTask( [uuid, this]( Services::PostponedTask& ) mutable {
        std::unique_lock< std::mutex > lock( m_asyncResponsesMapMutex );
        auto it = m_asyncResponsesMap.find( uuid );
        if( it == m_asyncResponsesMap.end() ) {
            return;
        }
        auto data = std::move( it->second );
        m_asyncResponsesMap.erase( it );
        lock.unlock();
        data.timer->stop();
        data.timeoutCallback();
    } );

    std::unique_lock< std::mutex > lock( m_asyncResponsesMapMutex );
    m_asyncResponsesMap[ uuid ] = { std::move( receiveCallback ), std::move( timeoutCallback ), timer };
    lock.unlock();
    timer->start( timeout, /*isPeriodic*/ false );

    if( serviceName == m_name ) {
        addTask( [this, request, requestType, uuid]() mutable {
            if( !handleAsyncRequestFromBus( request, requestType, uuid, getQualifiedName() ) ) {
                auto response = handleRequestFromBus( request, requestType );
                if( response != unhandledRequestAnswer() ) {
                    handleAsyncResponseFromBus( uuid, response );
                }
            }
        } );
    }
    else {
        Message messageToSend;
        messageToSend.data = JSON_EMPTY_OBJ;
        messageToSend.data[ "requestBody" ] = request;
        messageToSend.data[ "uuid" ] = uuid;
        messageToSend.data[ "sender" ] = getQualifiedName();
        messageToSend.msgClass = REQUEST;
        messageToSend.msgSubClass = requestType;
        messageToSend.group = serviceName;
        try {
            sendUnsafe( messageToSend );
        }
        catch( const MaxMessageLengthError& ex ) {
            auto callback = m_asyncResponsesMap[ uuid ].receiveCallback;
            const auto errorObject = Errors::get( Errors::BUS_MESSAGE_TOO_LONG );

            Json response;
            setErrorObject( response, errorObject );
            if( callback ) {
                callback( response );
            }
        }
    }
    return uuid;
}

Service::RequestUuid Service::sendAsyncRequest( const std::string& serviceName,
                                                const JsonObject& request,
                                                MessageSubClasses requestType,
                                                ReceiveCallback receiveCallback,
                                                std::chrono::seconds timeout ) {
    auto timeoutCallback = [receiveCallback]() {
        Json response;
        setError( response, Errors::BUS_REQUEST_TIMEOUT );
        receiveCallback( response );
    };

    return sendAsyncRequest(
        serviceName, request, requestType, std::move( receiveCallback ), std::move( timeoutCallback ), timeout );
}

JsonObject Service::sendAsyncResponse( const std::string& serviceName,
                                       const JsonObject& response,
                                       const std::string& uuid ) {
    if( serviceName == m_name ) {
        handleAsyncResponseFromBus( uuid, response );
    }
    else {
        Message responseMessage;
        responseMessage.data = JSON_EMPTY_OBJ;
        responseMessage.data[ "responseBody" ] = response;
        responseMessage.data[ "uuid" ] = uuid;
        responseMessage.group = serviceName;
        responseMessage.msgClass = RESPONSE;
        responseMessage.msgSubClass = RESPONSE_RESULT;

        try {
            sendUnsafe( responseMessage );
        }
        catch( const MaxMessageLengthError& ex ) {
            auto callback = m_asyncResponsesMap[ uuid ].receiveCallback;
            const auto errorObject = Errors::get( Errors::BUS_MESSAGE_TOO_LONG );

            Json response;
            setErrorObject( response, errorObject );
            if( callback ) {
                callback( response );
            }
        }
    }

    return JSON_EMPTY_OBJ;
}

void Service::discardExistingAsyncRequestResponses() {
    std::unique_lock< std::mutex > lock( m_asyncResponsesMapMutex );
    m_asyncResponsesMap.clear();
}

JsonObject Service::sendMethodCall( const std::string& serviceName,
                                    MessageSubClasses methodType,
                                    const JsonObject& data ) {
    try {
        m_busWrapper->getBus().send( serviceName.c_str(), MessageClasses::JSON_RPC, methodType, data );
    }
    catch( const MaxMessageLengthError& ex ) {
        LOGGER_ERROR( "Send message error: " + ex.what() );
        return Errors::get( Errors::BUS_MESSAGE_TOO_LONG );
    }
    catch( std::exception& ex ) {
        LOGGER_ERROR( "Send message error: " + ex.what() );
        return Errors::get( Errors::BUS_INTERNAL_ERROR );
    }

    return JSON_EMPTY_OBJ;
}

JsonObject Service::sendRequest( const std::string& serviceName,
                                 MessageSubClasses requestType,
                                 const JsonObject& request ) {
    return sendRequest( serviceName, request, requestType );
}

const IDbClient& Service::getDBClient() const {
    assert( m_dbClient );
    return *m_dbClient;
}

IDbClient& Service::getDBClient() {
    assert( m_dbClient );
    return *m_dbClient;
}

IDbClient& Service::getDBClientSafe() {
    createDatabase();
    return *m_dbClient;
}

const std::string& Service::getName() const {
    return m_name;
}

const std::string& Service::getQualifiedName() const {
    return getName();
}

std::string Service::generateMessageUUID() {
    return db::DbId::genOID();
}

void Service::sendBroadcast( const MessageSubClasses eventType,
                             const JsonObject& broadcast,
                             const std::string& broadcastGroup ) {
    Services::Message message = { broadcastGroup, EVENT, eventType, broadcast };
    send( message );
}

void Service::setBinaryName( const std::string& name ) {
    m_binaryName = name;
}

int Service::restoreBackup( const std::string& /*tempBackupFolder*/ ) {
    return 0;
}

void Service::handleLogCommand( const Message& message, const std::string& sender ) {
    switch( message.msgSubClass ) {
    case SET_HA_LOG_LEVEL:
        resetLogOptions();
        break;
    default:
        Log::processLogCommand( message.msgSubClass, message.data );
    };
}

void Service::initLogOptions() {
    return resetLogOptions();
}

void Service::resetLogOptions() {
    LOGGER_TRACE( "Reset log options" );
    bool ok = false;
    Hub::SettingsReader reader( getDBClient() );
    if( auto settingLogIndent = reader.findSetting( Resources::HubSettings::LogsIndent ) ) {
        int indent = std::stoi( settingLogIndent->getTokenValue() );
        Log::setLocalIndent( indent );
    }
    else {
        LOGGER_WARN( cc::warn( "Could not load a setting, name: " ) + cc::warn( Resources::HubSettings::LogsIndent ) );
    }
    if( auto settingLogColors = reader.findSetting( Resources::HubSettings::LogsColor ) ) {
        Log::setLocalColors( settingLogColors->getBoolValue() );
    }
    else {
        LOGGER_WARN( cc::warn( "Could not load a setting, name: " ) + cc::warn( Resources::HubSettings::LogsColor ) );
    }
    if( auto settingLogLevel = reader.findSetting( Resources::HubSettings::LogsLevel ) ) {
        std::string token = settingLogLevel->getTokenValue();
        if( token == "WARNING" )
            token = "WARN";

        Log::setLocalLogLevel( Log::toPriorityLevel( token.c_str() ) );
    }
    else {
        LOGGER_WARN( cc::warn( "Could not load a setting, name: " ) + cc::warn( Resources::HubSettings::LogsLevel ) );
    }
}

void Service::handleSystemDateTimeChanged() {
    LOGGER_INFO_RAW( cc::info( "System time changed" ) );
}

void Service::handleRotateLogsBroadcast() {
    LOGGER_INFO_RAW( cc::info( "Logs have been rotated" ) );
    printHubUptime();
    printServiceStartupTime();
    Log::printLogCounter();
}

void Service::printHubUptime() {
    long systemUptime = TimeUtils::getSystemUptime();
    std::string readableUptime = TimeUtils::formatUptime( systemUptime );
    Log::info( cc::info( "Hub uptime: " ) + cc::note( readableUptime ) );
}

void Service::printServiceStartupTime() {
    auto currentTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast< std::chrono::seconds >( currentTime - m_serviceStartupTime );
    std::string readableUptime = TimeUtils::formatUptime( duration.count() );
    Log::info( cc::info( "Service uptime: " ) + cc::note( readableUptime ) );
}

JsonObject Service::handleServiceInfoGet( const JsonObject& request ) {
    try {
        auto currentTime = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast< std::chrono::seconds >( currentTime - m_serviceStartupTime );
        std::string readableUptime = TimeUtils::formatUptime( duration.count() );

        JsonObject answer;
        answer[ "name" ] = getQualifiedName();
        answer[ "status" ] = "running";
        answer[ "uptime" ] = readableUptime;
        answer[ "info" ] = getServiceInfo();
        answer[ "logging" ][ "log_level" ] = Log::toString( static_cast< Log::PriorityLevel >( Log::getLogLevel() ) );
        answer[ "logging" ][ "counters" ] = Log::logCounterToJson();
        return answer;
    }
    catch( const std::exception& ex ) {
        LOGGER_ERROR( "serviceInfoGet exception: " + ex.what() );
        Json result;
        setError( result, Errors::INTERNAL_OPERATION_ERROR, ex.what() );
        return result;
    }
}

JsonObject Service::getServiceInfo() {
    return JSON_EMPTY_OBJ;
}

}  // namespace Services
}  // namespace HubPlatform
