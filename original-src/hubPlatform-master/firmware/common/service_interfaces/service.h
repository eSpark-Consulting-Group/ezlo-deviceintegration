#pragma once

#include "service_interfaces/message.h"
#include "service_interfaces/groups_names.h"
#include "bus/messagesclasses.h"
#include "database_client/idbclient.h"
#include "service_interfaces/task_loop.h"
#include "plugin_events_subscriber/plugin_events_subscriber.h"
#include "system_utils/thread_unique_safe_wrapper.h"
#include "service_interfaces/reporter.h"

#include <atomic>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <future>
#include <unordered_map>
#include <list>
#include <ctime>

class BaseBus;

namespace tiny {
namespace command_line {
class parser;
}
}  // namespace tiny

namespace HubPlatform {
namespace Services {

namespace UT {
class ServiceTestSuite;
}

class BusWrapper;
class ScriptEvent;

class Service : public TaskLoop {
public:
    Service( const std::string& name );
    virtual ~Service();

    void run() final;
    void stop( StoppingMode mode = StoppingMode::Auto ) final;

    void join( const std::string& group );

    using ReceiveCallback = std::function< void( const JsonObject& message ) >;
    using TimeoutCallback = std::function< void() >;
    using RequestUuid = std::string;
    using serviceTime = std::chrono::steady_clock::time_point;

    JsonObject sendRequest( const std::string& serviceName,
                            const JsonObject& request,
                            MessageSubClasses requestType,
                            std::chrono::seconds timeout = std::chrono::seconds( 5 ) );

    JsonObject sendRequest( const std::string& serviceName, MessageSubClasses requestType, const JsonObject& request );

    RequestUuid sendAsyncRequest( const std::string& serviceName,
                                  const JsonObject& request,
                                  MessageSubClasses requestType,
                                  ReceiveCallback receiveCallback,
                                  TimeoutCallback timeoutCallback,
                                  std::chrono::seconds timeout = std::chrono::seconds( 5 ) );

    RequestUuid sendAsyncRequest( const std::string& serviceName,
                                  const JsonObject& request,
                                  MessageSubClasses requestType,
                                  ReceiveCallback receiveCallback,
                                  std::chrono::seconds timeout = std::chrono::seconds( 5 ) );

    JsonObject sendMethodCall( const std::string& serviceName,
                               MessageSubClasses requestType,
                               const JsonObject& dasendAsyncResponseta );

    void sendBroadcast( const MessageSubClasses eventType,
                        const JsonObject& broadcast,
                        const std::string& broadcastGroup = GroupsNames::BROADCAST_GROUP );

    JsonObject sendAsyncResponse( const std::string& serviceName, const JsonObject& response, const std::string& uuid );
    void discardExistingAsyncRequestResponses();

    void sendUnsafe( const Message& message );
    bool send( const Message& message );

    const IDbClient& getDBClient() const;
    IDbClient& getDBClient();
    IDbClient& getDBClientSafe();

    const std::string& getName() const;

    virtual const std::string& getQualifiedName() const;

    const std::string& getHostChannel();

    void setBinaryName( const std::string& name );

    virtual int restoreBackup( const std::string& tempBackupFolder );

    virtual void defineDaemonParameters( tiny::command_line::parser& parser );

    bool isRunning() const noexcept;

    Json subscribe( PluginEventsSubscriber listener );
    void unsubscribe( const PluginEventsSubscriber& listener );

    Json changeSubscriberFilterRules( const PluginEventsSubscriber::ModuleName& moduleName,
                                      const PluginEventsSubscriber::PluginEventsSubsriberId& subscriberId,
                                      const PluginEventsSubscriber::FilterRules& filterRules );

    void emit( const ScriptEvent& event );
    void emit( const std::string& pluginId, const ScriptEvent& event );

    void createDatabase();

    static std::string generateMessageUUID();

    void setReporter( std::unique_ptr< ReporterInterface > reporterInterfacePtr );
    void addReporter( std::unique_ptr< ReporterInterface > reporterInterfacePtr );

public:
    static constexpr const char* const UnsubscribeMethod = "unsubscribe";

protected:
    friend class UT::ServiceTestSuite;

    Service( std::string name,
             std::unique_ptr< BusWrapper > busWrapper,
             std::unique_ptr< IDbClient > dbClient,
             size_t taskThreadCount = 0U );

    virtual void setUp();
    virtual void setUpFinished() {}
    virtual void tearDown();

    virtual void handleMessageFromBus( const Message& message, const std::string& sender );
    virtual void handleLegacyMethodCall( const JsonObject& json, const std::string& sender );
    virtual void handleMethodCall( const JsonObject& json, MessageSubClasses requestType, const std::string& sender );
    virtual JsonObject handleRequestFromBus( const JsonObject& request, MessageSubClasses requestType );
    virtual bool handleAsyncRequestFromBus( const JsonObject& request,
                                            MessageSubClasses requestType,
                                            const std::string& uuid,
                                            const std::string& sender );
    virtual void initLogOptions();
    virtual void resetLogOptions();
    virtual void handleLogCommand( const Message& message, const std::string& sender );

    BaseBus* getBus();

    const std::string& getBinaryName();

    const Json& unhandledRequestAnswer() const;
    virtual void handleSystemDateTimeChanged();

    virtual JsonObject getServiceInfo();

protected:
    using PluginId = std::string;
    using ModuleName = std::string;
    using SubscriberId = std::string;
    using PluginSubscribers = std::unordered_map< SubscriberId, PluginEventsSubscriber >;
    using SubscribersByPluginId = std::unordered_map< PluginId, PluginSubscribers >;
    using SubscribersByModuleName = std::unordered_map< ModuleName, SubscribersByPluginId >;
    threads::UniqueSafeWrapper< SubscribersByModuleName > m_subscribers;

    Services::PostponedTaskPtr m_reportLoop;
    Services::PostponedTaskPtr m_addReporterLoop;

private:
    void processTasks();
    void handleResponseFromBus( const Message& message );
    void handleAsyncResponseFromBus( const std::string& uuid, const Json& response );
    int getId() const;

    virtual void onSubscriberAdded( const PluginId& pluginId ) {}
    void printHubUptime();
    void printServiceStartupTime();
    void handleRotateLogsBroadcast();

    JsonObject handleServiceInfoGet( const JsonObject& request );

private:
    using ServiceIntervalTime = std::chrono::minutes;
    std::string m_name;

    std::unique_ptr< ReporterInterface > m_reporter;
    std::vector< std::unique_ptr< ReporterInterface > > m_reporterArray;

    std::unique_ptr< BusWrapper > m_busWrapper;
    std::unique_ptr< IDbClient > m_dbClient;

    std::mutex m_responsesMapMutex;
    std::unordered_map< std::string, std::promise< JsonObject > > m_responsesMap;

    struct AsyncResponseData {
        ReceiveCallback receiveCallback;
        TimeoutCallback timeoutCallback;
        Services::PostponedTaskPtr timer;
    };

    std::mutex m_asyncResponsesMapMutex;
    std::unordered_map< std::string, AsyncResponseData > m_asyncResponsesMap;

    std::string m_binaryName;

    std::atomic_bool m_isRunning{ false };
    std::chrono::steady_clock::time_point m_serviceStartupTime;
};

}  // namespace Services
}  // namespace HubPlatform
