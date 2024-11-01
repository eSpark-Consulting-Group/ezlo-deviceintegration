#include "zwave/script_events/node_firmware_download_status_event.h"

#include "zwave/resources/zwave_resources.h"

namespace HubPlatform {
namespace Zwave {

NodeFirmwareDownloadStatusEvent::NodeFirmwareDownloadStatusEvent( std::string image,
                                                                  std::string checksum,
                                                                  std::string status,
                                                                  znet::utils::optional< uint8_t > progress,
                                                                  std::string errorDescription )
: Services::ScriptEvent( Resources::ScriptEvent::Name::NodeFirmwareDownloadStatus )
, m_image( std::move( image ) )
, m_checksum( std::move( checksum ) )
, m_status( std::move( status ) )
, m_progress( progress )
, m_errorDescription( std::move( errorDescription ) ) {}

Json NodeFirmwareDownloadStatusEvent::toJson() const {
    auto result = ScriptEvent::toJson();

    result[ Resources::ScriptEvent::Param::Image ] = m_image;
    result[ Resources::ScriptEvent::Param::Status ] = m_status;

    if( !m_checksum.empty() ) {
        result[ Resources::ScriptEvent::Param::Checksum ] = m_checksum;
    }

    if( m_progress ) {
        result[ Resources::ScriptEvent::Param::Progress ] = *m_progress;
    }

    if( !m_errorDescription.empty() ) {
        result[ Resources::ScriptEvent::Param::Error ] = m_errorDescription;
    }

    return result;
}

}  // namespace Zwave
}  // namespace HubPlatform
