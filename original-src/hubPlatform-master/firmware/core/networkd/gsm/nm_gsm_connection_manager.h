#pragma once

#include "networkd/configuration/nm_config_file_manager.h"

#include <memory>
#include <functional>
#include <vector>

namespace HubPlatform {

namespace Services {
class TaskLoop;
}  // namespace Services

namespace Network {

class GSMInterface;

namespace Mobile {

class InterfaceStateManager;

class ConnectionManager final {
public:
    ConnectionManager( GSMInterface& interface,
                       InterfaceStateManager& interfaceStateManager,
                       Services::TaskLoop& taskLoop,
                       const NetworkConfigFileManager::MobileConfiguration& configuration );
    void start( const std::weak_ptr< GSMInterface >& weakInterface );

    void onConnectionInterrupted( bool probablyFalsePositive = false );
    void onConnectionRestored();

private:
    /* first  - need to schedule
     * second - timeout to schedule */
    using RevivalOperationResult = std::pair< bool, std::chrono::seconds >;

    void handleConnectionInterrupted();
    RevivalOperationResult tryRevive();

    RevivalOperationResult reRegisterRevivalOperation();
    RevivalOperationResult rebootModemRevivalOperation();

private:
    std::weak_ptr< GSMInterface > m_weakInterface;

    GSMInterface& m_interface;
    InterfaceStateManager& m_interfaceStateManager;
    Services::TaskLoop& m_taskLoop;
    const NetworkConfigFileManager::MobileConfiguration& m_configuration;

    using RevivalOperation = std::function< RevivalOperationResult() >;
    std::vector< RevivalOperation > m_revivalOperations;
    std::size_t m_revivalOperationIdx = 0;

    bool m_connectionEstablished = false;
};

}  // namespace Mobile
}  // namespace Network
}  // namespace HubPlatform
