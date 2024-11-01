#include "2gig_lua_client.h"
#include "2gig_addon.h"
#include "2gig/resources/resources.h"

#include "jsonrpc/jsonrpc.h"

namespace HubPlatform {
namespace TwoGig {

LuaClient::LuaClient( Addons::Addon& addon )
: ScriptClient( Addons::Addon::createQualifiedName( Addon::Name ), addon ) {}

void LuaClient::reset() {
    sendMethodCall( TWOGIG_RESET, JSON_EMPTY_OBJ );
}

void LuaClient::startDeviceDiscovering() {
    sendMethodCall( TWOGIG_START_DEVICE_DISCOVERING, JSON_EMPTY_OBJ );
}

void LuaClient::stopDeviceDiscovering() {
    sendMethodCall( TWOGIG_STOP_DEVICE_DISCOVERING, JSON_EMPTY_OBJ );
}

void LuaClient::addDevice( Json&& deviceInfo ) {
    sendMethodCall( TWOGIG_ADD_DEVICE, std::move( deviceInfo ) );
}

void LuaClient::removeDevice( const std::vector< uint32_t >& txIds ) {
    auto params = JSON_EMPTY_OBJ;
    params[ Resources::TxIds ] = txIds;

    sendMethodCall( TWOGIG_REMOVE_DEVICE, std::move( params ) );
}

void LuaClient::configureDevice( const std::vector< uint32_t >& txIds, Json&& config ) {
    auto params = JSON_EMPTY_OBJ;
    params[ Resources::TxIds ] = txIds;
    params[ Resources::Configuration ] = std::move( config );

    sendMethodCall( TWOGIG_CONFIGURE_DEVICE, std::move( params ) );
}

}  // namespace TwoGig
}  // namespace HubPlatform
