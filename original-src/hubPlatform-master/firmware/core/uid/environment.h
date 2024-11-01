#pragma once

#include "config_reader/config_reader.h"
#include "provision_data_utils/provision_data.h"
#include "provision_data_utils/update_status.h"
#include "system_utils/thread_unique_safe_wrapper.h"

#include <chrono>
#include <mutex>

class IDbClient;

namespace HubPlatform {
namespace Uid {

class Environment;
using EnvironmentPtr = std::shared_ptr< Environment >;
using EnvironmentWPtr = std::weak_ptr< Environment >;

class ServiceSettings;

class Environment {
private:
    struct PrivateTag {};

public:
    Environment( ServiceSettings& settings, IDbClient& dbClient, PrivateTag );

    Environment( const Environment& ) = delete;

    Environment( Environment&& ) = default;

    static EnvironmentPtr create( ServiceSettings& settings, IDbClient& dbClient );

    std::string getCertificateFile() const;
    std::string getPrivateKeyFile() const;
    std::string getCertificationChainFile() const;

    std::string getProvisionServerURL() const;
    std::string getProvisionToken() const;
    int getProvisionVersion() const;
    std::string getCloudCoordinatorURL() const;
    time_t getCloudCoordinatorQueryTimeout() const;
    time_t getCloudCoordinatorReconnectPeriod() const;
    std::string askCloudCoordinatorForNmaServerURL( std::string& cloudCoordinatorURL );
    std::string askCloudCoordinatorForNmaServerURL();
    std::string getNmaServerURL( bool enableAskCloudCoordinator );
    bool isStopEzloHubEmotionsOnShutdown() const;

    void setExplicitHubSerialNumber( const std::string& explicitHubSerialNumber );
    void setExplicitHubType( const std::string& explicitHubType_ );
    void setExplicitCloudCoordinatorURL( const std::string& explicitCloudCoordinatorURL );
    void setExplicitNmaServerURL( const std::string& explicitNmaServerURL );

    void invalidateNmaServerURL();

    const HubPlatform::ConfigReader& getConfigReader() const;

    std::string getExplicitCloudCoordinatorURL() const;
    time_t getExplicitCloudCoordinatorQueryTimeoutInSeconds() const;
    time_t getExplicitCloudCoordinatorReconnectPeriodInSeconds() const;
    std::string getExplicitNmaServerURL() const;

    void updateNmaUrlFromCache();

    bool hasProvisionInformation() const;
    bool requestProvisionInformation();

    void restart( std::chrono::seconds waitBeforeRestart = std::chrono::seconds( 0 ) ) const;
    void reboot( std::chrono::seconds waitBeforeReboot = std::chrono::seconds( 0 ) ) const;

    ProvisionDataUtils::UpdateStatus requestNewProvisionInformation();
    ProvisionDataUtils::UpdateStatus updateProvisioningData( const ProvisionDataUtils::ProvisionData& provisionData );

    size_t getNmaClientRxBufferSize() const;

private:
    void fillValueIfFileExist( const char* key, std::string& value ) const;

    std::string requestCoordinatorForNewNMAUrl( const std::string& strCloudCoordinatorURL );

    bool isUrlCacheEnabled() const;

    void markProvisioningAsRequested();
    bool wasProvisioningRequestedAfterStart();

private:
    ServiceSettings& m_settings;
    IDbClient& m_dbClient;
    HubPlatform::ConfigReader m_configReader;
    threads::UniqueSafeWrapper< ProvisionDataUtils::ProvisionData > m_provisionData;
    std::mutex m_provisioningRequestMutex;
};

}  // namespace Uid
}  // namespace HubPlatform
