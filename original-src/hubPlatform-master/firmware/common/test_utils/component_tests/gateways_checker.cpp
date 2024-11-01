#include "test_utils/component_tests/gateways_checker.h"
#include "database_client/db_tables_names.h"
#include <gtest/gtest.h>

namespace HubPlatform {
namespace Tests {

GatewaysChecker::GatewaysChecker( IDbClient& dbClient )
: m_dbClient( dbClient ) {}

void GatewaysChecker::checkGatewayReady( const std::set< std::string >& pluginIds ) {
    auto waitForBroadcast = [this]( std::chrono::seconds timeout, const std::string& errorMessage ) {
        std::unique_lock< std::mutex > lock( m_broadcastsMutex );
        bool success = m_broadcastsCondVar.wait_for( lock, timeout, [this] { return !m_broadcastMessages.empty(); } );
        if( !success ) {
            LOGGER_ERROR( errorMessage );
            throw std::runtime_error( errorMessage );
        }
        m_broadcastMessages.pop_front();
    };

    auto timeout = std::chrono::seconds( 40 );
    while( true ) {
        auto allGatewaysIsReady = true;
        auto fieldsToReturn = JSON( "pluginId" << 1 << "status" << 1 );
        auto gateways = m_dbClient.findAll( HaGatewaysDBName, JSON_EMPTY_OBJ, &fieldsToReturn );
        if( gateways.size() != pluginIds.size() ) {
            waitForBroadcast( timeout, "Not all gateways are created: " + cc::j( gateways ) );
            timeout = std::chrono::seconds( 10 );
            continue;
        }
        auto tmpPlugins = pluginIds;
        for( const auto& gateway : gateways ) {
            auto pluginId = json_get_str( gateway, "pluginId" );
            tmpPlugins.erase( pluginId );
            auto status = json_get_str( gateway, "status" );
            if( status != "ready" ) {
                allGatewaysIsReady = false;
                break;
            }
        }
        if( allGatewaysIsReady && tmpPlugins.empty() ) {
            break;
        }
        waitForBroadcast( timeout, "Not all gateways are ready: " + cc::j( gateways ) );
        timeout = std::chrono::seconds( 10 );
    }
    m_broadcastMessages.clear();
}

void GatewaysChecker::handleMessage( rpc::Message& message ) {
    std::unique_lock< std::mutex > lock( m_broadcastsMutex );
    m_broadcastMessages.push_back( message.getMessage() );
    lock.unlock();
    m_broadcastsCondVar.notify_all();
}

}  // namespace Tests
}  // namespace HubPlatform
