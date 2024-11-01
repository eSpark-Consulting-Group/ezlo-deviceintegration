#include "zigbee/handlers/handler_interview.h"

#include "zigbee/resources/enums/mode_status.h"
#include "zigbee/resources/zigbee_resources.h"
#include "zigbee/utils/attributes_parser.h"
#include "zigbee/utils/discovery_parser.h"

#include "zigbee/model/zigbee_cluster.h"
#include "zigbee/model/enums/attribute_id.h"
#include "zigbee/model/enums/command_id.h"

#include "zigbee/interview_configurations/node_interview_configurations.h"
#include "zigbee/interview_configurations/node_interview_names_extractors.h"

#include "zigbee/zigbee_task_loop.h"
#include "zigbee/zigbee_log.h"

#include "common_utils/cpp14.h"
#include "log_utils/ezlo_log.h"
#include "database_client/dbqueryparser.h"

#include <tiny/tiny_consolecolors.h>

extern "C" {
#include "app/framework/plugin/device-table/device-table.h"
#include "app/framework/plugin/device-table/device-table-internal.h"
#include "app/framework/plugin/device-database/device-database.h"
#include "app/framework/plugin/poll-control-client/poll-control-client.h"
}
namespace {
constexpr const std::chrono::seconds discoveringStepTimeoutSeconds( 160 );

constexpr const std::chrono::seconds stepTimeoutLongSeconds( 8 );
constexpr const std::chrono::seconds stepTimeoutShortSeconds( 3 );

constexpr const size_t bindingRetriesCountMax = 3;
constexpr const size_t attributesRequestRetriesCountMax = 2;

constexpr const int attributesInSingleRequestCountMax = 5;

constexpr const uint8_t minimalProgressChange = 5;

// non-public stack definition
/// device-query-service.c
#define RECEIVER_ON_WHEN_IDLE 0x08
// With device announce there is only the ZDO sequence number, there is no status code.
#define DEVICE_ANNOUNCE_NODE_ID_OFFSET 1
#define DEVICE_ANNOUNCE_EUI64_OFFSET ( DEVICE_ANNOUNCE_NODE_ID_OFFSET + 2 )
#define DEVICE_ANNOUNCE_CAPABILITIES_OFFSET ( DEVICE_ANNOUNCE_EUI64_OFFSET + EUI64_SIZE )
/// device-query-service.c
}  // namespace

