#include "item_group_operation.h"
#include "json_utils/json_fwd.h"
#include "service_interfaces/service.h"

namespace HubPlatform {
namespace CoreD {

ItemGroupOperation::ItemGroupOperation( Services::Service& service )
: m_hostService( service ) {}

JsonObject ItemGroupOperation::create( const JsonObject& params ) {
    auto response = m_hostService.sendRequest( GroupsNames::CORED, params, MessageSubClasses::ITEM_GROUP_CREATE );
    return response;
}

JsonObject ItemGroupOperation::deleteGroup( const JsonObject& params ) {
    auto response = m_hostService.sendRequest( GroupsNames::CORED, params, MessageSubClasses::ITEM_GROUP_DELETE );
    return response;
}

JsonObject ItemGroupOperation::update( const JsonObject& params ) {
    auto response = m_hostService.sendRequest( GroupsNames::CORED, params, MessageSubClasses::ITEM_GROUP_UPDATE );
    return response;
}

JsonObject ItemGroupOperation::list( const JsonObject& params ) {
    auto response = m_hostService.sendRequest( GroupsNames::CORED, params, MessageSubClasses::ITEM_GROUPS_LIST );
    return response;
}

JsonObject ItemGroupOperation::get( const JsonObject& params ) {
    auto response = m_hostService.sendRequest( GroupsNames::CORED, params, MessageSubClasses::ITEM_GROUP_GET );
    return response;
}

}  // namespace CoreD
}  // namespace HubPlatform
