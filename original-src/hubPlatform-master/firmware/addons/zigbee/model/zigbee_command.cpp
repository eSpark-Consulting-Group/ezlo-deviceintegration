#include "zigbee/model/zigbee_command.h"

#include "zigbee/resources/zigbee_resources.h"
#include "zigbee/utils/attributes_formatter.h"
#include "database_client/dbqueryparser.h"
#include "json_utils/json_utils.h"

#include "common_utils/cpp14.h"

#include "zigbee/zigbee_log.h"

namespace HubPlatform {
namespace Zigbee {

std::unique_ptr< ZigbeeCommand > ZigbeeCommandFactory::createZigbeeCommand( uint16_t clusterId,
                                                                            uint8_t commandId,
                                                                            const std::string& commandName,
                                                                            CommandType type,
                                                                            bool isMfgSpecific ) {
    Log::print( Log::PriorityLevel::DEBUG,
                cc::debug( "New Zigbee command created: clusterId: 0x%04x, commandId: 0x%04x, commandName: %s, "
                           "commandType: %s, isMfgSpecific: %s" )
                    .c_str(),
                clusterId,
                commandId,
                commandName.c_str(),
                toString( type ),
                isMfgSpecific ? "true" : "false" );

    return std::make_unique< ZigbeeCommand >( commandId, commandName, clusterId, type, isMfgSpecific );
}

ZigbeeCommand::ZigbeeCommand( uint8_t id,
                              const std::string& name,
                              uint16_t clusterId,
                              CommandType type,
                              bool isMfgSpecific )
: m_id( id )
, m_name( name )
, m_clusterId( clusterId )
, m_type( type )
, m_isMfgSpecific( isMfgSpecific ) {}

ZigbeeCommand::ZigbeeCommand( const Json& fromJson )
: m_id( json_get_int( fromJson, Resources::Fields::Command::Id ) )
, m_name( json_get_str( fromJson, Resources::Fields::Command::Name ) )
, m_clusterId( json_get_int( fromJson, Resources::Fields::Command::ClusterId ) )
, m_type( commandTypeFromString( json_get_str( fromJson, Resources::Fields::Command::Type ) ) )
, m_isMfgSpecific( json_get_bool( fromJson, Resources::Fields::Command::IsMfgSpecific ) )
, m_mfgCode( json_get_int( fromJson, Resources::Fields::Command::MfgCode ) ) {}

uint8_t ZigbeeCommand::getId() const {
    return m_id;
}

CommandType ZigbeeCommand::getType() const {
    return m_type;
}

const std::string& ZigbeeCommand::getName() const {
    return m_name;
}

bool ZigbeeCommand::isMfgSpecific() const {
    return m_isMfgSpecific;
}

const uint16_t ZigbeeCommand::getMfgCode() const {
    return m_mfgCode;
}

void ZigbeeCommand::setMfgCode( const uint16_t newValue ) {
    m_mfgCode = newValue;
}

Json ZigbeeCommand::toJson() const {
    JsonObject result;

    result[ Resources::Fields::Command::Id ] = m_id;
    result[ Resources::Fields::Command::Name ] = m_name;
    result[ Resources::Fields::Command::ClusterId ] = m_clusterId;
    result[ Resources::Fields::Command::Type ] = toString( m_type );
    result[ Resources::Fields::Command::IsMfgSpecific ] = m_isMfgSpecific;
    result[ Resources::Fields::Command::MfgCode ] = m_mfgCode;

    return result;
}

}  // namespace Zigbee
}  // namespace HubPlatform
