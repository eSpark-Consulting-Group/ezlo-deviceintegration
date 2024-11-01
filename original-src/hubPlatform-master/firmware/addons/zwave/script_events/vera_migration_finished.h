#pragma once

#include "service_interfaces/script_event.h"

#include "base_utils/optional.h"

namespace HubPlatform {
namespace Zwave {

class VeraMigrationFinished : public Services::ScriptEvent {
public:
    VeraMigrationFinished( Json&& veraDevicesModel );

    Json toJson() const final;

private:
    Json simplyfiDeviceModel() const;
    static common::optional< uint32_t > findZwaveRootDevice( const JsonArray& devices );

    Json m_veraDevicesModel;
};

}  // namespace Zwave
}  // namespace HubPlatform
