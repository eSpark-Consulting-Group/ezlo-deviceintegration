#pragma once

#include "luup/user_data.h"
#include "luup/service_response.h"

#include "jsonrpc/message.h"
#include "config_reader/config_reader.h"

#include <tiny/tiny_http.h>

#include <string>
#include <unordered_map>
#include <memory>

namespace HubPlatform {
namespace Addons {

class Addon;
}
}  // namespace HubPlatform

class BaseService {
public:
    using Query = tiny::http::uri::qsmap;
    using RequestHandler = ServiceResponse ( BaseService::* )( const Query& query );
    using RequestHandlersMap = std::unordered_map< std::string, RequestHandler >;

public:
    BaseService( HubPlatform::Addons::Addon& addon, const std::string& port, const std::string& webRoot );

    virtual ~BaseService() = default;

    virtual void start() = 0;

protected:
    const std::string& getPort() const;

    const std::string& getWebRoot() const;

    ServiceResponse handleDataRequest( const Query& query );

private:
    ServiceResponse staticDataRequestHandler( const Query& query );
    ServiceResponse userDataRequestHandler( const Query& query );
    ServiceResponse actionsRequestHandler( const Query& query );
    ServiceResponse invokeRequestHandler( const Query& query );
    ServiceResponse sdataRequestHandler( const Query& query );
    ServiceResponse sudRequestHandler( const Query& query );
    ServiceResponse statusRequestHandler( const Query& query );
    ServiceResponse jobStatusRequestHandler( const Query& query );
    ServiceResponse cameraPortRequestHandler( const Query& query );
    ServiceResponse deviceRequestHandler( const Query& query );
    ServiceResponse roomRequestHandler( const Query& query );
    ServiceResponse sceneRequestHandler( const Query& query );
    ServiceResponse irRequestHandler( const Query& query );
    ServiceResponse deviceIpRequestHandler( const Query& query );
    ServiceResponse fileRequestHandler( const Query& query );
    ServiceResponse luaRequestHandler( const Query& query );
    ServiceResponse actionRequestHandler( const Query& query );
    ServiceResponse variableSetRequestHandler( const Query& query );
    ServiceResponse variableGetRequestHandler( const Query& query );
    ServiceResponse variableClearRequestHandler( const Query& query );
    ServiceResponse objectGetRequestHandler( const Query& query );
    ServiceResponse findDeviceRequestHandler( const Query& query );
    ServiceResponse childDeviceRequestHandler( const Query& query );
    ServiceResponse resyncRequestHandler( const Query& query );
    ServiceResponse updatePluginRequestHandler( const Query& query );
    ServiceResponse wgetRequestHandler( const Query& query );
    ServiceResponse findUpnpRequestHandler( const Query& query );
    ServiceResponse requestImageRequestHandler( const Query& query );
    ServiceResponse refreshCacheRequestHandler( const Query& query );
    ServiceResponse archiveVideoRequestHandler( const Query& query );
    ServiceResponse videoStreamRequestHandler( const Query& query );
    ServiceResponse serveStreamRequestHandler( const Query& query );
    ServiceResponse streamRequestHandler( const Query& query );
    ServiceResponse audioStreamRequestHandler( const Query& query );
    ServiceResponse bleScanResultRequestHandler( const Query& query );
    ServiceResponse addAllertRequestHandler( const Query& query );
    ServiceResponse delAlertRequestHandler( const Query& query );
    ServiceResponse relayRequestHandler( const Query& query );
    ServiceResponse holePunchingRequestHandler( const Query& query );
    ServiceResponse camLightsRequestHandler( const Query& query );
    ServiceResponse ipRequestsRequestHandler( const Query& query );
    ServiceResponse masterRegisteringRequestHandler( const Query& query );
    ServiceResponse slaveDataChangedRequestHandler( const Query& query );
    ServiceResponse blackListIpRequestHandler( const Query& query );
    ServiceResponse provisionRequestHandler( const Query& query );
    ServiceResponse liveEnergyUsageRequestHandler( const Query& query );
    ServiceResponse temperatureRequestHandler( const Query& query );
    ServiceResponse crash1RequestHandler( const Query& query );
    ServiceResponse crash2RequestHandler( const Query& query );
    ServiceResponse readAldbRequestHandler( const Query& query );
    ServiceResponse reloadRequestHandler( const Query& query );
    ServiceResponse purgeUpnpDevicesRequestHandler( const Query& query );
    ServiceResponse aliveRequestHandler( const Query& query );
    ServiceResponse localtimeRequestHandler( const Query& query );
    ServiceResponse dummyRequestHandler( const Query& query );
    ServiceResponse backupCreateRequestHandler( const Query& query );
    ServiceResponse backupRestoreRequestHandler( const Query& query );
    ServiceResponse backupGetListRequestHandler( const Query& query );
    ServiceResponse setHouseModeRequestHandler( const Query& query );

    bool getQueryValue( const Query& query, const std::string& key, std::string& value );
    std::string getRoomId( int roomIdx );

private:
    static ResponseFormat getResponseFormat( const Query& query );
    static RequestHandler getRequestHandler( const std::string& id );

private:
    std::string m_port;
    std::string m_webRoot;

    HubPlatform::Addons::Addon& m_addon;

    HubPlatform::Luup::UserData m_userData;
    HubPlatform::ConfigReader m_config;

protected:
    static const std::string DataRequestDir;
    static const std::string LuaDir;
};

class Message : public rpc::Message {
public:
    virtual rpc::Message::GenericSenderPtr getNetworkObject() const override {
        rpc::Message::GenericSenderPtr ptr;
        return ptr;
    }
};
