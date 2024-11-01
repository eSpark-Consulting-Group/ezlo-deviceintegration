#pragma once

#include "zigbee/model/enums/cluster_id.h"
#include "zigbee/model/enums/attribute_id.h"

#include "json_utils/json_fwd.h"

namespace HubPlatform {
namespace Zigbee {

enum class AttributeValueType;
enum class IasAceArmResponse;
enum class IasAcePalenStatus;

class CommandsSenderInterface {
public:
    enum class CommandDirection { ClientToServer, ServerToClient };

    virtual ~CommandsSenderInterface() = default;

    // Send cluster specific command
    virtual void sendCommand( uint16_t nodeId,
                              uint8_t endpointId,
                              uint16_t clusterId,
                              const std::string& commandName,
                              const Json& commandParams ) = 0;

    // Set attribute value
    virtual void setAttributeValue( uint16_t nodeId,
                                    uint8_t endpointId,
                                    uint16_t clusterId,
                                    uint16_t attributeId,
                                    uint8_t attributeValueType,
                                    const Json& value ) = 0;

    // Bind specified cluster
    virtual void sendBindingCommand( uint16_t nodeId, uint8_t endpointId, uint16_t clusterId ) = 0;

    // Configure attributes reporting with default configurations
    virtual void sendConfigureAttributeReportingCommand( uint16_t nodeId,
                                                         uint8_t endpointId,
                                                         uint16_t clusterId,
                                                         uint16_t attributeId,
                                                         bool isMfgSpecific,
                                                         uint16_t mfgCode ) = 0;

    // Configure attributes reporting with custom configurations
    virtual void sendConfigureAttributeReportingCommand( uint16_t nodeId,
                                                         uint8_t endpointId,
                                                         uint16_t clusterId,
                                                         uint16_t attributeId,
                                                         AttributeValueType attributeValueType,
                                                         const uint16_t minInterval,
                                                         const uint16_t maxInterval,
                                                         const Json& reportableChange,
                                                         bool isMfgSpecific,
                                                         uint16_t mfgCode ) = 0;

    // Request single attribute value
    virtual bool requestAttribute( uint16_t nodeId,
                                   uint8_t endpointId,
                                   uint16_t clusterId,
                                   uint16_t attributeId,
                                   bool isMfgSpecific,
                                   uint16_t mfgCode ) = 0;

    // Request several attributes values
    virtual bool requestAttributes( uint16_t nodeId,
                                    uint8_t endpointId,
                                    uint16_t clusterId,
                                    const std::vector< uint16_t >& attributes,
                                    bool isMfgSpecific = false,
                                    uint16_t mfgCode = 0xffff ) = 0;

    // Discover cluster attributes
    virtual bool discoverAttributes( uint16_t nodeId,
                                     uint8_t endpointId,
                                     uint16_t clusterId,
                                     uint16_t startAttribute,
                                     uint8_t maxAttributesCount,
                                     bool isMfgSpecific = false,
                                     uint16_t mfgCode = 0xffff ) = 0;

    // Discover cluster commands
    virtual bool discoverCommandsReceived( uint16_t nodeId,
                                           uint8_t endpointId,
                                           uint16_t clusterId,
                                           uint16_t startCommand,
                                           uint8_t maxCommandsCount,
                                           CommandDirection direction,
                                           bool isMfgSpecific = false,
                                           uint16_t mfgCode = 0xffff ) = 0;

    virtual bool discoverCommandsGenerated( uint16_t nodeId,
                                            uint8_t endpointId,
                                            uint16_t clusterId,
                                            uint16_t startCommand,
                                            uint8_t maxCommandsCount,
                                            CommandDirection direction,
                                            bool isMfgSpecific = false,
                                            uint16_t mfgCode = 0xffff ) = 0;
};

}  // namespace Zigbee
}  // namespace HubPlatform
