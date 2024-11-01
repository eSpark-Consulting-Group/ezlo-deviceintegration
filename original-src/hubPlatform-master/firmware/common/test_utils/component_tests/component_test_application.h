#pragma once

#include "test_utils/component_tests/test_application.h"
#include "system_utils/thread_unique_safe_wrapper.h"
#include "http_bridge/ws_api_bridge.h"

#include <unordered_map>
#include <deque>

namespace rpc {
class Message;
}

namespace HubPlatform {
namespace Tests {

class TestServer;
class TestServerPeer;

class ComponentTestApplicationError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class ComponentTestApplication : public TestApplication {
public:
    using HubConnectionPtr = TestServerPeer*;

    using AnswersCollection = std::unordered_map< std::string, std::deque< JsonObject > >;
    using BroadcastsProcessFunction = std::function< void( rpc::Message& ) >;
    using InternalMessageProcessFunction = std::function< void( const Services::Message& ) >;
    using InternalMethodCallProcessFunction =
        std::function< void( const JsonObject& json, MessageSubClasses requestType, const std::string& sender ) >;
    using RequestsProcessFunction = std::function< void( rpc::Message& ) >;
    using CloudProcessFunction = std::function< void( rpc::Message& ) >;
    using CloudMethods = std::map< std::string, CloudProcessFunction >;
    using RequiredGateways = std::set< std::string >;

    enum class ConnectionType { UI, NMA };

public:
    ComponentTestApplication( const std::string& etalonRootFolderPath );

    ~ComponentTestApplication() override;

    static ComponentTestApplication& get();

    HubConnectionPtr getHubPeer();

public:
    void sendtoHub( const std::string& request, ConnectionType connectionType = ConnectionType::UI );

    void sendtoHub( JsonObject request, ConnectionType connectionType = ConnectionType::UI );
    void sendResponseToHub( const JsonObject& request, Json result );

    JsonObject sendtoHubAndGetAnswer( JsonObject request, ConnectionType connectionType = ConnectionType::UI );
    JsonObject sendtoHubAndGetAnswer( const std::string& request, ConnectionType connectionType = ConnectionType::UI );

    std::string assignMessageId( JsonObject& request );
    JsonObject fetchAnswer( const JsonObject& request, const std::string& messageId );

    void setBroadcastsProcessFunction( BroadcastsProcessFunction broadcastsFunction );
    void resetBroadcastsProcessFunction();

    void setInternalBroadcastsProcessFunction( InternalMessageProcessFunction broadcastsFunction );
    void resetInternalBroadcastsProcessFunction();

    void setInternalMethodCallProcessFunction( InternalMethodCallProcessFunction messageFunction );
    void resetInternalMethodCallProcessFunction();

    void setRequestsProcessFunction( RequestsProcessFunction requestsFunction );
    void resetRequestsProcessFunction();

    void setCloudMethod( const std::string& methodName, CloudProcessFunction handler );

    void resetCloudMethod( const std::string& methodName );
    int getNmaPort() const;
    void resetApiBridgeHandlers( HttpBridge::ApiBridgeHandlersPtr handlers );
    virtual RequiredGateways getRequiredGateways() const;

protected:
    virtual void beforeApplicationsStartup();
    virtual void setUpAdditionalApplications() = 0;
    virtual void afterApplicationsStartup();

    virtual bool shouldUseRandomLocalPort();

    void tearDown() override;

private:
    void setUpChildApplications() override;

    bool runCloudMethod( rpc::Message& message );

    void handleInternalBroadcast( const Services::Message& message ) override;
    void handleInternalMethodCall( const JsonObject& json,
                                   MessageSubClasses requestType,
                                   const std::string& sender ) override;

    void handleMessage( rpc::Message& peerMessage );

    void injectApiBridge();

private:
    int m_nmaPort = 0;

    AnswersCollection m_answers;
    std::mutex m_answersMutex;
    std::condition_variable m_newAnswersCondition;

    BroadcastsProcessFunction m_processBroadcastsFunc;
    std::mutex m_broadcastsMutex;

    InternalMessageProcessFunction m_processInternalBroadcastsFunc;
    std::mutex m_internalBroadcastsMutex;

    InternalMethodCallProcessFunction m_processInternalMethodCallFunc;
    std::mutex m_internalMethodCallMutex;

    RequestsProcessFunction m_processRequestsFunc;
    std::mutex m_requestsMutex;

    CloudMethods m_cloudMethods;
    std::mutex m_cloudMutex;

    threads::UniqueSafeWrapper< HubConnectionPtr > m_hubPeer;
    std::unique_ptr< TestServer > m_nmaServer;

    HttpBridge::ApiBridgeHandlersPtr m_apiBridgeHandlers{ nullptr };

    static ComponentTestApplication* m_instance;
};

}  // namespace Tests
}  // namespace HubPlatform
