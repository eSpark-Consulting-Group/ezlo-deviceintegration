#pragma once

#include "service_interfaces/service.h"
#include "networkd/network_manager.h"
#include "networkd/remote_interface/network_statistics_settings_manager.h"
#include "networkd/remote_interface/network_remote_interface.h"

#include "cored/remote_interface/cored_interface.h"
#include "http/http.h"
#include "cloud_storage/response_dispatcher.h"
#include <cloud_storage/request_builder.h>

namespace HubPlatform {
namespace Network {
namespace CommunicationTest {

enum class MobileTestConnectionStates { Default = 0, TryToEnable, EnabledForTest };
using HttpRequestCallback = std::function< void() >;
using ErrorCallback = std::function< void() >;
class ManualCommunicationTest {
public:
    explicit ManualCommunicationTest( Services::Service& service, std::shared_ptr< Network::NetworkManager > manger );
    Json process( const std::string& interface, const std::string operationId, int timeout );
    void onNetworkChanged( Json& message );
    static bool isCloudControllerSignalingGood( int responseStatus, int responseComplete );
    static Json createBroadcastPayload( const std::string& operationId,
                                        const std::string& status,
                                        int completed,
                                        Json& errObject );
    bool isTestStarted();

private:
    void sendTestCommunication( const std::string operationId,
                                const std::string& interfaceId,
                                Services::Service::ReceiveCallback callback,
                                std::chrono::seconds timeout );

    void sendCommunicationProgress( const std::string& operationId,
                                    const std::string& status,
                                    const int completed,
                                    const Json& error );

    void httpDataReceived( const std::string& eventHandler, const CommonHttp::EventPtr& event );

    void httpConnectionClosed( const std::string& eventHandler, const CommonHttp::EventPtr& event );

    void manualTestHttpOptions( CommonHttp::HttpOptions& httpOptions,
                                HubPlatform::Common::CloudStorageInterface::RequestBuilder& builder,
                                const std::string& token );

    void sendBroadcast( const MessageSubClasses eventType,
                        const JsonObject& broadcast,
                        const std::string& broadcastGroup = GroupsNames::BROADCAST_GROUP );

    void sendRequest( const std::string& interfaceId,
                      std::chrono::seconds timeout,
                      CommonHttp::HttpOptions& httpOptions,
                      CommonHttp::Http::RequestId& requestId );

    IDbClient& getDBClient();
    Json getInterfaceInfo( const std::string& interfaceId );
    bool isInternetAvailableForInterface( Json& interface );
    Json checkAndEnableMobileInterface( const std::string& interfaceId );
    bool isEnabledInterface( Json& interface );

    static JsonObject protectControllerSignalsCloudMethod( const Json& params );
    bool isMobileInterface( const std::string& interface );
    void checkInternetConnectionAndSendHttpRequest();
    bool isInternetAvailable( const std::string& interfaceId,
                              const HttpRequestCallback& callback,
                              const ErrorCallback& errorCallback );
    void testRequestToCloud( const std::string& interface,
                             const std::string& operationId,
                             int timeout,
                             std::function< void( const std::string& ) > doneCallback );
    std::string getEnabledState( Json& interface );
    void recoveryToDefaultState();
    void setTestStarted( bool started );
    bool isActiveNetworkInterface( const std::string& interfaceId );

private:
    Services::Service& m_service;
    CommonHttp::Http m_http;
    std::unique_ptr< Common::CloudStorageInterface::Dispatcher > m_dispatcher;
    std::shared_ptr< NetworkManager > m_manager;
    std::string m_interfaceId;
    MobileTestConnectionStates m_mobileConnectionState;
    std::string m_operationId;
    int m_timeout = 0;
    std::string m_savedEnabledState;
    Services::PostponedTaskPtr m_internetCheckTask;
    int m_checkInternetAttempts = 0;
};

}  // namespace CommunicationTest
}  // namespace Network
}  // namespace HubPlatform
