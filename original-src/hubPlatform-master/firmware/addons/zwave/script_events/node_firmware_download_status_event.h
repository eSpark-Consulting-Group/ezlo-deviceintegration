#include "service_interfaces/script_event.h"
#include <znet/optional.hpp>

namespace HubPlatform {
namespace Zwave {

class NodeFirmwareDownloadStatusEvent : public Services::ScriptEvent {
public:
    NodeFirmwareDownloadStatusEvent( std::string image,
                                     std::string checksum,
                                     std::string status,
                                     znet::utils::optional< uint8_t > progress = {},
                                     std::string errorDescription = {} );

    Json toJson() const override;

private:
    std::string m_image;
    std::string m_checksum;
    std::string m_status;
    znet::utils::optional< uint8_t > m_progress;
    std::string m_errorDescription;
};

}  // namespace Zwave
}  // namespace HubPlatform
