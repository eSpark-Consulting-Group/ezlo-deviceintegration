#include "uid/ui_broadcaster.h"
#include "uid/nma_client.h"
#include "http_bridge/ws_api_bridge.h"

using namespace HubPlatform;

UidBroadcasterPtr& global_uid_broadcaster() {
    static UidBroadcasterPtr g_uidBroadcaster( new uid_broadcaster );
    return g_uidBroadcaster;
}

uid_broadcaster::uid_broadcaster()
: m_uiService( HubPlatform::Uid::Service::getInstance() ) {}

uid_broadcaster::~uid_broadcaster() {}

bool uid_broadcaster::sendMessage( const std::string& msg, int nOpCode ) {
    if( auto nmaClient = HubPlatform::Uid::Service::getInstance().getNmaClient() )
        nmaClient->sendMessage( msg, tiny::ws::client::opcv( nOpCode ) );
    if( m_uiService.getServerListener() ) {
        m_uiService.getServerListener()->postMessageToAllLoggedInPeers( msg, tiny::ws::server::opcv( nOpCode ) );
        m_uiService.getServerListener()->getApiBridge().sendBroadcast( msg );
    }

    m_uiService.getClusterManager().postMessageToAllConnectedControllers( msg, tiny::ws::server::opcv( nOpCode ) );
    return true;
}

bool uid_broadcaster::sendMessageText( const std::string& msg ) {
    if( auto nmaClient = HubPlatform::Uid::Service::getInstance().getNmaClient() )
        nmaClient->sendMessageText( msg );
    if( m_uiService.getServerListener() ) {
        m_uiService.getServerListener()->postMessageToAllLoggedInPeers( msg, tiny::ws::server::opcv::text );
        m_uiService.getServerListener()->getApiBridge().sendBroadcast( msg );
    }

    m_uiService.getClusterManager().postMessageToAllConnectedControllers( msg, tiny::ws::server::opcv::text );
    return true;
}

bool uid_broadcaster::sendMessageBinary( const std::string& msg ) {
    if( auto nmaClient = HubPlatform::Uid::Service::getInstance().getNmaClient() )
        nmaClient->sendMessageBinary( msg );
    if( m_uiService.getServerListener() ) {
        m_uiService.getServerListener()->postMessageToAllLoggedInPeers( msg, tiny::ws::server::opcv::binary );
        m_uiService.getServerListener()->getApiBridge().sendBroadcast( msg );
    }

    m_uiService.getClusterManager().postMessageToAllConnectedControllers( msg, tiny::ws::server::opcv::binary );
    return true;
}

bool uid_broadcaster::postMessage( const std::string& msg ) {
    if( auto nmaClient = HubPlatform::Uid::Service::getInstance().getNmaClient() )
        nmaClient->postMessage( msg, tiny::ws::client::opcv::text );
    if( m_uiService.getServerListener() ) {
        m_uiService.getServerListener()->postMessageToAllLoggedInPeers( msg, tiny::ws::server::opcv::text );
        m_uiService.getServerListener()->getApiBridge().sendBroadcast( msg );
    }

    m_uiService.getClusterManager().postMessageToAllConnectedControllers( msg, tiny::ws::server::opcv::text );
    return true;
}

bool uid_broadcaster::postMessage( const std::string& msg, int nOpCode ) {
    if( auto nmaClient = HubPlatform::Uid::Service::getInstance().getNmaClient() )
        nmaClient->postMessage( msg, tiny::ws::client::opcv( nOpCode ) );
    if( m_uiService.getServerListener() ) {
        m_uiService.getServerListener()->postMessageToAllLoggedInPeers( msg, tiny::ws::server::opcv( nOpCode ) );
        m_uiService.getServerListener()->getApiBridge().sendBroadcast( msg );
    }

    m_uiService.getClusterManager().postMessageToAllConnectedControllers( msg, tiny::ws::server::opcv( nOpCode ) );
    return true;
}

bool uid_broadcaster::sendMessageNoLog( const std::string& msg, int nOpCode ) {
    if( auto nmaClient = HubPlatform::Uid::Service::getInstance().getNmaClient() )
        nmaClient->sendMessageNoLog( msg, tiny::ws::client::opcv( nOpCode ) );
    if( m_uiService.getServerListener() ) {
        m_uiService.getServerListener()->postMessageToAllLoggedInPeersNoLog( msg, tiny::ws::server::opcv( nOpCode ) );
        m_uiService.getServerListener()->getApiBridge().sendBroadcast( msg );
    }

    m_uiService.getClusterManager().postMessageToAllConnectedControllers( msg, tiny::ws::server::opcv( nOpCode ) );
    return true;
}

bool uid_broadcaster::postMessageNoLog( const std::string& msg, int nOpCode ) {
    if( auto nmaClient = HubPlatform::Uid::Service::getInstance().getNmaClient() )
        nmaClient->postMessageNoLog( msg, tiny::ws::client::opcv( nOpCode ) );
    if( m_uiService.getServerListener() ) {
        m_uiService.getServerListener()->postMessageToAllLoggedInPeersNoLog( msg, tiny::ws::server::opcv( nOpCode ) );
        m_uiService.getServerListener()->getApiBridge().sendBroadcast( msg );
    }

    m_uiService.getClusterManager().postMessageToAllConnectedControllers( msg, tiny::ws::server::opcv( nOpCode ) );
    return true;
}
