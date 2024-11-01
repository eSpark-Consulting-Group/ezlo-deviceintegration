#pragma once

#include "zigbee/utils/commands_sender_interface.h"
#include "zigbee/model/enums/cluster_id.h"
#include "zigbee/model/enums/attribute_id.h"

#include "json_utils/json_fwd.h"

namespace HubPlatform {
namespace Zigbee {

enum class AttributeValueType;
enum class IasAceArmResponse;
enum class IasAcePanelStatus;
enum class IasAceAlarmStatus;
enum class UserStatus;

class CommandsSender : public CommandsSenderInterface {
public:
    // Send cluster specific command
    void sendCommand( uint16_t nodeId,
                      uint8_t endpointId,
                      uint16_t clusterId,
                      const std::string& commandName,
                      const Json& commandParams ) override;

    // Set attribute value
    void setAttributeValue( uint16_t nodeId,
                            uint8_t endpointId,
                            uint16_t clusterId,
                            uint16_t attributeId,
                            uint8_t attributeValueType,
                            const Json& value ) override;

    // Bind specified cluster
    void sendBindingCommand( uint16_t nodeId, uint8_t endpointId, uint16_t clusterId ) override;

    // Configure attributes reporting with default configurations
    void sendConfigureAttributeReportingCommand( uint16_t nodeId,
                                                 uint8_t endpointId,
                                                 uint16_t clusterId,
                                                 uint16_t attributeId,
                                                 bool isMfgSpecific,
                                                 uint16_t mfgCode ) override;

    // Configure attributes reporting with custom configurations
    void sendConfigureAttributeReportingCommand( uint16_t nodeId,
                                                 uint8_t endpointId,
                                                 uint16_t clusterId,
                                                 uint16_t attributeId,
                                                 AttributeValueType attributeValueType,
                                                 const uint16_t minInterval,
                                                 const uint16_t maxInterval,
                                                 const Json& reportableChange,
                                                 bool isMfgSpecific,
                                                 uint16_t mfgCode ) override;

    // Request single attribute value
    bool requestAttribute( uint16_t nodeId,
                           uint8_t endpointId,
                           uint16_t clusterId,
                           uint16_t attributeId,
                           bool isMfgSpecific,
                           uint16_t mfgCode ) override;

    // Request several attributes values
    bool requestAttributes( uint16_t nodeId,
                            uint8_t endpointId,
                            uint16_t clusterId,
                            const std::vector< uint16_t >& attributes,
                            bool isMfgSpecific = false,
                            uint16_t mfgCode = 0xffff ) override;

    // Discover cluster attributes
    bool discoverAttributes( uint16_t nodeId,
                             uint8_t endpointId,
                             uint16_t clusterId,
                             uint16_t startAttribute,
                             uint8_t maxAttributesCount,
                             bool isMfgSpecific = false,
                             uint16_t mfgCode = 0xffff ) override;

    // Discover cluster commands
    bool discoverCommandsReceived( uint16_t nodeId,
                                   uint8_t endpointId,
                                   uint16_t clusterId,
                                   uint16_t startCommand,
                                   uint8_t maxCommandsCount,
                                   CommandDirection direction,
                                   bool isMfgSpecific = false,
                                   uint16_t mfgCode = 0xffff ) override;

    bool discoverCommandsGenerated( uint16_t nodeId,
                                    uint8_t endpointId,
                                    uint16_t clusterId,
                                    uint16_t startCommand,
                                    uint8_t maxCommandsCount,
                                    CommandDirection direction,
                                    bool isMfgSpecific = false,
                                    uint16_t mfgCode = 0xffff ) override;

private:
    // On/Off cluster
    void sendOnOffClusterCommand( uint16_t nodeId, uint8_t endpointId, const std::string& commandName );
    void sendOnOffClusterOnCommand( uint16_t nodeId, uint8_t endpointId );
    void sendOnOffClusterOffCommand( uint16_t nodeId, uint8_t endpointId );

    // Level control cluster
    void sendLevelControlClusterCommand( uint16_t nodeId,
                                         uint8_t endpointId,
                                         const std::string& commandName,
                                         const Json& commandParams );
    void sendLevelControlClusterMoveToLevelCommand( uint16_t nodeId, uint8_t endpointId, uint8_t level );
    void sendLevelControlClusterMoveCommand( uint16_t nodeId, uint8_t endpointId, const std::string& direction );
    void sendLevelControlClusterStopMovingCommand( uint16_t nodeId, uint8_t endpointId );

    // Color control cluster
    void sendColorControlClusterCommand( uint16_t nodeId,
                                         uint8_t endpointId,
                                         const std::string& commandName,
                                         const Json& commandParams );

    void sendColorControlClusterMoveToColorCommand( uint16_t nodeId,
                                                    uint8_t endpointId,
                                                    uint16_t currentX,
                                                    uint16_t currentY );

    void sendColorControlClusterMoveToTemperatureCommand( uint16_t nodeId, uint8_t endpointId, uint16_t temperature );

