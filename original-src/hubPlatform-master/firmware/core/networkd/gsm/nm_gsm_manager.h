#pragma once

#include "networkd/interfaces/nm_gsm_interface.h"
#include "networkd/monitoring/nm_interface_monitor_factory.h"
#include "networkd/monitoring/nm_interface_change_listener.h"
#include "networkd/configuration/nm_config_manager.h"
#include "networkd/configuration/nm_config_file_manager.h"
#include "networkd/gsm/nm_gsm_interface_number_manager.h"

#include "provision_data_utils/provision_data.h"
#include "database_client/idbclient.h"
#include "service_interfaces/task_loop.h"

#include <string>
#include <map>
#include <memory>
#include <queue>
#include <stdexcept>

namespace HubPlatform {
namespace Network {

class InvalidGSMInterface : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

/* isn't thread safe;
 * Note: a gsm modem wired on pcb of h2_secure boards is managed by the same way
 *       as external usb-dongle based gsm modems with the only difference that it can't be unplugged */
class GSMManager final {
public:
    using GSMInterfacePtr = std::shared_ptr< GSMInterface >;

    GSMManager( IDbClient& dbClient,
                const ProvisionDataUtils::ProvisionData& provisionData,
                NetworkConfigManager& configManager,
                const NetworkConfigFileManager::MobileConfiguration& configuration,
                InterfaceMonitorFactory&& interfaceMonitorFactory,
                InterfaceInfoListener& interfaceInfoListener,
                Services::TaskLoop& taskLoop );
    ~GSMManager();

    void start();

    void restoreDynamicInterfaces();

    bool isMobileModem( const std::string& devicePath );
    bool addGSMInterface( const std::string& devicePath );
    void removeGSMInterface( const std::string& devicePath );
    void addModemEventToQueue( const std::string& eventType, const std::string& devicePath );

    bool isGSMInterface( const std::string& interfaceName ) const;

    /* throw an InvalidGSMInterface exception if interface doesn't exist */
    GSMInterface& getGSMInterface( const std::string& interfaceName );
    const GSMInterface& getGSMInterface( const std::string& interfaceName ) const;

    /* reset all settings for all gsm interfaces */
    void resetGSMInterfaces();

    /* iterate over all dynamic interfaces */
    void foreachGSMInterface( const std::function< void( GSMInterface& ) >& callback );
    void foreachGSMInterface( const std::function< void( const GSMInterface& ) >& callback ) const;

    bool noInterfaces() const;
    bool noEmbeddedInterfaces() const;

    void onInterfaceCreated( const std::string& interfaceName,
                             const std::string& devicePath,
                             const std::string& model,
                             bool embedded );
    void onInterfaceDestroyed( const std::string& interfaceName, const std::string& devicePath, bool embedded );
    void onInterfaceCreationFailed( const std::string& devicePath, bool embedded );

    static Json handleGSMErrors( const std::exception& ex );
    JsonObject getModemInfo() const;

protected:
    class EventQueue {
    public:
        using Delay = std::chrono::seconds;
        using Event = std::function< void() >;

        explicit EventQueue( Services::TaskLoop& taskLoop );
        void addEvent( Event&& event, const Delay& delay );

    private:
        void processEvent();
        void scheduleEvent();

    private:
        using EventData = std::pair< Event, Delay >;
        Services::TaskLoop& m_taskLoop;
        std::mutex m_mutex;
        std::list< EventQueue::EventData > m_queue;
    };

private:
    void syncInterfaces();
    void embeddedModemWatchdog();

private:
    IDbClient& m_dbClient;
    const ProvisionDataUtils::ProvisionData& m_provisionData;
    NetworkConfigManager& m_configManager;
    const NetworkConfigFileManager::MobileConfiguration& m_configuration;
    const InterfaceMonitorFactory m_interfaceMonitorFactory;
    InterfaceInfoListener& m_interfaceInfoListener;
    Services::TaskLoop& m_taskLoop;
    Services::TaskLoopTimer m_embeddedModemWatchdog;

    using USBDevicePath = std::string;
    using DynamicGSMInterfaces = std::map< USBDevicePath, GSMInterfacePtr >;
    mutable std::mutex m_interfaceMutex;
    DynamicGSMInterfaces m_dynamicGSMInterfaces;

    Mobile::InterfaceNumberManager m_interfaceNumberManager;

    EventQueue m_eventQueue;
};

}  // namespace Network
}  // namespace HubPlatform
