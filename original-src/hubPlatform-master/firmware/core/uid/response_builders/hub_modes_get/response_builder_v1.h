#pragma once

#include "uid/response_builders/hub_modes_get/iresponse_builder.h"

#include "uid/response_builders/response_builder_base.h"

namespace HubPlatform {
namespace Uid {
namespace ResponseBuilder {
namespace HubModesGet {
namespace V1 {

class ResponseBuilder : public ResponseBuilderBase, public IResponseBuilder {
public:
    ResponseBuilder( const rpc::Message& message, const std::string& api );

    void setSettings( const std::string& currentHouseModeId,
                      const std::string& nextHouseMode,
                      int timeToLeftForSwitching,
                      int modesSwitchDelay,
                      int alarmOnDelay,
                      Json entryDelay,
                      Json abortWindow,
                      AllSecurityDevices securityDevices,
                      AllAlarmDevices alarmDevices,
                      AllCameraDevices cameraDevices,
                      AllProtectButtons protectButtons,
                      AllProtectDevices protectDevices ) override;

    void addHouseMode( const std::string& id,
                       const std::string& name,
                       const std::string& description,
                       bool ignoreSwitchDelay,
                       bool disarmedDefault,
                       bool protect,
                       bool armed,
                       const Notifications& notifications,
                       const DisarmedDevices& disarmedDevices,
                       const AlarmsOffDevices& alarmsOffDevices,
                       const CamerasOffDevices& camerasOffDevices,
                       const BypassDevices& bypassDevices ) override;

    JsonObject buildResponse( IDbClient& dbClient ) const override;

protected:
    std::string m_currentHouseModeId;
    std::string m_nextHouseMode;

    int m_timeToLeftForSwitching = -1;
    int m_modesSwitchDelay = -1;
    int m_alarmOnDelay = -1;
    Json m_entryDelay;
    Json m_abortWindow;
    AllSecurityDevices m_securityDevices;
    AllAlarmDevices m_alarmDevices;
    AllCameraDevices m_cameraDevices;
    AllProtectButtons m_protectButtons;
    AllProtectDevices m_protectDevices;

    std::vector< JsonObject > m_houseModes;
};

}  // namespace V1
}  // namespace HubModesGet
}  // namespace ResponseBuilder
}  // namespace Uid
}  // namespace HubPlatform
