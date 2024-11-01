#include "uid/icc_interface/icc_remote_interface.h"
#include "uid/icc_interface/icc_common.h"

#include "service_interfaces/service.h"
#include "json_utils/json_utils.h"

namespace HubPlatform {
namespace InterControllerCommunication {

namespace {
const auto DefaultICCRequestTimeout = std::chrono::seconds( 30 );

JsonObject generateIICRequest( const std::string& controllerId, const JsonObject& request ) {
    JsonObject internalRequest;

    internalRequest[ "controllerId" ] = controllerId;
    internalRequest[ "payload" ] = request;

    return internalRequest;
}

}  // namespace

RemoteInterface::RemoteInterface( Services::Service& hostService )
: m_hostService( hostService )
, m_iccRequestTimeout( DefaultICCRequestTimeout ) {}

JsonObject RemoteInterface::sendRequest( const std::string& controllerId, const JsonObject& request ) {
    JsonObject internalRequest = generateIICRequest( controllerId, request );

    return m_hostService.sendRequest( GroupsNames::UI_GROUP, internalRequest, ICC_REQUEST, m_iccRequestTimeout );
}

void RemoteInterface::sendAsyncRequest( const std::string& controllerId,
                                        const JsonObject& request,
                                        ReceiveCallback receiveCallback,
                                        TimeoutCallback timeoutCallback ) {
    JsonObject internalRequest = generateIICRequest( controllerId, request );

    m_hostService.sendAsyncRequest( GroupsNames::UI_GROUP,
                                    internalRequest,
                                    ICC_REQUEST,
                                    std::move( receiveCallback ),
                                    std::move( timeoutCallback ),
                                    m_iccRequestTimeout );
}

void RemoteInterface::sendAsyncRequest( const std::string& controllerId,
                                        const JsonObject& request,
                                        ReceiveCallback receiveCallback ) {
    JsonObject internalRequest = generateIICRequest( controllerId, request );

    m_hostService.sendAsyncRequest(
        GroupsNames::UI_GROUP, internalRequest, ICC_REQUEST, std::move( receiveCallback ), m_iccRequestTimeout );
}

std::string RemoteInterface::getControllerIdByUUID( const std::string& controllerUuid ) const {
    return Common::getControllerIdByUUID( m_hostService.getDBClient(), controllerUuid );
}

std::string RemoteInterface::getControllerUUIDById( const std::string& controllerId ) const {
    return Common::getControllerUUIDById( m_hostService.getDBClient(), controllerId );
}

void RemoteInterface::registerControllerConnection( const std::string& controllerId ) {
    Log::debug( cc::debug( "Registering controller " ) + cc::note( controllerId ) + cc::debug( " connection" ) );
    auto params = JSON_EMPTY_OBJ;
    params[ "controllerId" ] = controllerId;
    m_hostService.sendMethodCall( GroupsNames::UI_GROUP, ICC_REGISTER_CONTROLLER_CONNECTION, params );
}

void RemoteInterface::unregisterControllerConnection( const std::string& controllerId ) {
    Log::debug( cc::debug( "Unregistering controller " ) + cc::note( controllerId ) + cc::debug( " connection" ) );
    auto params = JSON_EMPTY_OBJ;
    params[ "controllerId" ] = controllerId;
    m_hostService.sendMethodCall( GroupsNames::UI_GROUP, ICC_UNREGISTER_CONTROLLER_CONNECTION, params );
}

}  // namespace InterControllerCommunication
}  // namespace HubPlatform
