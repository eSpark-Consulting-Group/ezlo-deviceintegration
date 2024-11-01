#pragma once

#include "service_interfaces/service.h"
#include "uid/server/server_listener.h"
#include "uid/service_settings.h"
#include "uid/environment.h"
#include "uid/icc_manager/icc_manager.h"
#include "uid/cluster_manager/cluster_manager.h"
#include "uid/extensions/ui_extensions_manager.h"
#include "uid/nma_client.h"
#include "reporter/connection_nma_reporter.h"

#include "config_reader/config_reader.h"
#include "tiny/tiny_atomic_shared_ptr.h"

#include <mutex>
#include <map>
#include <chrono>

namespace HubPlatform {
namespace ProvisionDataUtils {
class ProvisionData;
}
namespace Uid {

class CloudStorage;

namespace UT {
class FakeUiService;
}

class Service : public Services::Service {
public:
    Service();

    ~Service() override;

    static Service& getInstance();

    int getServerPort() const;
    int getInsecureServerPort() const;

    void manageServerListener( bool forceRestart = false );
    std::unique_ptr< server_listener >& getServerListener();

    NmaClientPtr getNmaClient();
    InterControllerCommunication::IccManager& getIccManager();
    ClusterManager& getClusterManager();

    EnvironmentPtr getEnvironment();

    void defineDaemonParameters( tiny::command_line::parser& parser ) override;

    void onHubRegistered( bool isHubRegistered );
    bool redirectCloudMessage( rpc::Message& rpcMessage );

    ProvisionDataUtils::UpdateStatus updateAndApplyProvisioningWithDelay(
        const ProvisionDataUtils::ProvisionData& provisioningData,
        const Interval& delay );

    std::unique_lock< std::mutex > lockCertificateFiles();
    tiny::ws::security_args getSecurityArgs();

    int restoreBackup( const std::string& tempBackupFolder ) override;

    struct CustomService {
        std::string m_name;
        std::string m_requestDomain;
        std::string m_spreadServiceName;
        std::chrono::seconds m_requestTimeout;

        Json toJson() const;
    };
    using CustomServiceName = std::string;
    using RequestDomain = std::string;
    const CustomService* matchCustomService( const RequestDomain& requestDomain ) const;

    void onHubRegisterRepeatStarted();
    void onHubRegisterRepeatFinished();
    void registerRequestOfflineCredentialsCall( Services::PostponedTaskPtr fetchOfflineCredentials );

protected:
    void setUp() override;
    void tearDown() override;

    void handleMessageFromBus( const Services::Message& message, const std::string& sender ) override;
    void handleMethodCall( const JsonObject& json, MessageSubClasses requestType, const std::string& sender ) override;

private:
    Service( std::unique_ptr< Services::BusWrapper > busWrapper, std::unique_ptr< IDbClient > dbClient );

    void handleEzloEventFromBus( const Services::Message& message );
    void handleHaExceptionFromBus( const Services::Message& message );
    void handleUiEventFromBus( const Services::Message& message );
    void handleEventFromBus( const Services::Message& message );
    void handleAllUiEventsFromBus( const Services::Message& message );
    void handleSystemInfoFromBus( const Services::Message& message );
    void handleLogCmdFromBus( const Services::Message& message );
    bool handleAsyncRequestFromBus( const JsonObject& request,
                                    MessageSubClasses requestType,
                                    const std::string& uuid,
                                    const std::string& sender ) override;
    JsonObject handleRequestFromBus( const JsonObject& request, MessageSubClasses requestType ) override;

    void setInstance( Service* instance );

    void printFirmwareVersion();
    void printRuntimeOptions();

    bool isNeedToSendInfoEvent( const std::string& message, const std::string& type );
    static bool sendResponseToUis( const MessageSubClasses eventType, const JsonObject& response );

    int getNextCertificateIndex() const;
    int getCurrentCertificateIndex() const;
    void setCurrentCertificateIndex( int idx );
    std::pair< int, int > getTimeUntilCurrentCertificateExpire() const;

    bool startServerListener();
    void stopServerListener();
    void watchForServerListener();

    void applyProvisioningRequirements( ProvisionDataUtils::UpdateStatus status );
    void injectApiBridge();
    void onLocalServerStarted( uint16_t securePort, uint16_t insecurePort );

    void registerCustomServices();
    void printRegisteredCustomServices( std::ostream& os ) const;
    int checkRotateLogsStatus();
    int resetRotateLogsStatus();
    virtual JsonObject getServiceInfo() override;

private:
    ServiceSettings m_settings;

    NmaClientPtr m_nmaClient;
    InterControllerCommunication::IccManager m_iccManager;
    ClusterManager m_clusterManager;
    Extensions::ExtensionsManager m_uiExtensionsManager;
    EnvironmentPtr m_environment;
    std::unique_ptr< server_listener > m_server;
    ConfigReader m_configReader;
    std::mutex m_mutex;
    std::mutex m_certificateFilesMutex;
    std::string m_previousSystemInfoMessage;
    std::unique_ptr< CloudStorage > m_cloudStorage;
    std::map< CustomServiceName, CustomService > m_customServices;
    std::unique_ptr< ConnectionNmaReporter > m_connectionReporter;
    Services::PostponedTaskPtr m_fetchOfflineCredentials;  //!< Timer to request access keys

    friend class UT::FakeUiService;

    static Service* m_instance;
};

}  // namespace Uid
}  // namespace HubPlatform
