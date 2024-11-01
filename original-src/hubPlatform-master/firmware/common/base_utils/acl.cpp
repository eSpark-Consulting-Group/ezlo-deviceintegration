#include <set>
#include <algorithm>

#include "base_utils/utils.h"
#include "database_client/db_tables_names.h"
#include "base_utils/fo_utils.h"
#include "base_utils/ezlodb.h"

#include "acl.h"

void Acl::create( const std::string& id, const Resource::Resources resource, const JsonObject& params ) {
    const Json idObj = Db::getIdObj( id );
    Db::update(
        AclPermissionDBName,
        idObj,
        JSON( "$set" << JSON( idObj << "creator" << Permission::getSender( params ) << "resource"
                                    << Resource::get( resource ) << "open2all" << Permission::getDefaultOpen2All()
                                    << "close2all" << Permission::getDefaultClose2All() ) ),
        true );
}

void Acl::remove( const std::string& id, requestUnbindUserClbk_t requestUnbindUserClbk ) {
    JsonObject page = JSON_EMPTY_OBJ;
    JsonArray users = JSON_EMPTY_ARR;
    std::string userEmail;
    get( id, page );
    if( json_get_arr( page, "assigns", users ) ) {
        for( auto& user : users ) {
            userEmail = json_get_str( user, "user" );
            if( Permission::getCode( json_get_str( user, "permission" ) ) <= Permission::EXECUTE &&
                Db::count( AclPermissionDBName, JSON( "assigns.user" << userEmail ) ) == 1 )
                requestUnbindUserClbk( userEmail );
        }
    }
    Db::remove( AclPermissionDBName, Db::getIdObj( id ) );
}

void Acl::set( const std::string& id, const std::string& open2all, const std::string& close2all ) {
    JsonBuilder toSet;
    bool hasToSet = false;
    if( Permission::getCode( open2all ) != Permission::UNKNOWN ) {
        toSet << "open2all" << open2all;
        hasToSet = true;
    }
    if( Permission::getCode( close2all ) != Permission::UNKNOWN ) {
        toSet << "close2all" << close2all;
        hasToSet = true;
    }
    if( hasToSet )
        Db::update( AclPermissionDBName, Db::getIdObj( id ), JSON( "$set" << toSet.obj() ), true );
}

void Acl::get( const std::string& id, JsonObject& result ) {
    result = Db::findOne( AclPermissionDBName, Db::getIdObj( id ) );
}

void Acl::assign( const std::string& id, const std::string& user, const std::string& permission, JsonObject& answer ) {
    if( Permission::getCode( permission ) == Permission::UNKNOWN ) {
        setError( answer, Errors::UNKNOWN_PERMISSION );
        return;
    }
    FoUtils::assignUserWithPermission( AclPermissionDBName, id, user, permission );
}

bool Acl::checkAssigned( const JsonObject& params,
                         JsonObject& answer,
                         pageViewUserRemoveClbk_t pageViewUserRemoveClbkr ) {
    JsonArray users;
    std::string owner;
    if( !getField( params, "owner", answer, owner ) || !getField( params, "users", answer, users ) )
        return false;
    std::set< std::string > emails;
    Json fieldsToreturn = JSON( "creator" << 1 << "assigns.user" << 1 );
    for( auto& object : Db::findAll( AclPermissionDBName,
                                     JSON( "$or" << JSON_ARRAY( JSON( "creator" << JSON( "$ne" << owner ) ) << JSON(
                                                                    "assigns.user" << JSON( "$exists" << true ) ) ) ),
                                     &fieldsToreturn ) ) {
        if( object.count( "creator" ) )
            emails.insert( json_get_str( object, "creator" ) );
        if( object.count( "assigns" ) && object.at( "assigns" ).is_array() )
            for( auto& assignElement : object.at( "assigns" ) )
                emails.insert( json_get_str( assignElement, "user" ) );
    }
    for( JsonObject& user : users )
        if( user.is_string() )
            emails.erase( json_get_str( user ) );
    for( const std::string& email : emails )
        unassignAll( email, owner, pageViewUserRemoveClbkr );
    return true;
}

void Acl::unassign( const std::string& id, const std::string& user, requestUnbindUserClbk_t requestUnbindUserClbk ) {
    std::string lowerUser = toLower( user );
    Db::update( AclPermissionDBName,
                JSON( Db::getIdObj( id ) << "assigns.user" << lowerUser ),
                JSON( "$pull" << JSON( "assigns" << JSON( "user" << lowerUser ) ) ) );

    if( Db::count( AclPermissionDBName, JSON( "assigns.user" << lowerUser ) ) == 0 )
        requestUnbindUserClbk( lowerUser );
}

void Acl::unassignAll( const JsonObject& params, pageViewUserRemoveClbk_t pageViewUserRemoveClbk ) {
    std::string sender, email;
    if( json_get( params, "sender", sender ) && json_get( params, "email", email ) )
        unassignAll( email, sender, pageViewUserRemoveClbk );
}

