#pragma once
#include "uid/extensions/http_links/http_links_manager.h"
#include "json_utils/json_fwd.h"
#include "uid/extensions/ui_extension.h"
#include "uid/extensions/ui_bus_extension.h"
#include "uid/extensions/ui_http_extension.h"
#include "bus/messagesclasses.h"
#include "tiny/tiny_consolecolors.h"
#include "log_utils/ezlo_log.h"
#include <string>
#include <utility>

namespace HubPlatform {
namespace HttpLinks {

class HttpLinksBusExtension : public HubPlatform::Uid::Extensions::BusExtension {
public:
    explicit HttpLinksBusExtension( HubPlatform::HttpLinks::HttpLinksManager& manager );

private:
    void handleMethodCall( const JsonObject& json, MessageSubClasses requestType, const std::string& sender ) override;
    JsonObject handleRequestFromBus( const JsonObject& request, MessageSubClasses requestType ) override;
    void handleEventFromBus( const JsonObject& event, MessageSubClasses requestType ) override;

private:
    JsonObject handleCreateLinkUrl( const JsonObject& json );
    JsonObject handleDeleteLinkUrl( const JsonObject& json );
    JsonObject handleGetLinkInfo( const JsonObject& json );

    JsonObject handleCloseConnection( const JsonObject& json );
    JsonObject handleSendData( const JsonObject& json );
    JsonObject handleGetConnectionInfo( const JsonObject& json );

private:
    HubPlatform::HttpLinks::HttpLinksManager& m_manager;
};

}  // namespace HttpLinks
}  // namespace HubPlatform