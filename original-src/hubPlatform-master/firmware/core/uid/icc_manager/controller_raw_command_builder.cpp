#include "uid/icc_manager/controller_raw_command_builder.h"

namespace HubPlatform {
namespace InterControllerCommunication {

ControllerRawCommandBuilder::ControllerRawCommandBuilder( Json command, const std::string& controllerUuid ) {
    Json params;
    params[ "command" ] = std::move( command );
    params[ "controller_uuid" ] = controllerUuid;

    Json message;
    m_command[ "api" ] = "1";
    m_command[ "method" ] = "cloud.controller_raw_command";
    m_command[ "params" ] = std::move( params );
}

void ControllerRawCommandBuilder::setQueued( bool value ) {
    m_command[ "params" ][ "queued" ] = static_cast< int >( value );
}

void ControllerRawCommandBuilder::setInstant( bool value ) {
    m_command[ "params" ][ "instant" ] = static_cast< int >( value );
}

void ControllerRawCommandBuilder::setExpiryTimestamp( std::chrono::seconds value ) {
    m_command[ "expiry_timestamp" ] = value.count();
}

void ControllerRawCommandBuilder::setInstantTimeout( std::chrono::seconds value ) {
    m_command[ "instant_timeout" ] = value.count();
}

void ControllerRawCommandBuilder::setHistoryId( const std::string& id ) {
    m_command[ "history" ] = 1;
    m_command[ "history_id" ] = id;
}

Json ControllerRawCommandBuilder::build( const std::string& messageId ) {
    m_command[ "id" ] = messageId;
    return std::move( m_command );
}

}  // namespace InterControllerCommunication
}  // namespace HubPlatform