void Acl::unassignAll( const std::string& email,
                       const std::string& sender,
                       pageViewUserRemoveClbk_t pageViewUserRemoveClbk ) {
    std::string lowerEmail = toLower( email );
    std::string lowerSender = toLower( sender );
    Db::update( AclPermissionDBName,
                JSON( "assigns.user" << lowerEmail ),
                JSON( "$pull" << JSON( "assigns" << JSON( "user" << lowerEmail ) ) ),
                false,
                true );
    if( lowerSender != lowerEmail ) {
        Db::update( AclPermissionDBName,
                    JSON( "creator" << lowerEmail ),
                    JSON( "$set" << JSON( "creator" << lowerSender ) ),
                    false,
                    true );
        pageViewUserRemoveClbk( lowerEmail );
    }
}

bool Acl::checkPermission( const std::string& id,
                           const JsonObject& params,
                           const Resource::Resources resource,
                           const int operation ) {
#if !defined( __HUB_ENABLE_ACL__ )
    return true;
#endif
    JsonObject answer;
    return ( id.empty() ? check( params, resource, operation, answer ) :
                          check( id, params, resource, operation, answer ) );
}

bool Acl::checkWithParent( const std::string& id,
                           const JsonObject& params,
                           Resource::Resources resource,
                           const int operation,
                           const std::string& dbName,
                           JsonObject& answer ) {
#if !defined( __HUB_ENABLE_ACL__ )
    return true;
#endif
    static const std::string parentIdField = "parent_id";
    JsonObject answerCopy = answer;
    if( check( id, params, resource, operation, answer ) )
        return true;
    const JsonObject fieldsToReturn = JSON( parentIdField << 1 );
    Json object = Db::findOne( dbName, Db::getIdObj( id ), &fieldsToReturn );
    if( object.empty() || !object.count( parentIdField ) )
        return false;
    std::string parentId = json_get_str( object, parentIdField );
    if( parentId.empty() )
        return false;
    answer = answerCopy;
    return check( parentId, params, resource, operation, answer );
}

bool Acl::check( const std::string& id,
                 const JsonObject& params,
                 Resource::Resources resource,
                 const int operation,
                 JsonObject& answer ) {
#if !defined( __HUB_ENABLE_ACL__ )
    return true;
#endif
    if( !checkLocked( operation, answer ) )
        return false;
    Json permission = Db::findOne( AclPermissionDBName, Db::getIdObj( id ) );
    if( !permission.empty() ) {
        std::string user = Permission::getSender( params );
        if( permission.count( "creator" ) && user == json_get_str( permission, "creator" ) )
            return true;
        std::string userPermission, open2all, close2all;
        if( permission.count( "assigns" ) && permission.at( "assigns" ).is_array() ) {
            for( Json& assign : permission.at( "assigns" ) ) {
                if( user == json_get_str( assign, "user" ) ) {
                    if( assign.count( "permission" ) )
                        userPermission = json_get_str( assign, "permission" );
                    if( Permission::getCode( userPermission ) >= operation )
                        return true;
                    setError( answer, Errors::USER_DOESNT_HAVE_PERMISSION );
                    return false;
                }
            }
        }
        if( permission.count( "open2all" ) ) {
            open2all = json_get_str( permission, "open2all" );
            if( Permission::getCode( open2all ) >= operation )
                return true;
        }
        if( permission.count( "close2all" ) ) {
            close2all = json_get_str( permission, "close2all" );
            int close2allCode = Permission::getCode( close2all );
            if( close2allCode > Permission::CLOSE && operation >= close2allCode ) {
                setError( answer, Errors::CLOSED_FOR_ALL );
                return false;
            }
        }
        if( resource == Resource::UNKNOWN && permission.count( "resource" ) )
            resource =
                static_cast< Resource::Resources >( Resource::getCode( json_get_str( permission, "resource" ) ) );
    }
    return check_( params, resource, operation, answer );
}

bool Acl::check( const std::string& id, const JsonObject& params, const int operation, JsonObject& answer ) {
#if !defined( __HUB_ENABLE_ACL__ )
    return true;
#endif
    return check( id, params, Resource::UNKNOWN, operation, answer );
}

bool Acl::check( const JsonObject& params,
                 const Resource::Resources resource,
                 const int operation,
                 JsonObject& answer ) {
#if !defined( __HUB_ENABLE_ACL__ )
    return true;
#endif
    if( !checkLocked( operation, answer ) )
        return false;
    return check_( params, resource, operation, answer );
}

bool Acl::check_( const JsonObject& params,
                  const Resource::Resources resource,
                  const int operation,
                  JsonObject& answer ) {
#if !defined( __HUB_ENABLE_ACL__ )
    return true;
#endif
    if( Permission::getCode( Permission::get( params, resource ) ) >= operation )
        return true;
    setError( answer, Errors::USER_DOESNT_HAVE_PERMISSION );
    return false;
}

bool Acl::checkLocked( const int operation, JsonObject& answer ) {
#if !defined( __HUB_ENABLE_ACL__ )
    return true;
#endif
    if( ( operation == Permission::CREATE || operation >= Permission::EDIT ) && Db::getFlag( "isDevicesLocked" ) ) {
        setError( answer, Errors::DEVICES_LOCKED );
        return false;
    }
    return true;
}
