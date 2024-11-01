#pragma once

#include "uid/extensions/ui_bus_messenger.h"
#include "uid/extensions/ui_extension.h"
#include "uid/extensions/ui_http_session.h"

#include "http_bridge/ws_api_bridge.h"
#include "http_bridge/addons_endpoint.h"

namespace HubPlatform {
namespace Uid {

class Service;

namespace Extensions {

class ExtensionsManager : public BusMessenger, public HttpBridge::AddonsEndpoint {
public:
    explicit ExtensionsManager( Service& service, HttpBridge::ApiBridge& httpApiBridge );
    ~ExtensionsManager() override;

    void setUp();

    void handleMethodCall( const JsonObject& json, MessageSubClasses requestType, const std::string& sender );
    JsonObject handleRequestFromBus( const JsonObject& request, MessageSubClasses requestType );
    void handleEventFromBus( const JsonObject& event, MessageSubClasses requestType ) override;

private:
    void sendMethodCall( const std::string& serviceName,
                         MessageSubClasses requestType,
                         const JsonObject& data ) override;
    JsonObject sendRequest( const std::string& serviceName,
                            MessageSubClasses requestType,
                            const JsonObject& request ) override;

private:
    virtual int endpointCallback( struct lws* wsi,
                                  enum lws_callback_reasons reason,
                                  void* user,
                                  void* in,
                                  size_t len ) override;

private:
    void registerExtension( std::unique_ptr< Extension > extension );
    void unregisterExtension( const std::string& extensionName );

    static std::tuple< std::string, std::string > extractAddonNameAndEndpoint( const std::string& url );

private:
    Service& m_service;
    HttpBridge::ApiBridge& m_httpApiBridge;

    std::map< std::string, std::unique_ptr< Extension > > m_extensions;
};

}  // namespace Extensions
}  // namespace Uid
}  // namespace HubPlatform
