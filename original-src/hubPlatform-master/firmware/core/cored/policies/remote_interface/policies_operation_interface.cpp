#include "policies_operation_interface.h"
#include "json_utils/json_fwd.h"
#include "service_interfaces/service.h"

namespace HubPlatform {
namespace CoreD {

PolicyOperationInterface::PolicyOperationInterface( Services::Service& hostService )
: m_hostService( hostService ) {}

JsonObject PolicyOperationInterface::create( const JsonObject& params ) {
    auto response = m_hostService.sendRequest( GroupsNames::CORED, params, MessageSubClasses::POLICY_CREATE );
    return response;
}

JsonObject PolicyOperationInterface::remove( const JsonObject& params ) {
    auto response = m_hostService.sendRequest( GroupsNames::CORED, params, MessageSubClasses::POLICY_DELETE );
    return response;
}

JsonObject PolicyOperationInterface::update( const JsonObject& params ) {
    auto response = m_hostService.sendRequest( GroupsNames::CORED, params, MessageSubClasses::POLICY_UPDATE );
    return response;
}

JsonObject PolicyOperationInterface::get( const JsonObject& params ) {
    auto response = m_hostService.sendRequest( GroupsNames::CORED, params, MessageSubClasses::POLICY_GET );
    return response;
}

JsonObject PolicyOperationInterface::list( const JsonObject& params ) {
    auto response = m_hostService.sendRequest( GroupsNames::CORED, params, MessageSubClasses::POLICY_LIST );
    return response;
}

}  // namespace CoreD
}  // namespace HubPlatform
