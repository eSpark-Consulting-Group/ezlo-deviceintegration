#pragma once

#include "networkd/ssh_tunnel/ssh_tunnel.h"

namespace HubPlatform {
namespace Network {
namespace SshTunnel {

class SshSupportTunnel : public SshTunnel {
public:
    SshSupportTunnel( Services::Service& service, TunnelManager& tunnelManager );

    void createSupportTunnel();
    void createSupportTunnel( int endTime );
    void deleteSupportTunnel();

    void tearDown();

protected:
    bool tunnelEnabled() override;

protected:
    class Scheduler {
    public:
        using Delay = std::chrono::seconds;
        using Task = std::function< void() >;

        Scheduler( Services::Service& service );
        void addTask( Task&& task, const Delay& delay );

    private:
        void executeTask();
        void scheduleTask();

    private:
        using TaskData = std::pair< Task, Delay >;
        Services::Service& m_service;
        std::mutex m_mutex;
        std::list< Scheduler::TaskData > m_queue;
    };

private:
    void createSupportTunnelTask( int endTime );
    void deleteSupportTunnelTask();

    int getSupportEndTime();
    void saveSupportEndTime( int endTime );
    void deleteSupportEndTime();

    Services::PostponedTaskPtr startSupportExpirationTimer( const Seconds& supportExpirationDuration );

private:
    Scheduler m_scheduler;
    Services::PostponedTaskPtr m_supportExpirationTimer;
    const std::string m_supportTunnelSettingsId{ "supportTunnelSettings" };
};

}  // namespace SshTunnel
}  // namespace Network
}  // namespace HubPlatform
