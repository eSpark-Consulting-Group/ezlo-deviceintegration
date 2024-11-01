#include "networkd/ssh_tunnel/ssh_tunnel.h"
#include "networkd/ssh_tunnel/ssh_state_init.h"
#include "json_utils/json_utils.h"
#include "database_client/dbid.h"
#include "jsonrpc/jsonrpc.h"
#include "base_utils/ezlo_common.h"
#include "base_utils/random.h"
#include "provision_data_utils/provision_data_manager.h"
#include "log_utils/logsettings.h"

/// @brief State schema
///                            Init
///                              |
///                              ↓
///              _          WaitStreams  ____________
///             / \       /             \            \
///             \  ↘     ↙               ↘           |
///        ____→ RequestToken ←-----→ StopTunnels    |
///       /            ↑                  |          |
///       |            ↓                  ↓          |
///       |      WaitTimeout  ←-----  StartTunnels   |
///       |             ↖                ↑           |
///       \              \              ↙            |
///        \____________→ TunnelsCreated  ←__________/

namespace HubPlatform {
namespace Network {
namespace SshTunnel {

class Exception : public std::exception {
public:
    Exception( const std::string& message )
    : m_message( message ) {}
    virtual const char* what() const noexcept {
        return m_message.c_str();
    }

private:
    std::string m_message;
};

constexpr SshTunnel::Seconds SshTunnel::ExpirationDateShift;
constexpr SshTunnel::Seconds SshTunnel::KeepAliveTimeout;
constexpr SshTunnel::Seconds SshTunnel::WaitTunnelsStoppingTimeout;

SshTunnel::SshTunnel( Services::Service& service, TunnelManager& tunnelManager, const std::string& method )
: m_service( service )
, m_dbClient( service.getDBClient() )
, m_tunnelManager( tunnelManager )
, m_method( method ) {
    dbStopAllTunnel();
}

void SshTunnel::initTunnels() {
    m_state = std::make_unique< InitState >( *this );
    m_state->init();
}

void SshTunnel::startStoredTunnels() {
    auto tunnelsData = dbGetTunnelsData();
    for( auto& data : tunnelsData ) {
        startTunnel( data );
    }
}

void SshTunnel::requestTunnel( const std::string& method,
                               const IdentitiesData& identities,
                               const Links& rejectedLinks ) {
    // Remove cached tunnels because they're bound to the old token
    m_removedTunnels.clear();
    m_service.sendAsyncRequest(
        GroupsNames::UI_GROUP,
        createRequestMessage( method, identities, rejectedLinks ),
        CLOUD_MESSAGE,
        [this, identities, method]( const JsonObject& result ) {
            try {
                if( hasError( result ) ) {
                    Log::error( "Error response from cloud: " + cc::j( result[ "error" ] ) );
                    m_state->onInvalidTokenFromCloud();
                    return;
                }
                auto tokenData = getTokenData( result );
                m_expirationTimer =
                    startTokenExpirationTimer( tokenData.expirationDate - getNow() - ExpirationDateShift );
                auto tunnelsData = getTunnelsData( result, identities, tokenData.token );
                dbSaveTunnelData( method, tokenData, tunnelsData );
                m_state->onTokenIsReceived( tunnelsData );
            }
            catch( const std::exception& ex ) {
                Log::error( cc::error( std::string( "Failure during cloud response handling: " ) + ex.what() ) );
                m_state->onInvalidTokenFromCloud();
            }
        },
        [this, identities, method]() {
            Log::warn( cc::warn( "Cloud response getting is timed out" ) );
            m_state->onInvalidTokenFromCloud();
        },
        std::chrono::seconds( 10 ) );
}

std::string SshTunnel::logFileName( const TunnelData& tunnelData ) const {
    auto serverAddress = tunnelData.data.serverAddress;
    auto serverPort = std::to_string( tunnelData.data.serverPort );
    auto serverUser = tunnelData.data.serverUser;
    auto localPort = std::to_string( tunnelData.data.ownPort );
    auto remotePort = std::to_string( tunnelData.data.remotePort );
    auto identity = tunnelData.identity;

    std::string fileName = "ssh_tunnel." + identity + ".log";
    std::string logFile = ezlo::log_settings::getLogDirectory() + fileName;
    return logFile;
}

bool SshTunnel::startTunnel( const TunnelData& tunnelData ) {
    try {
        Log::info( cc::info( "Ssh tunnel creating: " ) + cc::note( tunnelData.identity ) );
        setStatus( tunnelData.identity, Status::Connecting );
        Log::debug( cc::debug( "Ssh tunnel url: " + cc::note( tunnelData.data.serverAddress ) +
                               ", server port: " + cc::note( std::to_string( tunnelData.data.serverPort ) ) +
                               ", remote port: " + cc::note( std::to_string( tunnelData.data.remotePort ) ) ) +
                    ", local port: " + cc::note( std::to_string( tunnelData.data.ownPort ) ) );
        using TunnelStatus = Process::Forker::Status;
        auto processId = m_tunnelManager.createSshTunnel(
            tunnelData.data, [this, tunnelData]( pid_t, TunnelStatus status, int exitCode, const std::string& error ) {
                Log::trace( cc::trace( "Ssh tunnel is closed: " + tunnelData.identity +
                                       ", with status: " + std::to_string( (int) status ) ) +
                            ", exitCode: " + std::to_string( exitCode ) + " and reason: " + error );
                m_service.addTask( [this, tunnelData, status, exitCode, error]() {
                    setStatus( tunnelData.identity, Status::Disconnected );
                    auto it = m_tunnels.find( tunnelData.identity );
                    if( it == m_tunnels.end() ) {
                        Log::debug( cc::debug( "Ssh tunnel was removed: " ) + cc::note( tunnelData.identity ) );
                        m_state->onTunnelWasRemoved( tunnelData );
                        return;
                    }
                    auto& tunnel = it->second;
                    if( tunnel.state == TunnelState::Stopped ) {
                        Log::debug( cc::debug( "Ssh tunnel was stopped: " ) + cc::note( tunnelData.identity ) );
                        m_state->onTunnelWasClosed( tunnelData );
                        return;
                    }
                    tunnel.state = TunnelState::Stopped;
                    m_dbClient.remove( TempTunnelProcessIdsDBName, JSON( "_id" << tunnelData.identity ) );
                    if( status == TunnelStatus::Ok && ( exitCode == 5 || exitCode == 1 || exitCode == 255 ) ) {
                        Log::warn( cc::warn( "Ssh tunnel was rejected by cloud: " ) + cc::note( tunnelData.identity ) );
                        setStatus( tunnelData.identity, Status::Rejected );
                        m_state->onTunnelIsRejectedByCloud( tunnelData );
                    }
                    else if( status == TunnelStatus::Ok && exitCode == 0 ) {
                        Log::warn( cc::warn( "No connection with server, ssh tunnel was timed out: " ) +
                                   cc::note( tunnelData.identity ) );
                        m_state->onTunnelIsTimedOut( tunnelData );
                    }
                    else {
                        Log::error( cc::error( "Ssh tunnel was closed by unknown reason: " ) +
                                    cc::note( tunnelData.identity ) + cc::error( ", status: " ) +
                                    cc::note( std::to_string( (int) status ) ) + cc::error( ", exitCode: " ) +
                                    cc::note( std::to_string( exitCode ) ) + cc::error( ", reason: " ) +
                                    cc::note( error ) );
                        m_state->onTunnelIsClosedByUnknownReason( tunnelData );
                    }
                } );
            } );
        m_dbClient.insert( TempTunnelProcessIdsDBName,
                           JSON( "_id" << tunnelData.identity << "identity" << tunnelData.identity << "processId"
                                       << processId << "method" << m_method ) );
        auto it = m_tunnels.find( tunnelData.identity );
        if( it != m_tunnels.end() ) {
            setStatus( tunnelData.identity, Status::Connected );
            it->second.state = TunnelState::Running;
            it->second.processId = processId;
        }
        else {
            setStatus( tunnelData.identity, Status::Failed );
            Log::error( cc::error( "tunnel not found" ) );
        }
        Log::info( cc::info( "Ssh tunnel is created: " ) + cc::note( tunnelData.identity ) );
    }
    catch( const std::exception& ex ) {
        Log::error( "tunnel to " + tunnelData.data.serverAddress + ":" + std::to_string( tunnelData.data.remotePort ) +
                    " not created: " + ex.what() );
        setStatus( tunnelData.identity, Status::Failed );
        return false;
    }
    return true;
}

std::chrono::seconds SshTunnel::getTimeout() const {
    if( m_isInternetAvailable ) {
        return InternetAvailable::TryConnectionTimeout;
    }
    else {
        return InternetNotAvailable::TryConnectionTimeout;
    }
}

TunnelsData SshTunnel::getTunnelsData( const Json& cloudResponse,
                                       const IdentitiesData& identities,
                                       const std::string& token ) const {
    auto& endpoints = cloudResponse[ "endpoints" ];
    auto& ssh = endpoints[ "ssh" ];
    if( ssh.empty() ) {
        throw Exception( "empty endpoints" );
    }
    if( ssh.size() != identities.size() ) {
        Log::debug( cc::debug( "Requested identities are " ) + cc::note( std::to_string( identities.size() ) ) +
                    cc::debug( ", but returned endpoints are " ) + cc::note( std::to_string( ssh.size() ) ) );
    }
    TunnelsData tunnelsData;
    Common::Random random( 0, ssh.size() - 1 );
    for( std::size_t i = 0, j = 0; i < identities.size(); ++i ) {
        TunnelData tunnelData;
        auto tunnelFullLink = json_get_str( ssh[ j ] );
        auto pos = tunnelFullLink.find( ":" );
        if( pos == std::string::npos ) {
            throw Exception( "wrong tunnel link: " + tunnelFullLink );
        }
        tunnelData.identity = identities[ i ].identity;
        tunnelData.data.ownPort = identities[ i ].port;
        tunnelData.data.remotePort = i + 1;
        tunnelData.data.serverAddress = tunnelFullLink.substr( 0, pos );
        tunnelData.data.serverPort = std::stoi( tunnelFullLink.substr( pos + 1 ) );
        tunnelData.data.serverUser = token;
        tunnelData.data.keepAliveTimeoutSec = KeepAliveTimeout.count();
        tunnelsData.push_back( std::move( tunnelData ) );
        if( i < ssh.size() - 1 ) {
            ++j;
        }
        else {
            j = random();
        }
    }
    return tunnelsData;
}

bool SshTunnel::getTunnelPid( const std::string& identity, int& pid ) {
    auto it = m_tunnels.find( identity );
    if( it == m_tunnels.end() )
        return false;
    pid = it->second.processId;
    return true;
}

bool SshTunnel::getStatusStatistics( const std::string& identity, StatusStatistics& statistics ) {
    auto it = m_statusStatistics.find( identity );
    if( it == m_statusStatistics.end() )
        return false;
    statistics = it->second;
    return true;
}

bool SshTunnel::getTunnelCreationTime( const std::string& identity,
                                       std::chrono::steady_clock::time_point& creationTime ) {
    auto it = m_tunnels.find( identity );
    if( it == m_tunnels.end() )
        return false;
    creationTime = it->second.creationTime;
    return true;
}

bool SshTunnel::getTunnelUptime( const std::string& identity, std::chrono::milliseconds& uptime ) {
    auto it = m_tunnels.find( identity );
    if( it == m_tunnels.end() )
        return false;

    auto& tunnel = it->second;
    if( tunnel.status == Status::Connected ) {
        auto now = std::chrono::steady_clock::now();
        uptime = std::chrono::duration_cast< std::chrono::milliseconds >( now - tunnel.creationTime );
    }
    else {
        uptime = tunnel.uptime;
    }
    return true;
}

SshTunnel::TokenData SshTunnel::getTokenData( const Json& cloudResponse ) const {
    TokenData tokenData;
    auto& token = cloudResponse[ "token" ];
    if( token.empty() ) {
        throw Exception( "empty token field" );
    }
    tokenData.token = json_get_str( token, "string" );
    if( tokenData.token.empty() ) {
        throw Exception( "empty string field" );
    }
    tokenData.expirationDate = Seconds( json_get_int( token, "expires_ts" ) );
    if( tokenData.expirationDate.count() == -1 ) {
        throw Exception( "empty expires_ts field" );
    }
    return tokenData;
}

void SshTunnel::dbSaveTunnelData( const std::string& method, const TunnelsData& tunnelsData ) {
    if( tunnelsData.empty() ) {
        m_dbClient.remove( TempNetworkCloudSshTunnelsDBName, JSON( "_id" << m_method ) );
    }
    else {
        auto dbTunnelsData = m_dbClient.findOne( TempNetworkCloudSshTunnelsDBName, JSON( "_id" << m_method ), nullptr );
        dbTunnelsData[ "tunnels" ] = dbSerializeTunnelData( tunnelsData );
        m_dbClient.update( TempNetworkCloudSshTunnelsDBName, JSON_EMPTY_OBJ, dbTunnelsData, true );
    }
    setStatus( Status::Disconnected );
}

void SshTunnel::dbSaveTunnelData( const std::string& method,
                                  const SshTunnel::TokenData& tokenData,
                                  const TunnelsData& tunnelsData ) {
    if( tunnelsData.empty() ) {
        m_dbClient.remove( TempNetworkCloudSshTunnelsDBName, JSON( "_id" << m_method ) );
    }
    else {
        auto& provision = ProvisionDataUtils::getManager().getProvisionData();
        auto data =
            JSON( "_id" << method << "method" << method << "token" << provision.encryptSensitiveData( tokenData.token )
                        << "tokenExpirationDate" << tokenData.expirationDate.count() << "tunnels"
                        << dbSerializeTunnelData( tunnelsData ) );
        m_dbClient.update( TempNetworkCloudSshTunnelsDBName, JSON_EMPTY_OBJ, data, true );
    }
    setStatus( Status::Disconnected );
}

bool SshTunnel::dbForEachTunnel( const std::function< bool( TunnelData ) >& cb ) {
    Json fieldsToReturn = JSON( "tunnels" << 1 << "token" << 1 );
    auto dbTunnelsData =
        m_dbClient.findOne( TempNetworkCloudSshTunnelsDBName, JSON( "_id" << m_method ), &fieldsToReturn );
    auto& provision = ProvisionDataUtils::getManager().getProvisionData();
    auto dbTunnels = JSON_EMPTY_ARR;
    if( json_get_arr( dbTunnelsData, "tunnels", dbTunnels ) ) {
        for( const auto& dbTunnel : dbTunnels ) {
            auto tunnelData = dbDeserializeTunnelData( dbTunnel );
            tunnelData.data.serverUser = provision.decryptSensitiveData( json_get_str( dbTunnelsData, "token" ) );
            if( !cb( std::move( tunnelData ) ) ) {
                return false;
            }
        }
    }
    return true;
}

TunnelsData SshTunnel::dbGetTunnelsData() {
    TunnelsData tunnels;
    dbForEachTunnel( [&tunnels]( TunnelData data ) {
        tunnels.emplace_back( std::move( data ) );
        return true;
    } );
    return tunnels;
}

bool SshTunnel::dbGetTunnelData( const Identity& identity, TunnelData& result ) {
    return !dbForEachTunnel( [&result, &identity]( TunnelData data ) {
        auto entryFound = data.identity == identity;
        if( entryFound ) {
            result = std::move( data );
        }
        return !entryFound;
    } );
}

bool SshTunnel::dbExtractTunnelData( const Identity& identity, TunnelData& result ) {
    TunnelsData newTunnelList;
    bool tunnelFound = false;
    dbForEachTunnel( [&]( TunnelData data ) {
        if( data.identity == identity ) {
            tunnelFound = true;
            result = std::move( data );
        }
        else {
            newTunnelList.emplace_back( std::move( data ) );
        }
        return true;
    } );
    if( tunnelFound ) {
        dbSaveTunnelData( m_method, newTunnelList );
    }
    return tunnelFound;
}

void SshTunnel::dbInsertTunnelData( const Identity& identity, TunnelData data ) {
    auto tunnels = dbGetTunnelsData();
    tunnels.emplace_back( std::move( data ) );
    dbSaveTunnelData( m_method, tunnels );
}

JsonArray SshTunnel::dbSerializeTunnelData( const TunnelsData& tunnels ) {
    JsonArray dbTunnels = JSON_EMPTY_ARR;
    for( const auto& tunnel : tunnels ) {
        dbTunnels.push_back( JSON( "tunnelLink" << tunnel.data.serverAddress << "tunnelPort" << tunnel.data.serverPort
                                                << "remotePort" << tunnel.data.remotePort << "localPort"
                                                << tunnel.data.ownPort << "identity" << tunnel.identity ) );
    }
    return dbTunnels;
}

TunnelData SshTunnel::dbDeserializeTunnelData( const Json& dbTunnel ) {
    TunnelData tunnelData;
    tunnelData.data.ownPort = json_get_int( dbTunnel, "localPort" );
    tunnelData.data.remotePort = json_get_int( dbTunnel, "remotePort" );
    tunnelData.data.serverAddress = json_get_str( dbTunnel, "tunnelLink" );
    tunnelData.data.serverPort = json_get_int( dbTunnel, "tunnelPort" );
    tunnelData.data.keepAliveTimeoutSec = KeepAliveTimeout.count();
    tunnelData.identity = json_get_str( dbTunnel, "identity" );
    return tunnelData;
}

bool SshTunnel::dbIsTokenExpired( const std::string& method ) {
    auto fieldsToReturn = JSON( "tokenExpirationDate" << 1 );
    auto result = m_dbClient.findOne( TempNetworkCloudSshTunnelsDBName, JSON( "_id" << method ), &fieldsToReturn );
    auto expirationDate = Seconds( json_get_int( result, "tokenExpirationDate" ) ) - ExpirationDateShift;
    return getNow() > expirationDate;
}

void SshTunnel::stopAllTunnels() {
    for( auto& tunnelIt : m_tunnels ) {
        auto& tunnel = tunnelIt.second;
        stopTunnel( tunnel );
    }
}

void SshTunnel::stopTunnel( SshTunnel::Tunnel& tunnel ) {
    tunnel.stopTunnel();
}

void SshTunnel::dbStopAllTunnel() const {
    auto fieldsToReturn = JSON( "processId" << 1 );
    auto dbTunnelIds = m_dbClient.findAll( TempTunnelProcessIdsDBName, JSON( "method" << m_method ), &fieldsToReturn );
    for( auto dbId : dbTunnelIds ) {
        auto id = json_get_int( dbId, "processId" );
        m_tunnelManager.killSshTunnel( id );
    }
    m_dbClient.remove( TempTunnelProcessIdsDBName, JSON( "method" << m_method ) );
}

void SshTunnel::removeTunnel( const std::string& identity ) {
    m_state->removeTunnel( identity );
}

bool SshTunnel::restoreTunnelFromCache( const std::string& identity, TunnelData& data ) {
    auto tunnelIter = m_removedTunnels.find( identity );
    if( tunnelIter != m_removedTunnels.end() ) {
        data = std::move( tunnelIter->second.m_tunnelData );
        dbInsertTunnelData( identity, data );
        m_tunnels.emplace( std::make_pair( identity, std::move( tunnelIter->second.m_tunnel ) ) );
        m_removedTunnels.erase( tunnelIter );
        Log::info( cc::info( "Ssh tunnel is restored from cache: " ) + cc::note( identity ) );
        return true;
    }
    return false;
}

void SshTunnel::stopAndCacheTunnel( const std::string& identity ) {
    auto tunnelIter = m_tunnels.find( identity );
    if( tunnelIter != m_tunnels.end() ) {
        Tunnel tunnel = std::move( tunnelIter->second );
        tunnel.stopTunnel();
        m_tunnels.erase( tunnelIter );
        TunnelData data;
        if( dbExtractTunnelData( identity, data ) ) {
            m_removedTunnels.emplace(
                std::make_pair( identity, TunnelCacheEntry{ std::move( tunnel ), std::move( data ) } ) );
            Log::info( cc::info( "Ssh tunnel is moved to cache: " ) + cc::note( identity ) );
        }
    }
}

bool SshTunnel::getTunnelData( const std::string& identity, TunnelData& data, TunnelState& state ) {
    auto tunnelIter = m_tunnels.find( identity );
    if( tunnelIter != m_tunnels.end() ) {
        if( dbGetTunnelData( identity, data ) ) {
            state = tunnelIter->second.state;
            return true;
        }
    }
    return false;
}

void SshTunnel::setInternetAvailability( bool isAvailable ) {
    m_isInternetAvailable = isAvailable;
}

void SshTunnel::setOnStatusChangedCallback( OnStatusChangedCallback callback ) {
    m_statusChangedCallback = std::move( callback );
}

void SshTunnel::onIpAddressChanged() {
    if( m_state )
        m_state->onIpAddressChanged();
}

std::size_t SshTunnel::getTunnelsAmount() const {
    return m_tunnels.size();
}

SshTunnel::Identities SshTunnel::getIdentities() const {
    Identities identities;
    for( const auto& tunnel : m_tunnels ) {
        identities.insert( tunnel.first );
    }
    return identities;
}

void SshTunnel::addNewTunnel( const std::string& identity, int localPort, SshTunnel::TunnelState state ) {
    auto res = m_tunnels.emplace( identity, Tunnel( m_tunnelManager, m_dbClient, identity, state, localPort ) );
    if( !res.second ) {
        Log::warn( cc::warn( "Tunnel is already in handling: " ) + cc::note( identity ) );
    }
}

void SshTunnel::setState( SshTunnel::StatePtr&& state ) {
    m_state = std::move( state );
    m_state->init();
}

Services::PostponedTaskPtr SshTunnel::createTimer( Services::PostponedTaskFunction&& task ) {
    return m_service.createPostponedTask( std::move( task ) );
}

Services::PostponedTaskPtr SshTunnel::startTokenExpirationTimer( const SshTunnel::Seconds& tokenExpirationDuration ) {
    auto timer = m_service.createPostponedTask( [this]( Services::PostponedTask& ) {
        Log::info( cc::info( "Token is expired" ) );
        m_state->onTokenExpired();
        setStatus( Status::Disconnected );
    } );
    timer->start( tokenExpirationDuration, false );
    return timer;
}

Json SshTunnel::createRequestMessage( const std::string& method,
                                      const IdentitiesData& identitiesData,
                                      const Links& rejectedLinks ) {
    JsonArray identities;
    for( const auto& identityData : identitiesData ) {
        identities.push_back( JSON( "identity" << identityData.identity ) );
    }
    Json cloudMessage;
    cloudMessage[ "method" ] = method;
    cloudMessage[ "api" ] = "1";
    cloudMessage[ "params" ] = JSON( "forwards" << identities << "reject" << rejectedLinks );
    return cloudMessage;
}

SshTunnel::Seconds SshTunnel::getNow() const {
    return std::chrono::duration_cast< std::chrono::seconds >( std::chrono::system_clock::now().time_since_epoch() );
}

void SshTunnel::setStatus( const Identity& identity, Status status ) {
    if( m_statusChangedCallback ) {
        m_statusChangedCallback( identity, status );
    }

    dbSaveStatus( status );
    m_statusStatistics[ identity ][ status ]++;

    auto it = m_tunnels.find( identity );
    if( it != m_tunnels.end() ) {
        auto& tunnel = it->second;
        if( status == Status::Connected ) {
            tunnel.creationTime = std::chrono::steady_clock::now();
            tunnel.uptime = std::chrono::milliseconds{ 0 };
        }
        else if( tunnel.status == Status::Connected && status != Status::Connected ) {
            auto now = std::chrono::steady_clock::now();
            tunnel.uptime = std::chrono::duration_cast< std::chrono::milliseconds >( now - tunnel.creationTime );
        }

        tunnel.status = status;
    }
}

void SshTunnel::setStatus( Status status ) {
    for( auto& element : m_tunnels ) {
        auto& identity = element.first;
        setStatus( identity, status );
    }
}

void SshTunnel::dbSaveStatus( Status status ) {
    try {
        const std::string statusValue = toString( status );
        if( !statusValue.empty() ) {
            m_dbClient.update( TempNetworkCloudSshTunnelsDBName,
                               JSON( "_id" << m_method ),
                               JSON( "$set" << JSON( "status" << statusValue ) ) );
        }
    }
    catch( std::exception& e ) {
        Log::warn( cc::warn( "Exception in SshTunnel::dbSaveStatus: " + std::string( e.what() ) ) );
    }
}

SshTunnel::Tunnel::Tunnel( TunnelManager& tunnelManager,
                           IDbClient& dbClient,
                           const Identity& identity,
                           TunnelState state,
                           int port )
: state( state )
, port( port )
, identity( identity )
, tunnelManager( tunnelManager )
, dbClient( dbClient ) {}

SshTunnel::Tunnel::Tunnel( SshTunnel::Tunnel&& other )
: processId( other.processId )
, state( other.state )
, status( other.status )
, creationTime( other.creationTime )
, uptime( other.uptime )
, port( other.port )
, identity( other.identity )
, tunnelManager( other.tunnelManager )
, dbClient( other.dbClient ) {
    other.processId = -1;
    other.state = TunnelState::Stopped;
    other.port = -1;
    other.status = Status::Disconnected;
    other.uptime = std::chrono::milliseconds{ 0 };
}

SshTunnel::Tunnel::~Tunnel() {
    stopTunnel();
}

void SshTunnel::Tunnel::stopTunnel() {
    if( processId > 0 && state == TunnelState::Running ) {
        state = TunnelState::Stopped;
        tunnelManager.killSshTunnel( processId );
        dbClient.remove( TempTunnelProcessIdsDBName, JSON( "identity" << identity ) );
        Log::info( cc::info( "Ssh tunnel is stopped: " ) + cc::note( identity ) );
    }
}

}  // namespace SshTunnel
}  // namespace Network
}  // namespace HubPlatform
