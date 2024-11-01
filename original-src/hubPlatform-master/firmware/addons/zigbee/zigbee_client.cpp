#include "zigbee/zigbee_client.h"
#include "zigbee/zigbee_addon.h"
#include "zigbee/resources/zigbee_resources.h"
#include "zigbee/common/zigbee_exceptions.h"

#include "json_utils/json_utils.h"

#include "database_client/dbqueryparser.h"

namespace HubPlatform {
namespace Zigbee {

ZigbeeClient::ZigbeeClient( Addons::Addon& hostAddon )
: ScriptClient( Addons::Addon::createQualifiedName( ZigbeeAddon::Name ), hostAddon ) {}

void ZigbeeClient::startInclude( const std::string& interviewConfigurationsFile ) {
    auto params = JSON_EMPTY_OBJ;
    params[ Resources::RPCParams::InterviewConfigurationsFile ] = interviewConfigurationsFile;

    sendMethodCall( MessageSubClasses::ZIGBEE_START_INCLUDE, std::move( params ) );
}

void ZigbeeClient::stopInclude() {
    sendMethodCall( MessageSubClasses::ZIGBEE_STOP_INCLUDE, JSON_EMPTY_OBJ );
}

void ZigbeeClient::removeNode( const uint16_t nodeId ) {
    if( !hasNode( nodeId ) ) {
        throw NodeNotFoundError{ nodeId };
    }

    auto params = JSON_EMPTY_OBJ;
    params[ Resources::RPCParams::NodeId ] = nodeId;

    sendMethodCall( MessageSubClasses::ZIGBEE_REMOVE_NODE, std::move( params ) );
}

Json ZigbeeClient::getNodeAsJson( const uint16_t nodeId ) const {
    auto& db = getHost().getDBClient();
    auto result =
        db.findOne( Zigbee::Resources::DB::TableNames::Devices, JSON( db::ID_FIELD_NAME << nodeId ), nullptr );
    return result;
}

Json ZigbeeClient::getAllNodesIds() const {
    const JsonObject fieldsToReturn = JSON( Resources::DB::Fields::Id << "id" );
    auto data = getHost().getDBClient().findAll( Resources::DB::TableNames::Devices, JSON_EMPTY_OBJ, &fieldsToReturn );
    JsonArray result = Json::array();
    for( const auto& object : data ) {
        result.push_back( json_get_int( object, "id" ) );
    }
    return result;
}

bool ZigbeeClient::hasNode( const uint16_t nodeId ) const {
    auto node = getNodeAsJson( nodeId );
    return !node.empty();
}

Json ZigbeeClient::getInfo() const {
    return sendRequest( ZIGBEE_GET_INFO, {} );
}

void ZigbeeClient::setAttributeValue( const uint16_t nodeId,
                                      const uint8_t endpointId,
                                      const std::string& clusterName,
                                      const std::string& attributeName,
                                      const Json& value ) {
    if( !hasNode( nodeId ) ) {
        throw NodeNotFoundError{ nodeId };
    }

    auto rpcParams = JSON_EMPTY_OBJ;
    rpcParams[ Resources::RPCParams::NodeId ] = nodeId;
    rpcParams[ Resources::RPCParams::EndpointId ] = endpointId;
    rpcParams[ Resources::RPCParams::ClusterName ] = clusterName;
    rpcParams[ Resources::RPCParams::AttributeName ] = attributeName;
    rpcParams[ Resources::RPCParams::Value ] = value;

    sendMethodCall( MessageSubClasses::ZIGBEE_SET_ATTRIBUTE_VALUE, std::move( rpcParams ) );
}

void ZigbeeClient::setNetworkRadioChannel( const uint8_t value ) {
    auto rpcParams = JSON_EMPTY_OBJ;
    rpcParams[ Resources::RPCParams::Value ] = value;

    sendMethodCall( MessageSubClasses::ZIGBEE_SET_NETWORK_RADIO_CHANNEL, std::move( rpcParams ) );
}

void ZigbeeClient::collectChildAndRouteInfo() {
    auto rpcParams = JSON_EMPTY_OBJ;

    sendMethodCall( MessageSubClasses::ZIGBEE_GET_CHILD_AND_ROUTE_INFO, std::move( rpcParams ) );
}

void ZigbeeClient::sendCommand( const uint16_t nodeId,
                                const uint8_t endpointId,
                                const std::string& clusterName,
                                const std::string& commandName,
                                const Json& params ) {
    if( !hasNode( nodeId ) ) {
        throw NodeNotFoundError{ nodeId };
    }

    auto rpcParams = JSON_EMPTY_OBJ;
    rpcParams[ Resources::RPCParams::NodeId ] = nodeId;
    rpcParams[ Resources::RPCParams::EndpointId ] = endpointId;
    rpcParams[ Resources::RPCParams::ClusterName ] = clusterName;
    rpcParams[ Resources::RPCParams::CommandName ] = commandName;
    rpcParams[ Resources::RPCParams::CommandParams ] = params;

    sendMethodCall( MessageSubClasses::ZIGBEE_SEND_COMMAND, std::move( rpcParams ) );
}

void ZigbeeClient::requestAttributeValue( const uint16_t nodeId,
                                          const uint8_t endpointId,
                                          const std::string& clusterName,
                                          const std::string& attributeName ) {
    if( !hasNode( nodeId ) ) {
        throw NodeNotFoundError{ nodeId };
    }

    auto params = JSON_EMPTY_OBJ;
    params[ Resources::RPCParams::NodeId ] = nodeId;
    params[ Resources::RPCParams::EndpointId ] = endpointId;
    params[ Resources::RPCParams::ClusterName ] = clusterName;
    params[ Resources::RPCParams::AttributeName ] = attributeName;

    sendMethodCall( MessageSubClasses::ZIGBEE_REQUEST_ATTRIBUTE_VALUE, std::move( params ) );
}

void ZigbeeClient::requestUserCodes( const uint16_t nodeId,
                                     const uint8_t endpointId,
                                     const uint16_t startUserId,
                                     const uint16_t endUserId ) {
    if( !hasNode( nodeId ) ) {
        throw NodeNotFoundError{ nodeId };
    }

    auto params = JSON_EMPTY_OBJ;
    params[ Resources::RPCParams::NodeId ] = nodeId;
    params[ Resources::RPCParams::EndpointId ] = endpointId;
    params[ Resources::RPCParams::StartUserId ] = startUserId;
    params[ Resources::RPCParams::EndUserId ] = endUserId;

    sendMethodCall( MessageSubClasses::ZIGBEE_REQUEST_USER_CODES, std::move( params ) );
}

void ZigbeeClient::requestMfgSpecificAttributeValue( const uint16_t nodeId,
                                                     const uint8_t endpointId,
                                                     const uint16_t clusterId,
                                                     const uint16_t attributeId ) {
    if( !hasNode( nodeId ) ) {
        throw NodeNotFoundError{ nodeId };
    }

    auto params = JSON_EMPTY_OBJ;
    params[ Resources::RPCParams::NodeId ] = nodeId;
    params[ Resources::RPCParams::EndpointId ] = endpointId;
    params[ Resources::RPCParams::ClusterId ] = clusterId;
    params[ Resources::RPCParams::AttributeId ] = attributeId;

    sendMethodCall( MessageSubClasses::ZIGBEE_REQUEST_MFG_SPECIFIC_ATTRIBUTE_VALUE, std::move( params ) );
}

void ZigbeeClient::configureAttributeReport( const uint16_t nodeId,
                                             const uint8_t endpointId,
                                             const std::string& clusterName,
                                             const std::string& attributeName ) {
    if( !hasNode( nodeId ) ) {
        throw NodeNotFoundError{ nodeId };
    }

    auto params = JSON_EMPTY_OBJ;
    params[ Resources::RPCParams::NodeId ] = nodeId;
    params[ Resources::RPCParams::EndpointId ] = endpointId;
    params[ Resources::RPCParams::ClusterName ] = clusterName;
    params[ Resources::RPCParams::AttributeName ] = attributeName;

    sendMethodCall( MessageSubClasses::ZIGBEE_CONFIGURE_ATTRIBUTE_REPORT, std::move( params ) );
}

void ZigbeeClient::configureAttributeReport( const uint16_t nodeId,
                                             const uint8_t endpointId,
                                             const std::string& clusterName,
                                             const std::string& attributeName,
                                             const uint16_t minInterval,
                                             const uint16_t maxInterval,
                                             const Json& reportableChange ) {
    if( !hasNode( nodeId ) ) {
        throw NodeNotFoundError{ nodeId };
    }

    auto params = JSON_EMPTY_OBJ;
    params[ Resources::RPCParams::NodeId ] = nodeId;
    params[ Resources::RPCParams::EndpointId ] = endpointId;
    params[ Resources::RPCParams::ClusterName ] = clusterName;
    params[ Resources::RPCParams::AttributeName ] = attributeName;
    params[ Resources::RPCParams::MinInterval ] = minInterval;
    params[ Resources::RPCParams::MaxInterval ] = maxInterval;
    params[ Resources::RPCParams::ReportableChange ] = reportableChange;

    sendMethodCall( MessageSubClasses::ZIGBEE_CONFIGURE_ATTRIBUTE_REPORT, std::move( params ) );
}

void ZigbeeClient::configureMfgSpecificAttributeReport( const uint16_t nodeId,
                                                        const uint8_t endpointId,
                                                        const uint16_t clusterId,
                                                        const uint16_t attributeId ) {
    if( !hasNode( nodeId ) ) {
        throw NodeNotFoundError{ nodeId };
    }

    auto params = JSON_EMPTY_OBJ;
    params[ Resources::RPCParams::NodeId ] = nodeId;
    params[ Resources::RPCParams::EndpointId ] = endpointId;
    params[ Resources::RPCParams::ClusterId ] = clusterId;
    params[ Resources::RPCParams::AttributeId ] = attributeId;

    sendMethodCall( MessageSubClasses::ZIGBEE_CONFIGURE_MFG_SPECIFIC_ATTRIBUTE_REPORT, std::move( params ) );
}

void ZigbeeClient::configureMfgSpecificAttributeReport( const uint16_t nodeId,
                                                        const uint8_t endpointId,
                                                        const uint16_t clusterId,
                                                        const uint16_t attributeId,
                                                        const uint16_t minInterval,
                                                        const uint16_t maxInterval,
                                                        const Json& reportableChange ) {
    if( !hasNode( nodeId ) ) {
        throw NodeNotFoundError{ nodeId };
    }

    auto params = JSON_EMPTY_OBJ;
    params[ Resources::RPCParams::NodeId ] = nodeId;
    params[ Resources::RPCParams::EndpointId ] = endpointId;
    params[ Resources::RPCParams::ClusterId ] = clusterId;
    params[ Resources::RPCParams::AttributeId ] = attributeId;
    params[ Resources::RPCParams::MinInterval ] = minInterval;
    params[ Resources::RPCParams::MaxInterval ] = maxInterval;
    params[ Resources::RPCParams::ReportableChange ] = reportableChange;

    sendMethodCall( MessageSubClasses::ZIGBEE_CONFIGURE_MFG_SPECIFIC_ATTRIBUTE_REPORT, std::move( params ) );
}

void ZigbeeClient::reset() {
    sendMethodCall( MessageSubClasses::ZIGBEE_RESET, JSON_EMPTY_OBJ );
}
}  // namespace Zigbee
}  // namespace HubPlatform
