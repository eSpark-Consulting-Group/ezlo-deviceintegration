#pragma once

#include "networkd/ssh_tunnel/ssh_defs.h"
#include "networkd/ssh_tunnel/ssh_state.h"
#include "networkd/ssh_tunnel/ssh_tunnel_manager.h"
#include "service_interfaces/service.h"
#include "json_utils/json_fwd.h"
#include "system_utils/ssh.h"

#include <functional>
#include <map>
#include <mutex>

namespace HubPlatform {
namespace Network {
namespace SshTunnel {

namespace UT {
class SshTunnelTests;
}

namespace InternetAvailable {
constexpr std::chrono::seconds TryConnectionTimeout( 20 );
}

namespace InternetNotAvailable {
constexpr std::chrono::seconds TryConnectionTimeout( 60 );
}

class SshTunnel {
public:
    enum class TunnelState { Running, Stopped };

    using Seconds = std::chrono::seconds;
    using Link = std::string;
    using Links = std::vector< Link >;
    using StatePtr = std::unique_ptr< State >;
    using Identities = std::set< Identity >;
    using OnStatusChangedCallback = std::function< void( const Identity&, Status ) >;
    using StatusStatistics = std::map< Status, size_t >;

    SshTunnel( Services::Service& service, TunnelManager& tunnelManager, const std::string& method );
    virtual ~SshTunnel() = default;

    void initTunnels();

    void startStoredTunnels();

    void removeTunnel( const std::string& identity );

    bool restoreTunnelFromCache( const std::string& identity, TunnelData& data );
    void stopAndCacheTunnel( const std::string& identity );

    bool getTunnelData( const std::string& identity, TunnelData& data, TunnelState& state );
    bool getTunnelPid( const std::string& identity, int& pid );
    bool getStatusStatistics( const std::string& identity, StatusStatistics& statistics );
    bool getTunnelCreationTime( const std::string& identity, std::chrono::steady_clock::time_point& creationTime );
    bool getTunnelUptime( const std::string& identity, std::chrono::milliseconds& uptime );

    void setInternetAvailability( bool isAvailable );
    void setOnStatusChangedCallback( OnStatusChangedCallback callback );

    void onIpAddressChanged();

    std::size_t getTunnelsAmount() const;

    Identities getIdentities() const;

protected:
    static constexpr Seconds ExpirationDateShift = Seconds( 60 );
    static constexpr Seconds KeepAliveTimeout = Seconds( 60 );
    static constexpr Seconds WaitTunnelsStoppingTimeout = Seconds( 10 );

protected:
    using Timers = std::map< Identity, Services::PostponedTaskPtr >;

    struct Tunnel {
        Tunnel( TunnelManager& tunnelManager,
                IDbClient& dbClient,
                const Identity& identity,
                TunnelState state,
                int port );
        Tunnel( const Tunnel& ) = delete;
        Tunnel( Tunnel&& );
        ~Tunnel();
        void stopTunnel();
        pid_t processId = -1;
        TunnelState state = TunnelState::Stopped;
        Status status = Status::Disconnected;
        int port;
        Identity identity;
        std::chrono::steady_clock::time_point creationTime{};
        std::chrono::milliseconds uptime{ 0 };

    private:
        TunnelManager& tunnelManager;
        IDbClient& dbClient;
    };

    struct TokenData {
        std::string token;
        Seconds expirationDate;
    };

    using Tunnels = std::map< Identity, Tunnel >;

    Services::PostponedTaskPtr startTokenExpirationTimer( const Seconds& tokenExpirationDuration );
    Json createRequestMessage( const std::string& method,
                               const IdentitiesData& identities,
                               const Links& rejectedLinks = Links() );

    void requestTunnel( const std::string& method,
                        const IdentitiesData& identities,
                        const Links& rejectedLinks = Links() );
    bool startTunnel( const TunnelData& tunnelData );
    std::chrono::seconds getTimeout() const;

    TunnelsData getTunnelsData( const Json& cloudResponse,
                                const IdentitiesData& identities,
                                const std::string& token ) const;
    TokenData getTokenData( const Json& cloudResponse ) const;

    void dbSaveTunnelData( const std::string& method, const TokenData& tokenData, const TunnelsData& tunnelsData );
    void dbSaveTunnelData( const std::string& method, const TunnelsData& tunnelsData );

    TunnelsData dbGetTunnelsData();
    bool dbForEachTunnel( const std::function< bool( TunnelData ) >& cb );
    bool dbGetTunnelData( const Identity& identity, TunnelData& result );
    bool dbExtractTunnelData( const Identity& identity, TunnelData& result );
    void dbInsertTunnelData( const Identity& identity, TunnelData data );
    void dbStopAllTunnel() const;
    bool dbIsTokenExpired( const std::string& method );
    void dbSaveStatus( Status status );

    TunnelData dbDeserializeTunnelData( const Json& dbTunnel );
    JsonArray dbSerializeTunnelData( const TunnelsData& tunnels );

    void stopAllTunnels();
    void stopTunnel( Tunnel& tunnel );

    void addNewTunnel( const std::string& identity, int localPort, TunnelState state );

    virtual bool tunnelEnabled() = 0;

    void setState( StatePtr&& state );

    Services::PostponedTaskPtr createTimer( Services::PostponedTaskFunction&& task );

    Seconds getNow() const;

    void setStatus( const Identity& identity, Status status );
    void setStatus( Status status );

private:
    std::string logFileName( const TunnelData& tunnelData ) const;

protected:
    Services::PostponedTaskPtr m_expirationTimer;
    Services::Service& m_service;
    IDbClient& m_dbClient;
    Tunnels m_tunnels;

    OnStatusChangedCallback m_statusChangedCallback;

    struct TunnelCacheEntry {
        Tunnel m_tunnel;
        TunnelData m_tunnelData;
    };
    /**
     * Cache container used to prevent unwanted requests of a new token in case if the identity of a tunnel being added
     * had already been used with the current token (i.e. it was removed by camerad and now it stays here).
     * Once a tunnel is put into this container, it lives until a new token is requested.
     */
    std::map< Identity, TunnelCacheEntry > m_removedTunnels;
    std::map< Identity, StatusStatistics > m_statusStatistics;
    bool m_isInternetAvailable = false;
    TunnelManager& m_tunnelManager;
    StatePtr m_state;
    const std::string m_method;

    friend class QueueState;
    friend class WaitStreamsState;
    friend class InitState;
    friend class WaitTimeoutState;
    friend class TunnelsCreatedState;
    friend class UpdateTokenState;
    friend class StartTunnelsState;
    friend class RequestTokenState;
    friend class StopTunnelState;

    friend class UT::SshTunnelTests;
};

}  // namespace SshTunnel
}  // namespace Network
}  // namespace HubPlatform
