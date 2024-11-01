#include "networkd/ssh_tunnel/ssh_support_tunnel.h"

#include "json_utils/json_utils.h"
#include "provision_data_utils/provision_data_manager.h"

namespace HubPlatform {
namespace Network {
namespace SshTunnel {

#if defined( __HUB_TYPE_VHUB__ )
#define DEFAULT_SSH_PORT 50022
#else
#define DEFAULT_SSH_PORT 22
#endif

SshSupportTunnel::SshSupportTunnel( Services::Service& service, TunnelManager& tunnelManager )
: SshTunnel::SshTunnel( service, tunnelManager, "cloud.tunnel_support_coordinator" )
, m_scheduler( service ) {}

void SshSupportTunnel::createSupportTunnel() {
    auto endTime = getSupportEndTime();

    if( m_tunnels.empty() ) {
        if( tunnelEnabled() )
            createSupportTunnel( endTime );
    }
    else {
        m_supportExpirationTimer = startSupportExpirationTimer( Seconds( endTime ) - getNow() );
    }
}

void SshSupportTunnel::createSupportTunnel( int endTime ) {
    deleteSupportTunnel();
    m_scheduler.addTask( [this, endTime]() { createSupportTunnelTask( endTime ); }, Scheduler::Delay( 8 ) );
}

void SshSupportTunnel::createSupportTunnelTask( int endTime ) {
    constexpr int twoWeeks = 2 * 7 * 24 * 60 * 60;

    if( endTime == 0 )
        endTime = static_cast< int >( getNow().count() ) + twoWeeks;

    bool supportExpired = Seconds( endTime ) <= getNow();
    if( supportExpired ) {
        Log::warn( cc::warn( "Support date already expired" ) );
        return;
    }
    saveSupportEndTime( endTime );

    if( !m_tunnels.empty() ) {
        setStatus( Status::Connecting );
        startStoredTunnels();
    }
    else {
        auto& data = ProvisionDataUtils::getManager().getProvisionData();
        auto uuid = data.uuid;

        if( uuid.empty() ) {
            Log::warn( cc::warn( "Cannot read hub UUID" ) );
            setStatus( Status::Failed );
            return;
        }

        std::string identity = "controller-" + uuid;
        setStatus( Status::Connecting );
        m_state->createTunnel( identity, DEFAULT_SSH_PORT );
    }

    m_supportExpirationTimer = startSupportExpirationTimer( Seconds( endTime ) - getNow() );
}

void SshSupportTunnel::deleteSupportTunnel() {
    m_scheduler.addTask( [this]() { deleteSupportTunnelTask(); }, Scheduler::Delay( 5 ) );
}

void SshSupportTunnel::deleteSupportTunnelTask() {
    if( !m_tunnels.empty() ) {
        if( m_supportExpirationTimer )
            m_supportExpirationTimer->stop();

        stopAllTunnels();
        Log::info( cc::info( "Support tunnel stopped" ) );
    }
    setStatus( Status::Disconnected );
    deleteSupportEndTime();
}

int SshSupportTunnel::getSupportEndTime() {
    auto query = JSON( "_id" << m_supportTunnelSettingsId );
    auto result = m_dbClient.findOne( NetworkCloudSshTunnelsSettingsDBName, query, nullptr );
    if( result.empty() )
        return 0;

    int endTime = json_get_int( result, "endTime", 0 );
    if( endTime >= 0 && std::chrono::seconds( endTime ) <= getNow() )
        deleteSupportEndTime();
    return endTime;
}

void SshSupportTunnel::saveSupportEndTime( int endTime ) {
    deleteSupportEndTime();

    if( endTime <= 0 || std::chrono::seconds( endTime ) <= getNow() )
        return;

    auto query = JSON( "_id" << m_supportTunnelSettingsId << "endTime" << endTime );
    m_dbClient.insert( NetworkCloudSshTunnelsSettingsDBName, query );
}

void SshSupportTunnel::deleteSupportEndTime() {
    m_dbClient.remove( NetworkCloudSshTunnelsSettingsDBName, JSON( "_id" << m_supportTunnelSettingsId ) );
}

bool SshSupportTunnel::tunnelEnabled() {
    auto endTime = getSupportEndTime();
    if( endTime <= 0 || std::chrono::seconds( endTime ) <= getNow() )
        return false;
    return true;
}

Services::PostponedTaskPtr SshSupportTunnel::startSupportExpirationTimer(
    const SshTunnel::Seconds& supportExpirationDuration ) {
    auto timer = m_service.createPostponedTask( [this]( Services::PostponedTask& ) {
        Log::info( cc::info( "Support tunnel is expired. Stopping tunnel..." ) );
        stopAllTunnels();
        deleteSupportEndTime();
        setStatus( Status::Disconnected );
    } );
    timer->start( supportExpirationDuration, false );
    return timer;
}

void SshSupportTunnel::tearDown() {
    LOGGER_TRACE( cc::trace( "Delete all tunnel data in a synchronous way" ) );
    deleteSupportTunnelTask();
}

SshSupportTunnel::Scheduler::Scheduler( Services::Service& service )
: m_service( service ) {}

void SshSupportTunnel::Scheduler::addTask( Task&& task, const Delay& delay ) {
    std::lock_guard< std::mutex > lock( m_mutex );
    m_queue.emplace_back( std::move( task ), delay );
    if( m_queue.size() == 1 )
        scheduleTask();
}

void SshSupportTunnel::Scheduler::executeTask() {
    std::unique_lock< std::mutex > lock( m_mutex );
    if( m_queue.empty() ) {
        Log::warn( cc::warn( "SshSupportTunnel task was removed before execution" ) );
        return;
    }
    auto& task = m_queue.front().first;
    lock.unlock();

    task();

    lock.lock();
    m_queue.pop_front();
    if( !m_queue.empty() ) {
        scheduleTask();
    }
}

void SshSupportTunnel::Scheduler::scheduleTask() {
    if( m_queue.empty() )
        return;

    auto& delay = m_queue.front().second;
    m_service.createPostponedTask( [this]( Services::PostponedTask& ) { executeTask(); } )->start( delay, false );
}

}  // namespace SshTunnel
}  // namespace Network
}  // namespace HubPlatform