    // Thermostat cluster
    void setThermostatAttribute( uint16_t nodeId,
                                 uint8_t endpointId,
                                 uint16_t attributeId,
                                 uint8_t attributeValueType,
                                 const Json& value );
    void setThermostatSystemMode( uint16_t nodeId,
                                  uint8_t endpointId,
                                  uint16_t attributeId,
                                  uint8_t attributeValueType,
                                  const Json& value );
    void setThermostatSetpoint( uint16_t nodeId,
                                uint8_t endpointId,
                                uint16_t attributeId,
                                uint8_t attributeValueType,
                                const Json& value );

    void setThermostatLocalTemperatureCalibration( uint16_t nodeId,
                                                   uint8_t endpointId,
                                                   uint16_t attributeId,
                                                   uint8_t attributeValueType,
                                                   const Json& value );

    // Fan Control Cluster
    void setFanControlAttribute( uint16_t nodeId,
                                 uint8_t endpointId,
                                 uint16_t attributeId,
                                 uint8_t attributeValueType,
                                 const Json& value );
    void setFanControlFanMode( uint16_t nodeId,
                               uint8_t endpointId,
                               uint16_t attributeId,
                               uint8_t attributeValueType,
                               const Json& value );

    // Poll Control Cluster
    void setPollControlAttribute( uint16_t nodeId,
                                  uint8_t endpointId,
                                  uint16_t attributeId,
                                  uint8_t attributeValueType,
                                  const Json& value );

    void setPollControlCheckInIntervalAttribute( uint16_t nodeId,
                                                 uint8_t endpointId,
                                                 uint16_t attributeId,
                                                 uint8_t attributeValueType,
                                                 const Json& value );

    void sendPollControlClusterCommand( uint16_t nodeId,
                                        uint8_t endpointId,
                                        const std::string& commandName,
                                        const Json& commandParams );

    void sendGetAlertsResponseCommand( uint16_t nodeId, uint8_t endpointId, const std::string& commandName );

    void sendPollControlClusterCheckInResponseCommand( uint16_t nodeId,
                                                       uint8_t endpointId,
                                                       bool startFastPollingMode,
                                                       uint16_t fastPollTimeout );

    void sendPollControlClusterFastPollStopCommand( uint16_t nodeId, uint8_t endpointId );

    // IAS Ace Cluster
    void sendIasAceClusterCommand( uint16_t nodeId,
                                   uint8_t endpointId,
                                   const std::string& commandName,
                                   const Json& commandParams );

    void sendIasAceClusterArmResponseCommand( uint16_t nodeId, uint8_t endpointId, const IasAceArmResponse& status );

    void sendIasAceClusterGetPanelStatusResponseCommand( uint16_t nodeId,
                                                         uint8_t endpointId,
                                                         const IasAcePanelStatus& status );

    void sendIasAceClusterPanelStatusChangedCommand( uint16_t nodeId,
                                                     uint8_t endpointId,
                                                     const IasAcePanelStatus& status,
                                                     uint8_t secondsReamining,
                                                     const IasAceAlarmStatus& alarmStatus );

    // Door Lock cluster
    void sendDoorLockClusterCommand( uint16_t nodeId,
                                     uint8_t endpointId,
                                     const std::string& commandName,
                                     const Json& commandParams );

    void sendDoorLockClusterLockDoorCommand( uint16_t nodeId, uint8_t endpointId, const std::string& pinCode );
    void sendDoorLockClusterUnlockDoorCommand( uint16_t nodeId, uint8_t endpointId, const std::string& pinCode );

    void sendDoorLockClusterSetPinCodeCommand( uint16_t nodeId,
                                               uint8_t endpointId,
                                               uint16_t userId,
                                               UserStatus userStatus,
                                               const std::string& pinCode );
    void sendDoorLockClusterGetPinCodeCommand( uint16_t nodeId, uint8_t endpointId, uint16_t userId );
    void sendDoorLockClusterClearPinCodeCommand( uint16_t nodeId, uint8_t endpointId, uint16_t userId );
    void sendDoorLockClusterClearAllPinCodesCommand( uint16_t nodeId, uint8_t endpointId );

    // Occupancy Sensing cluster
    void setOccupancySensingAttribute( uint16_t nodeId,
                                       uint8_t endpointId,
                                       uint16_t attributeId,
                                       uint8_t attributeValueType,
                                       const Json& value );

    // Common
    template < typename DataType >
    void sendConfigureAttributeReportingCommandInternal( uint16_t nodeId,
                                                         uint8_t endpointId,
                                                         uint16_t clusterId,
                                                         uint16_t attributeId,
                                                         uint8_t attributeValueType,
                                                         const uint16_t minInterval,
                                                         const uint16_t maxInterval,
                                                         DataType reportableChange,
                                                         bool isMfgSpecific,
                                                         uint16_t mfgCode );

    void sendConfigureAttributeReportingCommandInternal( uint16_t nodeId,
                                                         uint8_t endpointId,
                                                         uint16_t clusterId,
                                                         uint16_t attributeId,
                                                         uint8_t attributeValueType,
                                                         const uint16_t minInterval,
                                                         const uint16_t maxInterval,
                                                         bool isMfgSpecific,
                                                         uint16_t mfgCode );

    template < typename DataType >
    size_t addDataToMessage( uint8_t* data, size_t startIndex, DataType reportableChange );

    void sendCommandWithEndpoint( uint16_t nodeId, uint8_t endpointId );
};

}  // namespace Zigbee
}  // namespace HubPlatform