namespace HubPlatform {
namespace Zigbee {
namespace {

using State = HandlerInterview::State;
const std::string& stepStateToString( State state ) {
    static const std::string unknownName = "Unknown state";
    static const std::unordered_map< State, std::string > names = {
        { State::Idle, std::string{ "Idle" } },
        { State::Discovery, std::string{ "Discovery" } },
        { State::ClustersCollection, std::string{ "ClustersCollection" } },
        { State::ManufacturerInfoExtraction, std::string{ "ManufacturerInfoExtraction" } },
        { State::AttributesDiscovery, std::string{ "AttributesDiscovery" } },
        { State::ClustersInterview, std::string{ "ClustersInterview" } },
        { State::ClustersCollectionWithNoAttrDisc, std::string{ "CkustersCollectionWithNoAttrDisc" } },
        { State::ClustersInterviewWithExtraction, std::string{ "ClustersInterviewWithExtraction" } },
        { State::CheckInIntervalExtraction, std::string{ "CheckInIntervalExtraction" } },
        { State::DiscoverCommandsReceived, std::string{ "DiscoverCommandsReceived" } },
        { State::DiscoverCommandsGenerated, std::string{ "DiscoverCommandsGenerated" } },
        { State::CollectClustersToBind, std::string{ "CollectClustersToBind" } },
        { State::ClustersBinding, std::string{ "ClustersBinding" } },
        { State::MfgInfoExtractionPrepare, std::string{ "MfgInfoExtractionPrepare" } },
        { State::MfgAttributesDiscovery, std::string{ "MfgAttributesDiscovery" } },
        { State::MfgClustersInterview, std::string{ "MfgClustersInterview" } },
        { State::InterviewDone, std::string{ "InterviewDone" } },
    };
    const auto it = names.find( state );
    return ( it != names.end() ? it->second : unknownName );
}

}  // namespace

HandlerInterview::BindingEntity HandlerInterview::BindingEntity::InvalidBindingEntity{ 0xFF, 0xFFFF };

HandlerInterview::BindingEntity::BindingEntity( uint8_t endpointId, uint16_t clusterId )
: m_endpointId( endpointId )
, m_clusterId( clusterId ) {}

HandlerInterview::BindingEntity::BindingEntity( HandlerInterview::BindingEntity&& fromMove )
: m_endpointId( std::move( fromMove.m_endpointId ) )
, m_clusterId( std::move( fromMove.m_clusterId ) ) {}

uint8_t HandlerInterview::BindingEntity::getEndpointId() const {
    return m_endpointId;
}

uint16_t HandlerInterview::BindingEntity::getClusterId() const {
    return m_clusterId;
}

HandlerInterview::HandlerInterview( ZigbeeAddonCallbackInterface& callbackInterface,
                                    CommandsSenderInterface& commandsSender )
: m_state( State::Idle )
, m_currentProgress( 0 )
, m_nodeId( 0 )
, m_callbackInterface( callbackInterface )
, m_commandsSender( commandsSender )
, m_bindingRetriesCount( 0 )
, m_currentCluster( 0 )
, m_clusterRetriesCount( 0 )
, m_stepTimer( [this]() {
    ZigbeeTaskLoop::getInstance().addTask( [this]() {
        Log::debug( "Inclusion step timeout for node: " + std::to_string( m_nodeId ) );
        switch( m_state ) {
        case State::Discovery:
            m_callbackInterface.includeStatusChange( ModeStatus::Timeout );
            break;

        case State::ManufacturerInfoExtraction:
        case State::AttributesDiscovery:
        case State::ClustersInterview:
        case State::ClustersInterviewWithExtraction:
        case State::ClustersBinding:
        case State::DiscoverCommandsReceived:
        case State::DiscoverCommandsGenerated:
            ++m_clusterRetriesCount;
            executeCurrentStep( false );
            break;

        default:
            assert( !"Unsupported state" );
        }
    } );
} ) {
    m_nodeInterviewNamesExtractors = m_callbackInterface.provideNodeInterviewNamesExtractors();
}

void HandlerInterview::executeStep( State state ) {
    Log::print(
        Log::PriorityLevel::DEBUG, cc::success( "ExecuteStep: %s" ).c_str(), stepStateToString( state ).c_str() );
    sendProgress( state );

    m_state = state;
    m_stepTimer.stop();

    switch( m_state ) {
    case State::ClustersCollection:
        m_clusters.clear();
        resetClustersProcessing();
        m_node->forEachEndpoint( [this]( ZigbeeEndpoint& endpoint ) -> bool {
            return endpoint.forEachCluster( [this]( ZigbeeClusterBase& cluster ) -> bool {
                m_clusters.push_back( &cluster );
                return true;
            } );
        } );
        executeNextStep( true );
        break;

    case State::ManufacturerInfoExtraction:
        executeManufacturerInfoExtraction();
        break;

    case State::AttributesDiscovery:
        executeAttributesDiscovery();
        break;

    case State::ClustersInterview:
        executeClustersInterview( false );
        break;

    case State::ClustersCollectionWithNoAttrDisc:
        executeClustersCollectionWithNoAttrDisc();
        executeNextStep( true );
        break;

    case State::ClustersInterviewWithExtraction:
        executeClustersInterview( true );
        break;

    case State::CheckInIntervalExtraction:
        executeCheckInIntervalExtraction();
        break;

    case State::DiscoverCommandsReceived:
    case State::DiscoverCommandsGenerated:
        executeDiscoverCommands();
        break;

    case State::CollectClustersToBind:
        collectClustersToBind();
        executeNextStep( true );
        break;

    case State::ClustersBinding:
        ++m_bindingRetriesCount;
        bindNextEntity();
        break;

    case State::MfgInfoExtractionPrepare:
        executeMfgInfoExtractionPrepare();
        break;

    case State::MfgAttributesDiscovery:
        executeMfgAttributesDiscovery();
        break;

    case State::MfgClustersInterview:
        executeMfgClustersInterview();
        break;

    case State::InterviewDone: {
        interviewDone( ModeStatus::Finished );
        executeStep( State::Idle );
        return;
    }
    case State::Idle:
        // moving to this step simply stops step timer
        return;
    }
    m_stepTimer.start( getStepTimeout(), true );
}

void HandlerInterview::executeStepAsync( State state ) {
    ZigbeeTaskLoop::getInstance().addTask( [this, state]() { executeStep( state ); } );
}

void HandlerInterview::executeCurrentStep( bool async ) {
    if( async ) {
        executeStepAsync( m_state );
    }
    else {
        executeStep( m_state );
    }
}

void HandlerInterview::executeNextStep( bool async ) {
    static std::unordered_map< State, State > nextStates{
        { State::Discovery, State::ClustersCollection },
        { State::ClustersCollection, State::ManufacturerInfoExtraction },
        { State::ManufacturerInfoExtraction, State::AttributesDiscovery },
        { State::AttributesDiscovery, State::ClustersInterview },
        { State::ClustersInterview, State::ClustersCollectionWithNoAttrDisc },
        { State::ClustersCollectionWithNoAttrDisc, State::ClustersInterviewWithExtraction },
        { State::ClustersInterviewWithExtraction, State::CheckInIntervalExtraction },
        { State::CheckInIntervalExtraction, State::DiscoverCommandsReceived },
        { State::DiscoverCommandsReceived, State::DiscoverCommandsGenerated },
        { State::DiscoverCommandsGenerated, State::CollectClustersToBind },
        { State::CollectClustersToBind, State::ClustersBinding },
        { State::ClustersBinding, State::MfgInfoExtractionPrepare },
        { State::MfgInfoExtractionPrepare, State::MfgAttributesDiscovery },
        { State::MfgAttributesDiscovery, State::MfgClustersInterview },
        { State::MfgClustersInterview, State::InterviewDone },
    };

    auto nextStepIt = nextStates.find( m_state );
    if( nextStepIt != nextStates.end() ) {
        if( async ) {
            executeStepAsync( nextStepIt->second );
        }
        else {
            executeStep( nextStepIt->second );
        }
    }
}

void HandlerInterview::processNextCluster() {
    finishClusterProcessing();
    executeCurrentStep( true );
}

void HandlerInterview::executeManufacturerInfoExtraction() {
    if( m_node->getManufacturerId().empty() || m_node->getModelId().empty() ) {
        auto* currentCluster = getCurrentCluster();
        if( currentCluster ) {
            if( currentCluster->getId() != ZCL_BASIC_CLUSTER_ID ) {
                processNextCluster();
                return;
            }

            executeRepeatedRequest( [this, currentCluster]() -> bool {
                std::vector< uint16_t > attributesToRequest;

                if( m_node->getManufacturerId().empty() ) {
                    attributesToRequest.push_back(
                        static_cast< uint16_t >( BasicClusterAttributeId::ManufacturerName ) );
                }

                if( m_node->getModelId().empty() ) {
                    attributesToRequest.push_back(
                        static_cast< uint16_t >( BasicClusterAttributeId::ModelIdentifier ) );
                }

                return m_commandsSender.requestAttributes(
                    m_nodeId, currentCluster->getEndpointId(), currentCluster->getId(), attributesToRequest );
            } );
            return;
        }
    }

    if( m_node->getManufacturerId().empty() ) {
        m_node->setManufacturerId( Resources::Fields::Node::Values::Unknown );
    }

    if( m_node->getModelId().empty() ) {
        m_node->setModelId( Resources::Fields::Node::Values::Unknown );
    }

    Log::info( cc::info( "Interviewing device: " + m_node->getManufacturerId() + "_" + m_node->getModelId() ) );

    m_nodeInterviewConfigurations =
        m_callbackInterface.provideNodeInterviewConfigurations( m_node->getManufacturerId(), m_node->getModelId() );
    resetClustersProcessing();
    if( m_nodeInterviewConfigurations ) {
        m_clusters.erase( std::remove_if( m_clusters.begin(),
                                          m_clusters.end(),
                                          [this]( const ZigbeeClusterBase* cluster ) {
                                              return m_nodeInterviewConfigurations->fullyIgnoreCluster(
                                                  cluster->getEndpointId(), cluster->getId() );
                                          } ),
                          m_clusters.end() );
    }

    executeNextStep( true );
}

void HandlerInterview::executeAttributesDiscovery() {
    auto* currentCluster = getCurrentCluster();
    if( currentCluster ) {
        if( !isFromSupportedProfile( *currentCluster ) ) {
            processNextCluster();
            return;
        }

        if( m_nodeInterviewConfigurations && !m_nodeInterviewConfigurations->discoverClusterAttributes(
                                                 currentCluster->getEndpointId(), currentCluster->getId() ) ) {
            processNextCluster();
            return;
        }

        executeRepeatedRequest( [this, currentCluster]() -> bool {
            const uint16_t* lastAttrId = currentCluster->getLastAttributeId();

            return discoverAttributes( m_nodeId,
                                       currentCluster->getEndpointId(),
                                       currentCluster->getId(),
                                       lastAttrId ? *lastAttrId + 1 : 0x00 );
        } );
        return;
    }

    resetClustersProcessing();
    executeNextStep( true );
}

void HandlerInterview::executeClustersCollectionWithNoAttrDisc() {
    m_clustersWithNoAttrDisc.clear();

    for( int i = 0; i < m_clusters.size(); ++i ) {
        if( m_clusters.at( i )->getAttributesCount() == 0 ) {
            m_clustersWithNoAttrDisc.insert( i );
        }
    }
}

void HandlerInterview::executeClustersInterview( bool withExtraction ) {
    auto* currentCluster = getCurrentCluster();
    if( currentCluster ) {
        executeRepeatedRequest( [this, currentCluster, withExtraction]() -> bool {
            auto result =
                requestClusterAttributes( m_nodeId, currentCluster->getEndpointId(), *currentCluster, withExtraction );

            if( result == AttributesRequestStatus::NoNeed ) {
                processNextCluster();
                return true;
            }

            return result != AttributesRequestStatus::Failed;
        } );
        return;
    }

    resetClustersProcessing();
    executeNextStep( true );
}

void HandlerInterview::executeCheckInIntervalExtraction() {
    auto endpoint = m_node->findEndpoint( 1 );
    if( endpoint ) {
        auto cluster = endpoint->findCluster( static_cast< uint16_t >( ClusterId::PollControl ) );
        if( cluster ) {
            auto attribute =
                cluster->findAttribute( static_cast< uint16_t >( PollControlClusterAttributeId::CheckInInterval ) );
            if( attribute ) {
                uint32_t value = strtoul( attribute->getValue().c_str(), NULL, 10 );
                if( value != 0 ) {
                    // Double CheckInInterval to have less false actions
                    m_node->setReachableStateRecheckTimeout( std::chrono::seconds( value / 4 ) * 2 );
                }
            }
        }
    }

    executeNextStep( true );
}

void HandlerInterview::executeDiscoverCommands() {
    auto* currentCluster = getCurrentCluster();
    if( currentCluster ) {
        if( !isFromSupportedProfile( *currentCluster ) ) {
            processNextCluster();
            return;
        }

        if( m_nodeInterviewConfigurations ) {
            auto functionToCheckConfigurations = m_state == State::DiscoverCommandsGenerated ?
                                                     &NodeInterviewConfigurations::discoverClusterCommandsGenerated :
                                                     &NodeInterviewConfigurations::discoverClusterCommandsReceived;
            if( !( m_nodeInterviewConfigurations.get()->*functionToCheckConfigurations )(
                    currentCluster->getEndpointId(), currentCluster->getId() ) ) {
                processNextCluster();
                return;
            }
        }

        executeRepeatedRequest( [this, currentCluster]() -> bool {
            const uint8_t* lastCommandId = currentCluster->getLastCommandId( getCurrentCommandType() );
            return discoverCommands( m_nodeId,
                                     currentCluster->getEndpointId(),
                                     currentCluster->getId(),
                                     currentCluster->getDataHolder(),
                                     lastCommandId ? *lastCommandId + 1 : 0x00 );
        } );
        return;
    }

    resetClustersProcessing();
    executeNextStep( true );
}

void HandlerInterview::collectClustersToBind() {
    for( const auto* cluster : m_clusters ) {
        if( !isFromSupportedProfile( *cluster ) ) {
            continue;
        }

        if( cluster->isServer() && cluster->isBindable() ) {
            auto endpointId = cluster->getEndpointId();
            auto clusterId = cluster->getId();

            if( !m_nodeInterviewConfigurations ||
                !m_nodeInterviewConfigurations->fullyIgnoreCluster( endpointId, clusterId ) ) {
                BindingEntity item{ endpointId, cluster->getId() };
                m_bindingList.push_back( std::move( item ) );
            }
        }
    }
}

void HandlerInterview::executeMfgInfoExtractionPrepare() {
    if( !m_nodeInterviewConfigurations || !m_nodeInterviewConfigurations->getMfgCode().has_value() ) {
        executeStepAsync( State::InterviewDone );
        return;
    }

    m_clusters.erase( std::remove_if( m_clusters.begin(),
                                      m_clusters.end(),
                                      [this]( const ZigbeeClusterBase* cluster ) {
                                          return !m_nodeInterviewConfigurations->shouldDiscoverMfgInfo(
                                              cluster->getEndpointId(), cluster->getId() );
                                      } ),
                      m_clusters.end() );

    resetClustersProcessing();
    executeNextStep( true );
}

void HandlerInterview::executeMfgAttributesDiscovery() {
    auto* currentCluster = getCurrentCluster();
    if( currentCluster ) {
        executeRepeatedRequest( [this, currentCluster]() -> bool {
            const uint16_t* lastAttrId = currentCluster->getLastMfgSpecificAttributeId();
            return discoverAttributes( m_nodeId,
                                       currentCluster->getEndpointId(),
                                       currentCluster->getId(),
                                       lastAttrId ? *lastAttrId + 1 : 0x00,
                                       *m_nodeInterviewConfigurations->getMfgCode() );
        } );
        return;
    }

    resetClustersProcessing();
    executeNextStep( true );
}

void HandlerInterview::executeMfgClustersInterview() {
    auto* currentCluster = getCurrentCluster();
    if( currentCluster ) {
        executeRepeatedRequest( [this, currentCluster]() -> bool {
            std::vector< uint16_t > attributesToRequest;
            currentCluster->forEachAttribute( [&attributesToRequest]( ZigbeeAttribute& attribute ) -> bool {
                if( attribute.isMfgSpecific() && !attribute.hasActualValue() ) {
                    attributesToRequest.push_back( attribute.getId() );
                }
                // Avoid buffer overload
                return attributesToRequest.size() < attributesInSingleRequestCountMax;
            } );

            if( attributesToRequest.empty() ) {
                processNextCluster();
                return true;
            }

            return m_commandsSender.requestAttributes( m_nodeId,
                                                       currentCluster->getEndpointId(),
                                                       currentCluster->getId(),
                                                       attributesToRequest,
                                                       true,
                                                       *m_nodeInterviewConfigurations->getMfgCode() );
        } );
        return;
    }

    resetClustersProcessing();
    executeNextStep( true );
}

std::chrono::seconds HandlerInterview::getStepTimeout() const {
    switch( m_state ) {
    case State::Discovery:
        return discoveringStepTimeoutSeconds;

    case State::ClustersInterviewWithExtraction:
    case State::ClustersBinding:
        return stepTimeoutLongSeconds;

    default:
        return stepTimeoutShortSeconds;
    }
}

void HandlerInterview::trustCenterJoinCallback( EmberNodeId nodeId,
                                                uint8_t* newNodeEui64,
                                                EmberNodeId parentOfNewNode,
                                                EmberDeviceUpdate status,
                                                EmberJoinDecision decision ) {
    if( EMBER_NO_ACTION == decision ) {
        // TODO
    }
    else if( EMBER_USE_PRECONFIGURED_KEY == decision || EMBER_SEND_KEY_IN_THE_CLEAR == decision ) {
        // TODO
    }
    else {
        // TODO
    }

    if( EMBER_DEVICE_LEFT == status ) {
        Log::error( "Try to delete node during its inclusion, node_id: " + std::to_string( nodeId ) );
    }
    else {
        Log::debug( "Device join / rejoin with id: " + std::to_string( nodeId ) );
        const bool hasSecurity = ( EMBER_STANDARD_SECURITY_SECURED_REJOIN == status );
        m_nodeId = nodeId;
        m_node = std::make_shared< ZigbeeNode >( nodeId, hasSecurity, false, false );
        m_callbackInterface.includeStatusChange( ModeStatus::Started );
        executeStep( State::Discovery );
    }
}

bool HandlerInterview::preZDOMessageReceivedCallback( EmberNodeId emberNodeId,
                                                      EmberApsFrame* apsFrame,
                                                      uint8_t* message,
                                                      uint16_t length ) {
    if( END_DEVICE_ANNOUNCE == apsFrame->clusterId ) {
        const uint8_t capabilities = message[ DEVICE_ANNOUNCE_CAPABILITIES_OFFSET ];
        Log::print( Log::PriorityLevel::DEBUG,
                    "End device announce: nodeId 0x%04x capabilities 0x%02x",
                    emberNodeId,
                    capabilities );

        if( m_node ) {
            m_node->setCapabilities( capabilities );
        }
        if( ( capabilities & RECEIVER_ON_WHEN_IDLE ) == 0 ) {
            emAfSetFastPollingMode( true );
        }
    }
    else if( BIND_RESPONSE == apsFrame->clusterId ) {
        // TODO match apsFrame->sequence between request and response
        Log::print( Log::PriorityLevel::DEBUG,
                    "Binding response received: sequence - 0x%02x, status - 0x%02x",
                    apsFrame->sequence,
                    message[ 1 ] );

        m_bindingRetriesCount = 0;
        m_bindingList.pop_back();
        executeCurrentStep( true );
    }
    return false;
}

bool HandlerInterview::readAttributesResponseCallback( EmberNodeId nodeId,
                                                       EmberAfClusterId clusterId,
                                                       uint8_t endpointId,
                                                       uint8_t* buffer,
                                                       uint16_t bufLen ) {
    Log::print( Log::PriorityLevel::DEBUG,
                "Attributes response received: nodeId: 0x%04x, endpointId 0x%02x, clusterId: 0x%04x",
                nodeId,
                endpointId,
                clusterId );

    switch( m_state ) {
    case State::ManufacturerInfoExtraction:
    case State::ClustersInterview:
    case State::ClustersInterviewWithExtraction:
    case State::MfgClustersInterview:
        break;
    default:
        return true;
    }

    auto* currentCluster = getCurrentCluster();
    if( !currentCluster || currentCluster->getEndpointId() != endpointId || currentCluster->getId() != clusterId ) {
        Log::print( Log::PriorityLevel::DEBUG, "Attributes from another cluster received" );
        return true;
    }

    bool receivedValue = false;

    using Callback = std::function< void( uint16_t, AttributeValueType, std::string, bool ) >;

    Callback manufacturerExtractionCallback =
        [this, &receivedValue](
            uint16_t attributeId, AttributeValueType valueType, std::string value, bool actualValue ) {
            using NameExtractorFuntion =
                common::optional< std::string > ( NodeInterviewNamesExtractors::* )( const std::string& ) const;
            using NameSetterFunction = void ( ZigbeeNode::* )( const std::string& );
            auto callback = [this, &receivedValue, &value]( NameExtractorFuntion nameExtractor,
                                                            NameSetterFunction nameSetter,
                                                            const std::string& fieldName ) {
                if( m_nodeInterviewNamesExtractors ) {
                    auto extractedName = ( m_nodeInterviewNamesExtractors.get()->*nameExtractor )( value );
                    if( extractedName ) {
                        Log::print( Log::PriorityLevel::DEBUG,
                                    "Extracted %s name \"%s\" from received \"%s\"",
                                    fieldName.c_str(),
                                    extractedName->c_str(),
                                    value.c_str() );
                        value = *extractedName;
                    }
                }
                ( m_node.get()->*nameSetter )( value );
                receivedValue = true;
            };

            switch( attributeId ) {
            case static_cast< uint16_t >( BasicClusterAttributeId::ManufacturerName ):
                callback( &NodeInterviewNamesExtractors::getExtractedManufacturerName,
                          &ZigbeeNode::setManufacturerId,
                          "manufacturer" );
                break;
            case static_cast< uint16_t >( BasicClusterAttributeId::ModelIdentifier ):
                callback( &NodeInterviewNamesExtractors::getExtractedModelName, &ZigbeeNode::setModelId, "model" );
                break;
            }
        };

    Callback attributesInterviewCallback =
        [this, &currentCluster, &receivedValue, &clusterId](
            uint16_t attributeId, AttributeValueType valueType, std::string value, bool actualValue ) {
            // TODO support case with failure during attribute parse
            auto* attribute = currentCluster->findAttribute( attributeId );
            if( !attribute ) {
                auto attributeInstance = ZigbeeAttributeFactory::createZigbeeAttribute(
                    clusterId, attributeId, valueType, m_state == State::MfgClustersInterview /*isMfgSpecific*/ );

                currentCluster->addAttribute( std::move( attributeInstance ) );
                attribute = currentCluster->findAttribute( attributeId );
            }

            attribute->setValue( value );
            receivedValue = true;
        };

    // parse attributes
    AttributesParser::parseAttributesResponse(
        buffer,
        bufLen,
        m_state == State::ManufacturerInfoExtraction ? manufacturerExtractionCallback : attributesInterviewCallback,
        [this]( uint16_t attributeId ) { m_unsupportedClusterAttributes.insert( attributeId ); } );

    if( !receivedValue ) {
        finishClusterProcessing();
    }

    executeCurrentStep( false );
    return true;
}

bool HandlerInterview::attributesDiscoveryResponseCallback( EmberNodeId emberNodeId,
                                                            EmberAfClusterId clusterId,
                                                            uint8_t endpointId,
                                                            uint8_t* buffer,
                                                            uint16_t bufLen,
                                                            bool extended ) {
    Log::print( Log::PriorityLevel::DEBUG,
                "Attributes discovery response received: nodeId: 0x%04x, endpointId 0x%02x, clusterId: 0x%04x",
                emberNodeId,
                endpointId,
                clusterId );

    if( m_state != State::AttributesDiscovery && m_state != State::MfgAttributesDiscovery ) {
        return true;
    }

    auto* currentCluster = getCurrentCluster();
    if( !currentCluster || currentCluster->getEndpointId() != endpointId || currentCluster->getId() != clusterId ) {
        Log::print( Log::PriorityLevel::DEBUG, "Attributes from another cluster received" );
        return true;
    }

    auto attributes = parseAttributesDiscovery( clusterId, buffer, bufLen, extended );
    for( auto& item : attributes ) {
        currentCluster->addAttribute( std::move( item.second ) );
    }
    if( attributes.empty() ) {
        finishClusterProcessing();
    }

    executeCurrentStep( false );
    return true;
}

bool HandlerInterview::commandsDiscoveryResponseCallback( EmberNodeId emberNodeId,
                                                          EmberAfClusterId clusterId,
                                                          uint8_t endpointId,
                                                          CommandType commandsType,
                                                          uint8_t* commandIds,
                                                          uint16_t commandIdCount ) {
    Log::print( Log::PriorityLevel::DEBUG,
                "Commands discovery response received: nodeId: 0x%04x, endpointId 0x%02x, clusterId: 0x%04x",
                emberNodeId,
                endpointId,
                clusterId );

    if( m_state != State::DiscoverCommandsGenerated && m_state != State::DiscoverCommandsReceived ) {
        Log::print( Log::PriorityLevel::DEBUG,
                    "Wrong state for commands discovery processing: %s",
                    stepStateToString( m_state ).c_str() );
        return true;
    }

    if( commandsType != getCurrentCommandType() ) {
        Log::print( Log::PriorityLevel::DEBUG, "Wrong commands type received: %s", toString( commandsType ) );
        return true;
    }

    auto* currentCluster = getCurrentCluster();
    if( !currentCluster || currentCluster->getEndpointId() != endpointId || currentCluster->getId() != clusterId ) {
        Log::print( Log::PriorityLevel::DEBUG, "Commands from another cluster received" );
        return true;
    }

    for( int i = 0; i < commandIdCount; ++i ) {
        uint8_t commandId = commandIds[ i ];
        auto command =
            ZigbeeCommandFactory::createZigbeeCommand( clusterId,
                                                       commandId,
                                                       toString( static_cast< ClusterId >( currentCluster->getId() ),
                                                                 currentCluster->getDataHolder(),
                                                                 commandsType,
                                                                 commandId ),
                                                       commandsType,
                                                       false );

        currentCluster->addCommand( std::move( command ) );
    }

    if( commandIdCount == 0 ) {
        finishClusterProcessing();
    }

    executeCurrentStep( false );
    return true;
}

bool HandlerInterview::defaultResponseCallback( EmberNodeId emberNodeId,
                                                EmberAfClusterId clusterId,
                                                uint8_t endpointId,
                                                uint8_t commandId,
                                                EmberAfStatus status ) {
    Log::print( Log::PriorityLevel::DEBUG,
                "Default response received: nodeId: 0x%04x, endpointId 0x%02x, clusterId: 0x%04x, commandId 0x%02x, "
                "status: 0x%04x",
                emberNodeId,
                endpointId,
                clusterId,
                commandId,
                static_cast< uint16_t >( status ) );

    if( status == EMBER_ZCL_STATUS_UNSUP_GENERAL_COMMAND ) {
        switch( m_state ) {
        case State::DiscoverCommandsGenerated:
        case State::DiscoverCommandsReceived:
            resetClustersProcessing();
            executeStep( State::CollectClustersToBind );
            break;

        case State::AttributesDiscovery:
            resetClustersProcessing();
            executeStep( State::ClustersCollectionWithNoAttrDisc );
            break;

        case State::ClustersBinding:
            executeStep( State::MfgInfoExtractionPrepare );
            break;
        }
    }

    return true;
}

void HandlerInterview::discoveryCompleteCallback( const EmberAfDeviceInfo* device ) {
    ZB_LOG_DEBUG( "%s\n", __PRETTY_FUNCTION__ );
    ZigbeeNode::Endpoints eps;
    ZB_LOG_DEBUG( "\tDevice: nodeId - 0x%04x, stack - 0x%02x, capabilities - 0x%02x, endpointCount - %u\n",
                  m_node->getId(),
                  device->stackRevision,
                  device->capabilities,
                  device->endpointCount );
    for( auto i = 0; i < device->endpointCount; ++i ) {
        const EmberAfEndpointInfoStruct& ep = device->endpoints[ i ];
        ZB_LOG_DEBUG( "\tEP[%02d]: endpoint - %u, deviceId - 0x%04x, profileId - 0x%04x, clusterCount - %u\n",
                      i,
                      ep.endpoint,
                      ep.deviceId,
                      ep.profileId,
                      ep.clusterCount );

        ZigbeeNode::EndpointUPtr epInstance =
            std::make_unique< ZigbeeEndpoint >( ep.endpoint, ep.profileId, ep.deviceId );
        for( auto j = 0; j < ep.clusterCount; ++j ) {
            const EmberAfClusterInfo& cl = ep.clusters[ j ];
            ZB_LOG_DEBUG(
                "\tCL[%02d]: clusterId - 0x%04x, server - %s\n", j, cl.clusterId, ( cl.server ? "true" : "false" ) );

            auto* existingCluster = epInstance->findCluster( cl.clusterId );
            if( !existingCluster ) {
                ZB_LOG_DEBUG( "\tCL[%02d]: create cluster %u\n", j, cl.clusterId );
                const ZigbeeClusterBase::ClusterDataHolder dataHolder =
                    cl.server ? ZigbeeClusterBase::ClusterDataHolder::Server :
                                ZigbeeClusterBase::ClusterDataHolder::Client;
                ZigbeeEndpoint::ClusterBaseUPtr clInstance =
                    std::make_unique< ZigbeeClusterBase >( cl.clusterId, ep.endpoint, dataHolder );
                epInstance->addCluster( std::move( clInstance ) );
            }
            else {
                ZB_LOG_DEBUG(
                    "\tCL[%02d]: id %u set dataholder %s\n", j, cl.clusterId, ( cl.server ? "true" : "false" ) );
                existingCluster->setDataHolder( cl.server ? ZigbeeClusterBase::ClusterDataHolder::Server :
                                                            ZigbeeClusterBase::ClusterDataHolder::Client );
            }
        }
        m_node->addEndpoint( std::move( epInstance ) );
    }
    m_node->setDiscovered( true );
    // start cluster interview
    executeNextStep( false );
}

void HandlerInterview::configureReportingResponseCallback( uint16_t nodeId,
                                                           uint8_t endpointId,
                                                           uint16_t clusterId,
                                                           uint8_t* buffer,
                                                           uint16_t bufLen ) {
    Log::print( Log::PriorityLevel::DEBUG,
                "Attribute configure response received: nodeId 0x%04x, endpointId 0x%02x, clusterId 0x%04x",
                nodeId,
                endpointId,
                clusterId );
}

HandlerInterview::AttributesRequestStatus HandlerInterview::requestClusterAttributes( EmberNodeId nodeId,
                                                                                      uint8_t endpointId,
                                                                                      ZigbeeClusterBase& cluster,
                                                                                      bool withExtraction ) {
    std::vector< uint16_t > attributesToRequest;

    if( withExtraction ) {
        if( m_clustersWithNoAttrDisc.find( m_currentCluster ) == m_clustersWithNoAttrDisc.end() ) {
            return AttributesRequestStatus::NoNeed;
        }

        auto callback = [&attributesToRequest, &cluster, this]( uint16_t attrId ) -> bool {
            if( !cluster.findAttribute( attrId ) &&
                m_unsupportedClusterAttributes.find( attrId ) == m_unsupportedClusterAttributes.end() ) {
                attributesToRequest.push_back( attrId );
            }

            return attributesToRequest.size() < attributesInSingleRequestCountMax;
        };

        if( m_nodeInterviewConfigurations &&
            !m_nodeInterviewConfigurations->discoverClusterAttributes( cluster.getEndpointId(), cluster.getId() ) ) {
            m_nodeInterviewConfigurations->forEachClusterAttributesToRequest(
                cluster.getEndpointId(), cluster.getId(), callback );
        }
        else {
            forEachAttributeId( static_cast< ClusterId >( cluster.getId() ), callback );
        }
    }
    else {
        cluster.forEachAttribute( [&attributesToRequest]( ZigbeeAttribute& attribute ) -> bool {
            if( !attribute.hasActualValue() ) {
                attributesToRequest.push_back( attribute.getId() );
            }
            // Avoid buffer overload
            return attributesToRequest.size() < attributesInSingleRequestCountMax;
        } );
    }

    if( attributesToRequest.empty() ) {
        return AttributesRequestStatus::NoNeed;
    }

    bool result = m_commandsSender.requestAttributes( nodeId, endpointId, cluster.getId(), attributesToRequest );
    return result ? AttributesRequestStatus::Success : AttributesRequestStatus::Failed;
}

bool HandlerInterview::discoverAttributes( EmberNodeId nodeId,
                                           uint8_t endpointId,
                                           EmberAfClusterId clusterId,
                                           uint16_t startAttribute,
                                           uint16_t mfgCode ) {
    Log::print( Log::PriorityLevel::DEBUG,
                "Try to discover attributes: nodeId 0x%04x, endpointId 0x%02x, clusterId 0x%04x, mfgCode 0x%04x",
                m_node->getId(),
                endpointId,
                clusterId,
                mfgCode );

    return m_commandsSender.discoverAttributes(
        nodeId, endpointId, clusterId, startAttribute, 0xff, m_state == State::MfgAttributesDiscovery, mfgCode );
}

bool HandlerInterview::discoverCommands( EmberNodeId nodeId,
                                         uint8_t endpointId,
                                         EmberAfClusterId clusterId,
                                         ZigbeeClusterBase::ClusterDataHolder dataHolder,
                                         uint16_t startCommand ) {
    Log::print( Log::PriorityLevel::DEBUG,
                "Try to discover commands: nodeId 0x%04x, endpointId 0x%02x, clusterId 0x%04x",
                m_node->getId(),
                endpointId,
                clusterId );

    auto commandDirection = dataHolder == ZigbeeClusterBase::ClusterDataHolder::Server ?
                                CommandsSenderInterface::CommandDirection::ClientToServer :
                                CommandsSenderInterface::CommandDirection::ServerToClient;

    if( m_state == State::DiscoverCommandsReceived ) {
        return m_commandsSender.discoverCommandsReceived(
            nodeId, endpointId, clusterId, startCommand, 0xff, commandDirection );
    }
    else {
        assert( m_state == State::DiscoverCommandsGenerated );
        return m_commandsSender.discoverCommandsGenerated(
            nodeId, endpointId, clusterId, startCommand, 0xff, commandDirection );
    }
}

void HandlerInterview::bindNextEntity( bool timeout ) {
    if( m_bindingRetriesCount > bindingRetriesCountMax && !m_bindingList.empty() ) {
        m_bindingRetriesCount = 0;
        m_bindingList.pop_back();
    }

    if( !m_bindingList.empty() ) {
        auto& entity = m_bindingList.back();
        Log::print( Log::PriorityLevel::DEBUG,
                    "Try ( %u ) send bind: nodeId 0x%04x, endpointId 0x%02x, clusterId 0x%04x",
                    m_bindingRetriesCount,
                    m_node->getId(),
                    entity.getEndpointId(),
                    entity.getClusterId() );

        m_commandsSender.sendBindingCommand( m_node->getId(), entity.getEndpointId(), entity.getClusterId() );
    }
    else {
        resetClustersProcessing();
        executeNextStep( false );
    }
}

void HandlerInterview::interviewDone( ModeStatus interviewStatus ) {
    if( interviewStatus == ModeStatus::Finished ) {
        emAfSetFastPollingMode( false );
        m_callbackInterface.nodeAdded( m_node );
    }

    m_callbackInterface.includeStatusChange( interviewStatus );
}

ZigbeeAttribute::Attributes HandlerInterview::parseAttributesDiscovery( EmberAfClusterId clusterId,
                                                                        uint8_t* buffer,
                                                                        uint16_t bufLen,
                                                                        bool extended ) {
    ZigbeeAttribute::Attributes attributes;

    bool isMfgSpecific = m_state == State::MfgAttributesDiscovery;

    DiscoveryParser::parseAttributesDiscoveryResponse(
        buffer,
        bufLen,
        extended,
        [this, clusterId, &attributes, isMfgSpecific]( uint16_t attributeId, AttributeValueType valueType, uint8_t* ) {
            auto attributeInstance =
                ZigbeeAttributeFactory::createZigbeeAttribute( clusterId, attributeId, valueType, isMfgSpecific );

            if( isMfgSpecific ) {
                attributeInstance->setMfgCode( *m_nodeInterviewConfigurations->getMfgCode() );
            }

            attributes.emplace( attributeId, std::move( attributeInstance ) );
        } );

    return attributes;
}

bool HandlerInterview::isFromSupportedProfile( const ZigbeeClusterBase& cluster ) const {
    return isSupportedProfile( m_node->findEndpoint( cluster.getEndpointId() )->getProfileId() );
}

bool HandlerInterview::isSupportedProfile( uint16_t id ) const {
    switch( id ) {
    case CBA_PROFILE_ID:
    case HA_PROFILE_ID:
    case SE_PROFILE_ID:
        return true;

    default:
        return false;
    }
}

void HandlerInterview::finishClusterProcessing() {
    m_clusterRetriesCount = 0;
    m_unsupportedClusterAttributes.clear();
    ++m_currentCluster;
}

void HandlerInterview::resetClustersProcessing() {
    m_clusterRetriesCount = 0;
    m_unsupportedClusterAttributes.clear();
    m_currentCluster = 0;
}

ZigbeeClusterBase* HandlerInterview::getCurrentCluster() {
    return m_currentCluster < m_clusters.size() ? m_clusters.at( m_currentCluster ) : nullptr;
}

CommandType HandlerInterview::getCurrentCommandType() const {
    assert( m_state == State::DiscoverCommandsGenerated || m_state == State::DiscoverCommandsReceived );
    return m_state == State::DiscoverCommandsGenerated ? CommandType::Generated : CommandType::Received;
}

void HandlerInterview::sendProgress( State state ) {
    auto newProgress = getCurrentProgress( state );

    if( newProgress > m_currentProgress && newProgress - m_currentProgress >= minimalProgressChange ) {
        m_currentProgress = newProgress;

        m_callbackInterface.includeDeviceProgress( m_currentProgress );
    }
}

uint8_t HandlerInterview::getCurrentProgress( State state ) const {
    static const std::unordered_map< State, uint8_t > percentOnStart = { { State::Idle, 0 },
                                                                         { State::Discovery, 0 },
                                                                         { State::ClustersCollection, 10 },
                                                                         { State::ManufacturerInfoExtraction, 10 },
                                                                         { State::AttributesDiscovery, 10 },
                                                                         { State::ClustersInterview, 25 },
                                                                         { State::ClustersCollectionWithNoAttrDisc,
                                                                           35 },
                                                                         { State::ClustersInterviewWithExtraction, 35 },
                                                                         { State::CheckInIntervalExtraction, 45 },
                                                                         { State::DiscoverCommandsReceived, 45 },
                                                                         { State::DiscoverCommandsGenerated, 55 },
                                                                         { State::CollectClustersToBind, 65 },
                                                                         { State::ClustersBinding, 65 },
                                                                         { State::MfgInfoExtractionPrepare, 75 },
                                                                         { State::MfgAttributesDiscovery, 75 },
                                                                         { State::MfgClustersInterview, 85 },
                                                                         { State::InterviewDone, 95 } };

    static const std::unordered_map< State, uint8_t > fullPhasePercents = { { State::AttributesDiscovery, 15 },
                                                                            { State::ClustersInterview, 10 },
                                                                            { State::ClustersInterviewWithExtraction,
                                                                              10 },
                                                                            { State::DiscoverCommandsReceived, 10 },
                                                                            { State::DiscoverCommandsGenerated, 10 },
                                                                            { State::MfgAttributesDiscovery, 10 },
                                                                            { State::MfgClustersInterview, 10 } };

    auto percentData = percentOnStart.find( state );
    assert( percentData != percentOnStart.end() );
    auto percents = percentData->second;

    if( fullPhasePercents.find( state ) != fullPhasePercents.end() ) {
        percents += std::floor( fullPhasePercents.find( state )->second / m_clusters.size() * m_currentCluster );
    }

    return percents;
}

void HandlerInterview::executeRepeatedRequest( std::function< bool() > request ) {
    while( true ) {
        if( m_clusterRetriesCount == attributesRequestRetriesCountMax ) {
            processNextCluster();
            return;
        }

        if( !request() ) {
            ++m_clusterRetriesCount;
            break;
        }
        else {
            return;
        }
    }
}

}  // namespace Zigbee
}  // namespace HubPlatform
