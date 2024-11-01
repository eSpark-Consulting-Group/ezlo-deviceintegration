#include "zwave/script_events/rediscover_device_progress.h"
#include "zwave/common/zwave_errors_utils.h"
#include "zwave/resources/zwave_resources.h"

namespace HubPlatform {
namespace Zwave {

RediscoverDeviceProgress::RediscoverDeviceProgress( uint8_t nodeId,
                                                    std::string status,
                                                    znet::utils::optional< uint8_t > progress,
                                                    znet::ErrorOpt errorDetails )
: NodeEvent( Resources::ScriptEvent::Name::RediscoverDeviceProgress, nodeId )
, m_status( std::move( status ) )
, m_progress( progress )
, m_errorDetails( std::move( errorDetails ) ) {}

Json RediscoverDeviceProgress::toJson() const {
    auto result = NodeEvent::toJson();

    result[ Resources::ScriptEvent::Param::Status ] = m_status;

    if( m_progress ) {
        result[ Resources::ScriptEvent::Param::Progress ] = *m_progress;
    }

    addErrorDetails( result, m_errorDetails );

    return result;
}

}  // namespace Zwave
}  // namespace HubPlatform
