#pragma once

#include <string>
#include <sstream>
#include <cassert>

#include "system_utils/thread_unique_safe_wrapper.h"

namespace tiny {
namespace command_line {
class parser;
}
}  // namespace tiny

namespace HubPlatform {
namespace Uid {

class ServiceSettings {
public:
    enum class ProvisioningState { Unknown, NotRequested, Requested };

public:
    ServiceSettings();

    void defineCommandLineParameters( tiny::command_line::parser& parser );

    bool isNoBus() const;
    size_t mainLoopLimit() const;  // main loop limit
    size_t cntThreadsInDispatcher() const;
    size_t queueLimitInDispatcher() const;
    bool isNoDbUnlock() const;
    int dbMaxCollectionSize() const;
    bool isNoLockedStateCheck() const;
    bool isNoConvertDataitemsToFOs() const;
    bool isNoReplication() const;
    bool isRandomLocalServerPort() const;

    const std::string& getCertificateFile() const;
    const std::string& getPrivateKeyFile() const;
    const std::string& getCertificationChainFile() const;

    std::string getExplicitCloudCoordinatorURL() const;
    std::string getExplicitNmaServerURL() const;

    void setExplicitNmaServerURL( const std::string& url );
    void setExplicitCloudCoordinatorURL( const std::string& url );

    time_t getExplicitCloudCoordinatorQueryTimeoutInSeconds() const;
    time_t getExplicitCloudCoordinatorReconnectPeriodInSeconds() const;

    void setProvisioningDataState( ProvisioningState state );

    ProvisioningState getProvisioningDataState() const;

private:
    size_t m_nMainLoopLimit;  // main loop limit
    size_t m_cntThreadsInDispatcher, m_queueLimitInDispatcher;
    int m_dbMaxCollectionSize;
    bool m_bIsNoBus : 1;
    bool m_isNoDbUnlock : 1;
    bool m_isNoLockedStateCheck : 1;
    bool m_isNoConvertDataitemsToFOs : 1;
    bool m_isNoReplication : 1;
    bool m_bRandomLocalServerPort : 1;

    std::string m_certificateFile;
    std::string m_privateKeyFile;
    std::string m_certificationChainFile;

    time_t m_explicitCloudCoordinatorQueryTimeoutInSeconds;
    time_t m_explicitCloudCoordinatorReconnectPeriodInSeconds;

    threads::UniqueSafeWrapper< std::string > m_explicitCloudCoordinatorURL;
    threads::UniqueSafeWrapper< std::string > m_explicitNmaServerURL;

    ProvisioningState m_provisioningState = ProvisioningState::Unknown;

};  /// class ServiceSettings

}  // namespace Uid
}  // namespace HubPlatform
