#include "device_group_validator.h"
#include "scenes_manager/modify_scene/scene_scale_modifier.h"
#include "resources.h"
#include "group_operations_utils.h"
#include "database_client/dbqueryparser.h"
#include "jsonrpc/jsonrpc.h"
#include "common_utils/hub_info_helper.h"
#include "cored/model/values/value_factory.h"

#include "database_client/idbclient.h"

namespace HubPlatform {
namespace CoreD {
using namespace Resources;

JsonObject DeviceGroupValidator::checkDevices( const JsonArray& devices, JsonPath& path ) {
    JsonObject answer;
    for( const auto& device : devices ) {
        auto id = device.get< std::string >();
        auto existingDevice = m_dbClient.findOne( HaDevicesDBName, JSON( "_id" << id ), nullptr );
        if( existingDevice.empty() ) {
            std::string reason = "Device id: " + id;
            setErrorWithField( answer, Errors::NO_DEVICE_FOUND, reason, path.toString() );
            return answer;
        }
    }
    return JSON_EMPTY_OBJ;
}

JsonObject DeviceGroupValidator::validateCreateGroupRequest( const JsonObject& params, bool checkRequiredParams ) {
    JsonObject answer = JSON_EMPTY_OBJ;
    JsonPath path;
    auto gv = path.makeGuard( "params" );
    auto checkCategory = [&]( const JsonObject& category, JsonPath& path ) {
        auto gv = path.makeGuard( DeviceGroup::Category );
        try {
            jsonCheckString( category, DeviceGroup::Category, false );
            jsonCheckStringArray( category, DeviceGroup::Subcategories, false );
            jsonCheckStringArray( category, DeviceGroup::Exceptions, false );
        }
        catch( const WrongParamsError& ex ) {
            setErrorWithField( answer, Errors::WRONG_PARAMS, "", ex.what() );
            return false;
        }
        catch( const WrongFieldError& ex ) {
            setErrorWithField( answer, Errors::WRONG_FIELD, "", ex.what() );
            return false;
        }

        return true;
    };
    try {
        jsonCheckString( params, DeviceGroup::Name, checkRequiredParams );
        jsonCheckString( params, DeviceGroup::EntryDelay, false );
        jsonCheckString( params, DeviceGroup::Role, false );
        jsonCheckStringArray( params, DeviceGroup::Devices, false );
        jsonCheckStringArray( params, DeviceGroup::Exceptions, false );
    }
    catch( const WrongParamsError& ex ) {
        setErrorWithField( answer, Errors::WRONG_PARAMS, "", ex.what() );
        return answer;
    }
    catch( const WrongFieldError& ex ) {
        setErrorWithField( answer, Errors::WRONG_FIELD, "", ex.what() );
        return answer;
    }
    try {
        if( json_contains( params, DeviceGroup::Categories ) ) {
            jsonCheckArray( params, DeviceGroup::Categories );
        }
    }
    catch( const std::runtime_error& ex ) {
        setErrorWithField( answer, Errors::WRONG_FIELD, ex.what(), DeviceGroup::Categories );
        return answer;
    }

    auto packageId = json_get_str( params, DeviceGroup::PackageId );
    if( !packageId.empty() ) {
        setErrorWithField( answer, Errors::OBJECT_NOT_FOUND, "package id should be empty", DeviceGroup::PackageId );
        return answer;
    }

    std::string name;
    name = json_get_str( params, DeviceGroup::Name );
    try {
        jsonCheckString( name, JsonStringChecks::RESTRICTED_LENGTH );
    }
    catch( ... ) {
        setErrorWithField( answer, Errors::TOO_LONG, "", DeviceGroup::Name );
        return answer;
    }
    JsonArray categories;
    JsonArray devices;
    JsonArray exceptions;
    bool hasCategories = json_get_arr( params, DeviceGroup::Categories, categories ) && !categories.empty();
    bool hasDevices = json_get_arr( params, DeviceGroup::Devices, devices ) && !devices.empty();
    bool hasExceptions = json_get_arr( params, DeviceGroup::Exceptions, exceptions ) && !exceptions.empty();

    if( hasCategories ) {
        auto gv = path.makeGuard( DeviceGroup::Categories );
        for( const auto& category : categories ) {
            if( !checkCategory( category, path ) ) {
                return answer;
            }
        }
    }

    if( hasDevices ) {
        auto gv = path.makeGuard( DeviceGroup::Devices );
        answer = checkDevices( devices, path );
        if( !answer.empty() ) {
            return answer;
        }
    }
    if( hasExceptions ) {
        auto gv = path.makeGuard( DeviceGroup::Exceptions );
        answer = checkDevices( exceptions, path );
        if( !answer.empty() ) {
            return answer;
        }
    }

    return answer;
}

}  // namespace CoreD
}  // namespace HubPlatform
