
#include "device_model/h/permission.h"
#include "json_utils/json_utils.h"

std::string Permission::getSender( const JsonObject& params ) {
    return json_get_str( params, "sender" );
}

std::string Permission::get( const JsonObject& params, const Resource::Resources resource ) {
    // if (resource == Resource::RULES) return get(SUPER);  // RULES PERMISSION OFF
    std::string result;
    JsonObject permission;
    json_get_obj( params, "permission", permission );
    json_get( permission, Resource::get( resource ).c_str(), result );
    if( getCode( result ) == UNKNOWN )
        result = get( CLOSE );
    return result;
}

int Permission::getCode( const std::string& permission ) {
    return strToEnum( getPermissionsMap(), permission, Permission::UNKNOWN );
}

int Permission::getCode( const JsonObject& params, const Resource::Resources resource ) {
    return getCode( get( params, resource ) );
}

std::string Permission::get( const Permission::Permissions& permissionCode ) {
    return enumToStr( getPermissionsMap(), permissionCode, "-" );
}

std::string Permission::getDefaultOpen2All() {
    return get( CLOSE );
}

std::string Permission::getDefaultClose2All() {
    return get( CLOSE );
}

void Permission::update( JsonObject& userPermission, const JsonObject& permission ) {
    try {
        for( auto field = permission.begin(); field != permission.end(); ++field )
            if( userPermission.count( field.key() ) < 1 )
                userPermission[ field.key() ] = field.value();
    }
    catch( const std::exception& ex ) {
    }
}

void Permission::updatePermission( JsonObject& set_permission, std::string set_profile ) {
    if( set_profile == "admin" ) {
        Permission::update( set_permission, Permission::getAdminDefault() );
    }
    if( set_profile == "expert" ) {
        Permission::update( set_permission, Permission::getExpertDefault() );
    }
    if( set_profile == "user" ) {
        Permission::update( set_permission, Permission::getUserDefault() );
    }
}

void Permission::setNoMore( JsonObject& userPermission, const JsonObject& permission ) {
    try {
        for( auto field = permission.cbegin(); field != permission.cend(); ++field )
            if( userPermission.count( field.key() ) < 1 ||
                ( getCode( userPermission[ field.key() ] ) > getCode( field.value() ) ) )
                userPermission[ field.key() ] = field.value();
    }
    catch( const std::exception& ex ) {
    }
}

/*const EnumToFunkMap &Permission::getPermissionsMap()
{
    static const EnumToFunkMap permissions = {
        {"user",    Permission::getUserDefault()},
        {"expert",   Permission::getExpertDefault()},
        {"admin",     Permission::getAdminDefault()},
    };
    return permissions;
}*/

const JsonObject& Permission::getExpertOwnerDefault() {
    static const JsonObject permission = {
        { Resource::get( Resource::EZLO ), get( EXPERT ) },    { Resource::get( Resource::USERS ), get( EXPERT ) },
        { Resource::get( Resource::UI ), get( EXPERT ) },      { Resource::get( Resource::SCENES ), get( EXPERT ) },
        { Resource::get( Resource::DEVICES ), get( EXPERT ) },
    };
    return permission;
}

const JsonObject& Permission::getExpertDefault() {
    static const JsonObject permission = {
        { Resource::get( Resource::EZLO ), get( EXPERT ) },    { Resource::get( Resource::USERS ), get( EXPERT ) },
        { Resource::get( Resource::UI ), get( EXPERT ) },      { Resource::get( Resource::SCENES ), get( EXPERT ) },
        { Resource::get( Resource::DEVICES ), get( EXPERT ) },
    };
    return permission;
}

const JsonObject& Permission::getOwnerDefault() {
    static const JsonObject permission = {
        { Resource::get( Resource::EZLO ), get( SUPER ) },    { Resource::get( Resource::USERS ), get( SUPER ) },
        { Resource::get( Resource::UI ), get( SUPER ) },      { Resource::get( Resource::SCENES ), get( SUPER ) },
        { Resource::get( Resource::DEVICES ), get( SUPER ) },
    };
    return permission;
}

const JsonObject& Permission::getAdminDefault() {
    static const JsonObject permission = {
        { Resource::get( Resource::EZLO ), get( EDIT ) },    { Resource::get( Resource::USERS ), get( EDIT ) },
        { Resource::get( Resource::UI ), get( EDIT ) },      { Resource::get( Resource::SCENES ), get( EDIT ) },
        { Resource::get( Resource::DEVICES ), get( EDIT ) },
    };
    return permission;
}

const JsonObject& Permission::getUserDefault() {
    static const JsonObject permission = {
        { Resource::get( Resource::EZLO ), get( CLOSE ) },    { Resource::get( Resource::USERS ), get( CLOSE ) },
        { Resource::get( Resource::UI ), get( CLOSE ) },      { Resource::get( Resource::SCENES ), get( CLOSE ) },
        { Resource::get( Resource::DEVICES ), get( CLOSE ) },
    };
    return permission;
}

const JsonObject& Permission::getGuestDefault() {
    static const JsonObject permission = {
        { Resource::get( Resource::EZLO ), get( CLOSE ) },    { Resource::get( Resource::USERS ), get( CLOSE ) },
        { Resource::get( Resource::UI ), get( CLOSE ) },      { Resource::get( Resource::SCENES ), get( CLOSE ) },
        { Resource::get( Resource::DEVICES ), get( CLOSE ) },
    };
    return permission;
}

bool Permission::check( const JsonObject& params, const Resource::Resources resource, const int operation ) {
    return ( getCode( params, resource ) >= operation );
}

bool Permission::check( const JsonObject& params,
                        const Resource::Resources resource,
                        const int operation,
                        JsonObject& answer ) {
    if( check( params, resource, operation ) )
        return true;
    setError( answer, Errors::USER_DOESNT_HAVE_PERMISSION );
    return false;
}

bool Permission::check( rpc::Message& message, const Resource::Resources resource, const int operation ) {
    return check( message.getParams(), resource, operation, message.getAnswer() );
}

const EnumToStrMap& Permission::getPermissionsMap() {
    static const EnumToStrMap permissions = {
        { Permission::CLOSE, "-" },   { Permission::CREATE, "c" }, { Permission::VIEW, "r" },
        { Permission::EXECUTE, "x" }, { Permission::EDIT, "w" },   { Permission::DELETE, "d" },
        { Permission::SUPER, "s" },   { Permission::EXPERT, "e" },
    };
    return permissions;
}

std::string Resource::get( const Resource::Resources& resourceCode ) {
    return enumToStr( getResourcesMap(), resourceCode, "UNKNOWN" );
}

int Resource::getCode( const std::string& resource ) {
    return strToEnum( getResourcesMap(), resource, Resource::UNKNOWN );
}

const EnumToStrMap& Resource::getResourcesMap() {
    static const EnumToStrMap resources = { { Resource::EZLO, "ezlo" },
                                            { Resource::USERS, "users" },
                                            { Resource::UI, "ui" },
                                            { Resource::SCENES, "rules" },
                                            { Resource::DEVICES, "devices" } };
    return resources;
}

const JsonArray& Permission::getSharedUserMinimalRights() {
    static_assert( VIEW < EXECUTE && EXECUTE < EDIT, "inconsisted shared user permissions" );
    static const JsonArray perms{ "r", "x" };
    return perms;
}
