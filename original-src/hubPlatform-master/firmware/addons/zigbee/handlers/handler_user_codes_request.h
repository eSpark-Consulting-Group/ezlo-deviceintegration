#pragma once

#include "zigbee/handlers/handler_monitoring.h"

namespace HubPlatform {
namespace Zigbee {

class HandlerUserCodesRequest final : public HandlerMonitoring {
public:
    HandlerUserCodesRequest( ZigbeeAddonCallbackInterface& callbackInterface,
                             CommandsSenderInterface& commandsSender,
                             uint16_t nodeId,
                             uint8_t endpointId );

    ~HandlerUserCodesRequest();

    void start( uint16_t startUserId, uint16_t endUserId );

private:
    void sendRequest();

    void sendProgress();

    // Callbacks
private:
    void doorLockGetPinResponse( EmberNodeId emberNodeId,
                                 uint8_t endpointId,
                                 uint16_t userId,
                                 uint8_t userStatus,
                                 uint8_t userType,
                                 const std::string& pin ) override;

private:
    uint16_t m_nodeId;
    uint8_t m_endpointId;
    uint16_t m_startUserId;
    uint16_t m_endUserId;

    uint16_t m_currentUserId;
    uint8_t m_retriesCount;

    NativeTimer m_stepTimer;
};

}  // namespace Zigbee
}  // namespace HubPlatform
