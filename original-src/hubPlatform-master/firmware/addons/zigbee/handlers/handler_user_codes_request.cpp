#include "zigbee/handlers/handler_user_codes_request.h"

#include "zigbee/zigbee_task_loop.h"

#include "log_utils/ezlo_log.h"

namespace HubPlatform {
namespace Zigbee {
namespace {
static const std::chrono::seconds AnswerWaitTimeoutSeconds( 5 );
static const uint8_t RetriesCountMax = 3;
}  // namespace

HandlerUserCodesRequest::HandlerUserCodesRequest( ZigbeeAddonCallbackInterface& callbackInterface,
                                                  CommandsSenderInterface& commandsSender,
                                                  uint16_t nodeId,
                                                  uint8_t endpointId )
: HandlerMonitoring( callbackInterface, commandsSender )
, m_nodeId( nodeId )
, m_endpointId( endpointId )
, m_startUserId( 0 )
, m_endUserId( 0 )
, m_currentUserId( 0 )
, m_retriesCount( 0 )
, m_stepTimer( [this]() {
    ZigbeeTaskLoop::getInstance().addTask( [this]() {
        Log::print( Log::PriorityLevel::DEBUG,
                    "Timeout during user code request, nodeId 0x%04x, endpointId 0x%02x, usetCodeId: %d",
                    m_nodeId,
                    m_endpointId,
                    m_currentUserId );
        ++m_retriesCount;
        sendRequest();
    } );
} ) {}

HandlerUserCodesRequest::~HandlerUserCodesRequest() {
    m_stepTimer.stop();
}

void HandlerUserCodesRequest::start( uint16_t startUserId, uint16_t endUserId ) {
    m_startUserId = startUserId;
    m_endUserId = endUserId;
    m_currentUserId = 0;

    sendRequest();
}

void HandlerUserCodesRequest::sendRequest() {
    if( m_retriesCount == RetriesCountMax ) {
        m_retriesCount = 0;
        ++m_currentUserId;
    }

    if( m_currentUserId > m_endUserId ) {
        return;
    }

    getCommandsSender().sendCommand( m_nodeId, m_endpointId, ZCL_DOOR_LOCK_CLUSTER_ID, "GetPinCode", m_currentUserId );
    m_stepTimer.start( AnswerWaitTimeoutSeconds, true );
}

void HandlerUserCodesRequest::sendProgress() {
    uint8_t progress = ( 100 / ( m_endUserId - m_startUserId ) ) * ( m_currentUserId - m_startUserId );
    getCallbackInterface().userCodesRequestProgress( m_nodeId, m_endpointId, progress );
}

void HandlerUserCodesRequest::doorLockGetPinResponse( EmberNodeId emberNodeId,
                                                      uint8_t endpointId,
                                                      uint16_t userId,
                                                      uint8_t userStatus,
                                                      uint8_t userType,
                                                      const std::string& pin ) {
    HandlerMonitoring::doorLockGetPinResponse( emberNodeId, endpointId, userId, userStatus, userType, pin );

    if( userId == m_currentUserId ) {
        m_stepTimer.stop();
        sendProgress();
        m_retriesCount = 0;
        ++m_currentUserId;
        sendRequest();
    }
}

}  // namespace Zigbee
}  // namespace HubPlatform
