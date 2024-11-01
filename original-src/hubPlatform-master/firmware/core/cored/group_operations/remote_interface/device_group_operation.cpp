#include <unistd.h>
#include "device_group_operation.h"
#include "service_interfaces/service.h"
#include "cored/group_operations/resources.h"
#include "jsonrpc/jsonrpc.h"
#include "cored/model/resources/cored_resources.h"
#include "crypto_utils/digests.h"

namespace HubPlatform {
namespace CoreD {

const auto ID = "_id";

DeviceGroupOperation::DeviceGroupOperation( Services::Service& service )
: m_hostService( service ) {}

JsonObject DeviceGroupOperation::create( const JsonObject& params ) {
    auto response = m_hostService.sendRequest( GroupsNames::CORED, params, MessageSubClasses::DEVICE_GROUP_CREATE );
    return response;
};

JsonObject DeviceGroupOperation::deleteGroup( const JsonObject& params ) {
    auto response = m_hostService.sendRequest( GroupsNames::CORED, params, MessageSubClasses::DEVICE_GROUP_DELETE );
    return response;
};

JsonObject DeviceGroupOperation::update( const JsonObject& params ) {
    auto response = m_hostService.sendRequest( GroupsNames::CORED, params, MessageSubClasses::DEVICE_GROUP_UPDATE );
    return response;
}

JsonObject DeviceGroupOperation::list( const JsonObject& params ) {
    using namespace Resources;
    Json answer;
    std::set< std::string > ids;
    JsonArray strings;
    JsonObject stringsObj;
    if( json_get( params, DeviceGroup::Ids, strings ) ) {
        if( !strings.is_array() ) {
            setErrorWithField( answer, Errors::WRONG_PARAMS, "Wrong value type", DeviceGroup::Ids );
            return answer;
        }
        try {
            fromJsonStringArray( strings, [&ids]( const std::string& value ) { ids.emplace( value ); } );
        }
        catch( std::exception& ex ) {
            std::string reason = "Wrong value type: ";
            reason += ex.what();
            setErrorWithField( answer, Errors::WRONG_PARAMS, reason, DeviceGroup::Ids );
            return answer;
        }
    }
    std::string versionParams;
    const Json& version = params[ "version" ];
    if( !version.empty() ) {
        if( !version.is_string() ) {
            setErrorWithField( answer, Errors::WRONG_PARAMS, "Wrong value type", "version" );
            return answer;
        }
        versionParams = json_get_str( params, "version" );
    }

    using namespace Model::Resources;
    auto groups = m_hostService.getDBClient().findAll( DBNames::DeviceGroupsDB, JSON_EMPTY_OBJ, nullptr );
    JsonArrayBuilder filteredGroups;
    std::stringstream ss;

    auto processParams = [&filteredGroups, &ss]( const JsonObject& group ) {
        filteredGroups << group;
        ss << group;
    };

    for( const auto& group : groups ) {
        auto id = json_get_str( group, ID );
        if( !ids.empty() && ids.count( id ) ) {
            processParams( group );
        }
        else if( ids.empty() ) {
            processParams( group );
        }
    }

    ss << getpid();

    auto hash = CryptoUtils::Digests::sha1( ss );

    if( !version.empty() && hash == versionParams ) {
        return JSON( "version" << hash );
    }

    return JSON( "version" << hash << "deviceGroups" << filteredGroups );
}

JsonObject DeviceGroupOperation::get( const JsonObject& params ) {
    Json answer;

    JsonObject idObject;
    if( json_get( params, Resources::DeviceGroup::Id, idObject ) ) {
        if( !idObject.is_string() ) {
            setErrorWithField( answer, Errors::WRONG_PARAMS, "Wrong value type", Resources::DeviceGroup::Id );
            return answer;
        }

        if( idObject.empty() ) {
            setError( answer, Errors::ID_IS_EMPTY );
            return answer;
        }
        auto id = idObject.get< std::string >();
        using namespace Model::Resources;
        answer = m_hostService.getDBClient().findOne( DBNames::DeviceGroupsDB, JSON( ID << id ), nullptr );

        if( answer.empty() ) {
            setErrorWithField( answer, Errors::DOES_NOT_EXIST, "", ID );
            return answer;
        }
    }
    else {
        setErrorWithField( answer, Errors::WRONG_PARAMS, "Not found", Resources::DeviceGroup::Id );
        return answer;
    }

    return answer;
}

JsonObject DeviceGroupOperation::find( const JsonObject& params ) {
    auto response = m_hostService.sendRequest( GroupsNames::CORED, params, MessageSubClasses::DEVICE_GROUP_FIND );
    return response;
}

JsonObject DeviceGroupOperation::expand( const JsonObject& params ) {
    auto response =
        m_hostService.sendRequest( GroupsNames::CORED, params, MessageSubClasses::DEVICE_ITEM_GROUP_EXPAND );
    return response;
}

JsonObject DeviceGroupOperation::addDevices( const JsonObject& params ) {
    auto response =
        m_hostService.sendRequest( GroupsNames::CORED, params, MessageSubClasses::DEVICE_GROUPS_DEVICE_ADD );
    return response;
}

JsonObject DeviceGroupOperation::removeDevices( const JsonObject& params ) {
    auto response =
        m_hostService.sendRequest( GroupsNames::CORED, params, MessageSubClasses::DEVICE_GROUPS_DEVICE_REMOVE );
    return response;
}

}  // namespace CoreD
}  // namespace HubPlatform
