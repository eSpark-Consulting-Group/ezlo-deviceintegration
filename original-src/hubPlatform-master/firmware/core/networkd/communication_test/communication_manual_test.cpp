#include "networkd/communication_test/communication_manual_test.h"
#include "networkd/network_manager.h"
#include "resources.h"
#include "http/http_events.h"
#include "http/http_options.h"
#include "http/resources.h"

#include "cloud_storage/request_builder.h"
#include "cloud_storage/controller_token.h"
#include "cloud_storage/response_dispatcher.h"
#include "common_utils/streambuf_from_vector.h"
#include "database_client/dbqueryparser.h"

#include "bus/messagesclasses.h"
#include "log_utils/ezlo_log.h"
#include "json_utils/json_utils.h"
#include "jsonrpc/jsonrpc.h"

namespace HubPlatform {
namespace Network {
namespace CommunicationTest {

constexpr std::chrono::seconds InternetConnectionCheckingPeriod( 15 );

ManualCommunicationTest::ManualCommunicationTest( Services::Service& service,
                                                  std::shared_ptr< Network::NetworkManager > manger )
: m_service( service )
, m_manager( manger ) {
    m_dispatcher = std::make_unique< Common::CloudStorageInterface::Dispatcher >();
    auto eventName = CommonHttp::Resources::HttpEvents::HttpDataReceived;
    m_http.on( eventName, [this]( const std::string& eventHandler, const CommonHttp::EventPtr& event ) {
        httpDataReceived( eventHandler, event );
    } );

    m_http.on( CommonHttp::Resources::HttpEvents::HttpConnectionClosed,
               [this]( const std::string& eventHandler, const CommonHttp::EventPtr& event ) {
                   httpConnectionClosed( eventHandler, event );
               } );

    m_mobileConnectionState = MobileTestConnectionStates::Default;
}

IDbClient& ManualCommunicationTest::getDBClient() {
    return m_service.getDBClient();
}

void ManualCommunicationTest::sendBroadcast( const MessageSubClasses eventType,
                                             const JsonObject& broadcast,
                                             const std::string& broadcastGroup ) {
    Services::Message message = { broadcastGroup, EVENT, eventType, broadcast };
    m_service.send( message );
}

void ManualCommunicationTest::httpDataReceived( const std::string& /* eventHandler */,
                                                const CommonHttp::EventPtr& event ) {
    if( event == nullptr )
        return;
    m_dispatcher->dispatchDataReceived( event );
}

void ManualCommunicationTest::httpConnectionClosed( const std::string& /* eventHandler */,
                                                    const CommonHttp::EventPtr& event ) {
    if( event == nullptr )
        return;
    m_dispatcher->dispatchConnectionClosed( event );
}

void ManualCommunicationTest::sendTestCommunication( const std::string operationId,
                                                     const std::string& interfaceId,
                                                     Services::Service::ReceiveCallback callback,
                                                     std::chrono::seconds timeout ) {
    using namespace Common::CloudStorageInterface;
    CommonHttp::HttpOptions httpOptions;
    CommonHttp::Http::RequestId requestId;
    sendRequest( interfaceId, timeout, httpOptions, requestId );

    auto onRequestFinished = [callback]( const Common::CloudStorageInterface::HttpResponse& response ) {
        auto httpStatusCode = response.getHttpStatus();

        auto returnErrorStatus = [callback, httpStatusCode]() {
            Json answer = JSON_EMPTY_OBJ;
            answer[ "code" ] = httpStatusCode;
            if( callback ) {
                callback( answer );
            }
        };

        auto status = response.getHttpStatusText();
        auto error = response.getErrorReasonText();
        auto code = response.getHttpStatus();
        Json responseBody;

        try {
            auto body = response.getBody();
            CommonUtils::streambufFromVector< char > streamBuf( body );
            std::istream is( &streamBuf );

            try {
                responseBody = Json::parse( is );
            }
            catch( const std::exception& ex ) {
                returnErrorStatus();
                Log::error( cc::error( "Couldn't parse json from response body: " ) + ex.what() );
                return;
            }
        }
        catch( const std::exception& ex ) {
            Log::error( cc::error( "Get response body failed: " ) + ex.what() );
            returnErrorStatus();
            return;
        }
        catch( ... ) {
            Log::error( cc::error( "Get response body failed: ..." ) );
            returnErrorStatus();
            return;
        }

        if( callback ) {
            Json answer;
            answer[ "code" ] = code;
            if( code == 200 ) {
                answer[ "data" ] = responseBody;
            }
            if( !error.empty() ) {
                answer[ "error" ] = error;
            }
            if( !status.empty() ) {
                answer[ "status" ] = status;
            }

            callback( answer );
        }
    };

    m_dispatcher->registerRequest( requestId, httpOptions.userData, onRequestFinished, nullptr );
}

void ManualCommunicationTest::sendRequest( const std::string& interfaceId,
                                           std::chrono::seconds timeout,
                                           CommonHttp::HttpOptions& httpOptions,
                                           CommonHttp::Http::RequestId& requestId ) {
    Common::CloudStorageInterface::RequestBuilder builder;
    std::string token;

    using namespace Common::CloudStorageInterface;
    ControllerToken controllerToken;
    ControllerToken::fetchDbRecord( getDBClient(), controllerToken );
    manualTestHttpOptions( httpOptions, builder, controllerToken.getTokenValue() );

    Json params = JSON( "signal"
                        << "hub_connection_check" );
    auto data = protectControllerSignalsCloudMethod( params );
    httpOptions.data = data.dump();
    httpOptions.userData = data;
    httpOptions.interface = interfaceId;
    httpOptions.timeoutMs = timeout.count() * 1000;

    LOGGER_DEBUG_RAW( "Manual communication test cloud command:" + data.dump() );

    requestId = m_http.request( "network_service", httpOptions );
}

JsonObject ManualCommunicationTest::protectControllerSignalsCloudMethod( const Json& params ) {
    return JSON( "call"
                 << "protect_controller_signals"
                 << "params" << params );
}

void ManualCommunicationTest::manualTestHttpOptions( CommonHttp::HttpOptions& httpOptions,
                                                     Common::CloudStorageInterface::RequestBuilder& builder,
                                                     const std::string& token ) {
    httpOptions.verbose = Log::getLogLevel() == Log::TRACE;
    Common::CloudStorageInterface::ControllerToken controllerToken;
    Common::CloudStorageInterface::ControllerToken::fetchDbRecord( m_service.getDBClient(), controllerToken );
    const Common::CloudStorageInterface::Endpoints& endpoints = controllerToken.getEndpoints();
    auto xapp = controllerToken.getXAppValue();
    std::string selectedUrl;
    endpoints.selectCloudApiUrl( selectedUrl );

    if( selectedUrl.empty() ) {
        endpoints.selectUploadRedirectFinishedUrl( selectedUrl );
    }

    auto testUrl = selectedUrl + "/v1/request";
    httpOptions.url = testUrl;
    httpOptions.verbose = true;
    httpOptions.headers.insert( { "xapp", xapp } );
    httpOptions.headers.insert( { "expect", "" } );
    httpOptions.handler = "empty";
    builder.httpOptionsAuth( httpOptions, token );
    auto httpMethod = "POST";
    auto contentType = "application/json";
    httpOptions.type = httpMethod;
    httpOptions.handler = "empty";
    httpOptions.contentType = contentType;
    httpOptions.redirection = 3;
    httpOptions.headers.insert( { "transfer-encoding", "chunked" } );
}

void ManualCommunicationTest::sendCommunicationProgress( const std::string& operationId,
                                                         const std::string& status,
                                                         const int completed,
                                                         const Json& error ) {
    Json errObject = error;
    if( error.empty() ) {
        errObject = nullptr;
    }

    Json payload = createBroadcastPayload( operationId, status, completed, errObject );

    sendBroadcast( NETWORK_COMMUNICATION_TEST_PROGRESS,
                   JSON( "operationId" << operationId << "status" << status << "completed" << completed << "error"
                                       << errObject ) );
}

Json ManualCommunicationTest::createBroadcastPayload( const std::string& operationId,
                                                      const std::string& status,
                                                      int completed,
                                                      Json& errObject ) {
    return JSON( "operationId" << operationId << "status" << status << "completed" << completed << "error"
                               << errObject );
}

Json ManualCommunicationTest::getInterfaceInfo( const std::string& interfaceId ) {
    auto interface =
        m_service.getDBClient().findOne( TempNetworkInterfaces, JSON( db::ID_FIELD_NAME << interfaceId ), nullptr );
    return interface;
}

std::string ManualCommunicationTest::getEnabledState( Json& interface ) {
    return json_get_str( interface, "enabled" );
}

bool ManualCommunicationTest::isEnabledInterface( Json& interface ) {
    auto status = getEnabledState( interface );
    return status != "off";
}

bool ManualCommunicationTest::isInternetAvailableForInterface( Json& interface ) {
    return json_get_bool( interface, "internetAvailable" );
}

JsonObject ManualCommunicationTest::checkAndEnableMobileInterface( const std::string& interfaceId ) {
    using namespace Network;
    auto info = getInterfaceInfo( interfaceId );
    m_mobileConnectionState = MobileTestConnectionStates::Default;

    Json errorMessage = JSON_EMPTY_OBJ;
    bool isEnabled = isEnabledInterface( info );

    if( !isEnabled ) {
        sendCommunicationProgress(
            m_operationId, "progress", ProgressIndicator::TryToEnabledInterface, JSON_EMPTY_OBJ );
        m_mobileConnectionState = MobileTestConnectionStates::TryToEnable;
        m_savedEnabledState = getEnabledState( info );
        m_manager->setEnablePolicy( interfaceId, "on" );
    }
    else {
        checkInternetConnectionAndSendHttpRequest();
    }

    return errorMessage;
}

bool ManualCommunicationTest::isMobileInterface( const std::string& interface ) {
    return interface.rfind( "mobile", 0 ) == 0;
}

void ManualCommunicationTest::setTestStarted( bool started ) {
    getDBClient().clear( TempNetworkManualCommunicationTestStatus );
    getDBClient().insert( TempNetworkManualCommunicationTestStatus, JSON( InternalStatus::Started << started ) );

    auto statusObject =
        this->getDBClient().findOne( TempNetworkManualCommunicationTestStatus, JSON_EMPTY_OBJ, nullptr );
}

bool ManualCommunicationTest::isActiveNetworkInterface( const std::string& interfaceId ) {
    auto interface =
        this->getDBClient().findOne( TempNetworkInterfaces, JSON( db::ID_FIELD_NAME << interfaceId ), nullptr );
    return json_get_str( interface, "status" ) == "up";
}

JsonObject ManualCommunicationTest::process( const std::string& interface,
                                             const std::string operationId,
                                             int timeout ) {
    JsonObject response = JSON_EMPTY_OBJ;

    setTestStarted( true );

    m_interfaceId = interface;
    m_operationId = operationId;
    m_timeout = timeout;
    m_service.addTask( [this, interface, operationId, timeout]() {
        sendCommunicationProgress( operationId, "started", ProgressIndicator::TestStarted, JSON_EMPTY_OBJ );
        if( isMobileInterface( interface ) ) {
            checkAndEnableMobileInterface( interface );
        }
        else {
            testRequestToCloud( interface, operationId, timeout, nullptr );
        }
    } );

    return response;
}

bool ManualCommunicationTest::isCloudControllerSignalingGood( int responseStatus, int responseComplete ) {
    return responseStatus == 1 && responseComplete == 1;
}

void ManualCommunicationTest::testRequestToCloud( const std::string& interface,
                                                  const std::string& operationId,
                                                  int timeout,
                                                  std::function< void( const std::string& ) > doneCallback ) {
    sendCommunicationProgress( operationId, "progress", ProgressIndicator::SendHTTPRequest, JSON_EMPTY_OBJ );
    try {
        sendTestCommunication( operationId,
                               interface,
                               [this, operationId, doneCallback, interface]( const JsonObject& message ) {
                                   int code = json_get_int( message, "code" );
                                   Json responseData = JSON_EMPTY_OBJ;
                                   json_get( message, "data", responseData );
                                   Json error;
                                   auto status = "finished";
                                   Json errorMessage = JSON_EMPTY_OBJ;
                                   if( code == 0 ) {
                                       status = "failed";
                                       setError( errorMessage, Errors::NETWORK_TEST_CONNECTION_ERROR );
                                   }
                                   else if( code == 200 ) {
                                       int responseStatus = json_get_int( responseData, "status" );
                                       int responseComplete = json_get_int( responseData, "complete" );
                                       LOGGER_DEBUG_RAW( "Manual communication test result: " + cc::j( responseData ) );
                                       LOGGER_DEBUG_RAW( "status: " + cc::num10( responseStatus ) );
                                       LOGGER_DEBUG_RAW( "complete: " + cc::num10( responseComplete ) );
                                       if( !isCloudControllerSignalingGood( responseStatus, responseComplete ) ) {
                                           if( !responseData.empty() ) {
                                               status = "failed";
                                               Json data;
                                               json_get( responseData, "data", data );
                                               setError( errorMessage,
                                                         Errors::NETWORK_TEST_RETURN_STATUS_ERROR,
                                                         "Cloud request failed: " + data.dump() );
                                           }
                                           LOGGER_DEBUG_RAW( "Manual communication test failed" );
                                       }
                                       else {
                                           LOGGER_DEBUG_RAW( "Manual communication test successful" );
                                       }
                                   }
                                   else {
                                       status = "failed";
                                       setError( errorMessage, Errors::NETWORK_TEST_BAD_REQUEST_ERROR );
                                   }

                                   sendCommunicationProgress(
                                       operationId, status, ProgressIndicator::TestCompleted, errorMessage[ "error" ] );
                                   if( doneCallback ) {
                                       doneCallback( interface );
                                   }
                                   setTestStarted( false );
                               },
                               std::chrono::seconds( timeout ) );
    }
    catch( ... ) {
        Json errorMessage;
        setError( errorMessage, Errors::NETWORK_TEST_BAD_REQUEST_ERROR );
        sendCommunicationProgress( operationId, "failed", ProgressIndicator::TestCompleted, errorMessage[ "error" ] );
        setTestStarted( false );
    }
}

bool ManualCommunicationTest::isInternetAvailable( const std::string& interfaceId,
                                                   const HttpRequestCallback& testCallback,
                                                   const ErrorCallback& errorCallback ) {
    m_internetCheckTask =
        m_service.createPostponedTask( [interfaceId, testCallback, errorCallback, this]( Services::PostponedTask& ) {
            if( m_checkInternetAttempts >= 4 ) {
                m_checkInternetAttempts = 0;

                if( errorCallback ) {
                    errorCallback();
                }

                m_internetCheckTask->stop();
            }

            auto info = getInterfaceInfo( interfaceId );
            if( isInternetAvailableForInterface( info ) ) {
                m_checkInternetAttempts = 0;
                if( testCallback ) {
                    testCallback();
                }

                m_internetCheckTask->stop();
            }

            m_checkInternetAttempts++;
        } );

    m_checkInternetAttempts = 0;
    m_internetCheckTask->start( InternetConnectionCheckingPeriod, true );

    return false;
}

void ManualCommunicationTest::recoveryToDefaultState() {
    m_manager->setEnablePolicy( m_interfaceId, m_savedEnabledState );
    m_mobileConnectionState = MobileTestConnectionStates::Default;
    setTestStarted( false );
}

void ManualCommunicationTest::checkInternetConnectionAndSendHttpRequest() {
    isInternetAvailable(
        m_interfaceId,
        [this]() {
            testRequestToCloud( m_interfaceId, m_operationId, m_timeout, [this]( const std::string& interfaceId ) {
                recoveryToDefaultState();
            } );
        },
        [this]() {
            Json errorMessage = JSON_EMPTY_OBJ;
            setError( errorMessage, Errors::NETWORK_TEST_CONNECTION_ERROR );
            sendCommunicationProgress( m_operationId, "failed", ProgressIndicator::TestCompleted, errorMessage );
            recoveryToDefaultState();
        } );
}

void ManualCommunicationTest::onNetworkChanged( Json& message ) {
    if( m_mobileConnectionState == MobileTestConnectionStates::TryToEnable ) {
        JsonArray interfaces;
        json_get_arr( message, "interfaces", interfaces );

        for( auto interface : interfaces ) {
            auto interfaceId = json_get_str( interface, db::ID_FIELD_NAME );
            if( isMobileInterface( interfaceId ) ) {
                if( interfaceId == m_interfaceId ) {
                    std::string enabled = json_get_str( interface, "enabled" );
                    m_mobileConnectionState = MobileTestConnectionStates::EnabledForTest;
                    sendCommunicationProgress(
                        m_operationId, "progress", ProgressIndicator::InterfaceIsEnabled, JSON_EMPTY_OBJ );
                    checkInternetConnectionAndSendHttpRequest();
                }
            }
        }

        if( m_mobileConnectionState != MobileTestConnectionStates::EnabledForTest ) {
            Json errorMessage = JSON_EMPTY_OBJ;
            setError( errorMessage, Errors::NETWORK_TEST_CONNECTION_ERROR );
            sendCommunicationProgress( m_operationId, "failed", ProgressIndicator::TestCompleted, errorMessage );
            recoveryToDefaultState();
        }
    }
}

}  // namespace CommunicationTest
}  // namespace Network
}  // namespace HubPlatform
