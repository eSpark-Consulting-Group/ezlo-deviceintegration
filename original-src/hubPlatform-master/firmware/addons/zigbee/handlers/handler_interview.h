#pragma once

#include "base_utils/native_timer.h"
#include "service_interfaces/service.h"
#include "zigbee/zigbee_node_manager.h"
#include "zigbee/zigbee_addon_callback.h"
#include "zigbee/utils/commands_sender_interface.h"

#include "zigbee/handlers/default_handler.h"

namespace HubPlatform {
namespace Zigbee {

class HandlerInterview final : public DefaultHandler {
private:
    class BindingEntity {
    public:
        explicit BindingEntity( uint8_t endpointId, uint16_t clusterId );
        BindingEntity( BindingEntity&& fromMove );

    private:
        BindingEntity() = delete;
        BindingEntity( BindingEntity& fromCopy ) = delete;

    public:
        static BindingEntity InvalidBindingEntity;
        uint8_t getEndpointId() const;
        uint16_t getClusterId() const;

    private:
        const uint8_t m_endpointId;
        const uint16_t m_clusterId;
    };

public:
    using ZigbeeTask = std::function< void() >;
    using ZigbeeNodeUPtr = std::unique_ptr< ZigbeeNode >;

    enum class State {
        Idle,
        Discovery,
        ClustersCollection,
        ManufacturerInfoExtraction,
        AttributesDiscovery,
        ClustersInterview,
        ClustersCollectionWithNoAttrDisc,
        ClustersInterviewWithExtraction,
        CheckInIntervalExtraction,
        DiscoverCommandsReceived,
        DiscoverCommandsGenerated,
        CollectClustersToBind,
        ClustersBinding,

        MfgInfoExtractionPrepare,
        MfgAttributesDiscovery,
        MfgClustersInterview,

        InterviewDone
    };

    enum class AttributesRequestStatus { NoNeed, Failed, Success };

    HandlerInterview( ZigbeeAddonCallbackInterface& callbackInterface, CommandsSenderInterface& commandsSender );
    HandlerInterview() = delete;
    ~HandlerInterview() = default;

private:
    void executeStep( State state );
    void executeStepAsync( State state );
    void executeCurrentStep( bool async );
    void executeNextStep( bool async );

    void processNextCluster();

    void executeManufacturerInfoExtraction();
    void executeAttributesDiscovery();
    void executeClustersCollectionWithNoAttrDisc();
    void executeClustersInterview( bool withExtraction );
    void executeCheckInIntervalExtraction();
    void executeDiscoverCommands();
    void collectClustersToBind();
    void executeMfgInfoExtractionPrepare();
    void executeMfgAttributesDiscovery();
    void executeMfgClustersInterview();

    std::chrono::seconds getStepTimeout() const;

    void trustCenterJoinCallback( EmberNodeId nodeId,
                                  uint8_t* newNodeEui64,
                                  EmberNodeId parentOfNewNode,
                                  EmberDeviceUpdate status,
                                  EmberJoinDecision decision ) override;

    bool preZDOMessageReceivedCallback( EmberNodeId emberNodeId,
                                        EmberApsFrame* apsFrame,
                                        uint8_t* message,
                                        uint16_t length ) override;

    bool readAttributesResponseCallback( EmberNodeId emberNodeId,
                                         EmberAfClusterId clusterId,
                                         uint8_t endpointId,
                                         uint8_t* buffer,
                                         uint16_t bufLen ) override;

    bool attributesDiscoveryResponseCallback( EmberNodeId emberNodeId,
                                              EmberAfClusterId clusterId,
                                              uint8_t endpointId,
                                              uint8_t* buffer,
                                              uint16_t bufLen,
                                              bool extended ) override;

    bool commandsDiscoveryResponseCallback( EmberNodeId emberNodeId,
                                            EmberAfClusterId clusterId,
                                            uint8_t endpointId,
                                            CommandType commandsType,
                                            uint8_t* commandIds,
                                            uint16_t commandIdCount ) override;

    bool defaultResponseCallback( EmberNodeId emberNodeId,
                                  EmberAfClusterId clusterId,
                                  uint8_t endpointId,
                                  uint8_t commandId,
                                  EmberAfStatus status ) override;

    void discoveryCompleteCallback( const EmberAfDeviceInfo* device ) override;

    void configureReportingResponseCallback( uint16_t nodeId,
                                             uint8_t endpointId,
                                             uint16_t clusterId,
                                             uint8_t* buffer,
                                             uint16_t bufLen ) override;

    // interview-specific methods
private:
    AttributesRequestStatus requestClusterAttributes( EmberNodeId nodeId,
                                                      uint8_t endpointId,
                                                      ZigbeeClusterBase& cluster,
                                                      bool withExtraction );

    bool discoverAttributes( EmberNodeId nodeId,
                             uint8_t endpointId,
                             EmberAfClusterId clusterId,
                             uint16_t startAttribute,
                             uint16_t mfgCode = 0xffff );

    bool discoverCommands( EmberNodeId nodeId,
                           uint8_t endpointId,
                           EmberAfClusterId clusterId,
                           ZigbeeClusterBase::ClusterDataHolder dataHolder,
                           uint16_t startCommand );

    void bindNextEntity( bool timeout = false );

    void interviewDone( ModeStatus interviewStatus );
    ZigbeeAttribute::Attributes parseAttributesDiscovery( EmberAfClusterId clusterId,
                                                          uint8_t* buffer,
                                                          uint16_t bufLen,
                                                          bool extended );

    bool isFromSupportedProfile( const ZigbeeClusterBase& cluster ) const;
    bool isSupportedProfile( uint16_t id ) const;

    void finishClusterProcessing();
    void resetClustersProcessing();
    ZigbeeClusterBase* getCurrentCluster();

    CommandType getCurrentCommandType() const;

    void sendProgress( State state );
    uint8_t getCurrentProgress( State state ) const;

    void executeRepeatedRequest( std::function< bool() > request );

private:
    State m_state;
    uint8_t m_currentProgress;

    EmberNodeId m_nodeId;
    std::shared_ptr< ZigbeeNode > m_node;
    std::unique_ptr< NodeInterviewNamesExtractors > m_nodeInterviewNamesExtractors;
    std::unique_ptr< NodeInterviewConfigurations > m_nodeInterviewConfigurations;

    ZigbeeAddonCallbackInterface& m_callbackInterface;
    CommandsSenderInterface& m_commandsSender;

    std::vector< BindingEntity > m_bindingList;
    size_t m_bindingRetriesCount;

    std::vector< ZigbeeClusterBase* > m_clusters;
    std::unordered_set< uint16_t > m_clustersWithNoAttrDisc;
    std::unordered_set< uint16_t > m_unsupportedClusterAttributes;
    size_t m_currentCluster;
    size_t m_clusterRetriesCount;

    // interview specific timers
    NativeTimer m_stepTimer;
};

}  // namespace Zigbee
}  // namespace HubPlatform
