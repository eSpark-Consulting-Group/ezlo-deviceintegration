#pragma once

#include "service_interfaces/script_client.h"
#include "addon_interfaces/addon.h"

namespace HubPlatform {
namespace Zigbee {

class ZigbeeClient : public ScriptClient {
public:
    ZigbeeClient( Addons::Addon& parent );

    void startInclude( const std::string& interviewConfigurationsFile );
    void stopInclude();
    void removeNode( const uint16_t nodeId );

    void setAttributeValue( const uint16_t nodeId,
                            const uint8_t endpointId,
                            const std::string& clusterName,
                            const std::string& attributeName,
                            const Json& value );

    void sendCommand( const uint16_t nodeId,
                      const uint8_t endpointId,
                      const std::string& clusterName,
                      const std::string& commandName,
                      const Json& params );

    void requestAttributeValue( const uint16_t nodeId,
                                const uint8_t endpointId,
                                const std::string& clusterName,
                                const std::string& attributeName );

    void requestUserCodes( const uint16_t nodeId,
                           const uint8_t endpointId,
                           const uint16_t startUserId,
                           const uint16_t endUserId );

    void requestMfgSpecificAttributeValue( const uint16_t nodeId,
                                           const uint8_t endpointId,
                                           const uint16_t clusterId,
                                           const uint16_t attributeId );

    void configureAttributeReport( const uint16_t nodeId,
                                   const uint8_t endpointId,
                                   const std::string& clusterName,
                                   const std::string& attributeName );

    void configureAttributeReport( const uint16_t nodeId,
                                   const uint8_t endpointId,
                                   const std::string& clusterName,
                                   const std::string& attributeName,
                                   const uint16_t minInterval,
                                   const uint16_t maxInterval,
                                   const Json& reportableChange );

    void configureMfgSpecificAttributeReport( const uint16_t nodeId,
                                              const uint8_t endpointId,
                                              const uint16_t clusterId,
                                              const uint16_t attributeId );

    void configureMfgSpecificAttributeReport( const uint16_t nodeId,
                                              const uint8_t endpointId,
                                              const uint16_t clusterId,
                                              const uint16_t attributeId,
                                              const uint16_t minInterval,
                                              const uint16_t maxInterval,
                                              const Json& reportableChange );

    void reset();

    Json getNodeAsJson( const uint16_t nodeId ) const;
    Json getAllNodesIds() const;
    bool hasNode( const uint16_t nodeId ) const;
    Json getInfo() const;

    void setNetworkRadioChannel( const uint8_t value );
    void collectChildAndRouteInfo();
};

}  // namespace Zigbee
}  // namespace HubPlatform
