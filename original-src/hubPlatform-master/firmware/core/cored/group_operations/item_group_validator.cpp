#include "item_group_validator.h"

#include "resources.h"
#include "group_operations_utils.h"
#include "database_client/dbqueryparser.h"
#include "jsonrpc/jsonrpc.h"
#include "common_utils/hub_info_helper.h"
#include "cored/model/values/value_factory.h"
#include "cored/model/values/values.h"
// DONE: EFWR-4000: Remove comparison_info dependency, move comparison_info back to scenes
#include "cored/model/values/value_families.h"

#include "database_client/idbclient.h"

namespace HubPlatform {
namespace CoreD {
namespace ItemGroupValidator {

bool checkValueTypeFamily( const std::string& checkedFamily ) {
    for( const auto& family : CoreD::Model::valueTypeFamilies() ) {
        if( family.first == checkedFamily )
            return true;
    }
    return false;
}

JsonObject checkValueTypeFamilyWithValueType( const std::string& checkedFamily, const std::string& checkedValueType ) {
    for( const auto& family : CoreD::Model::valueTypeFamilies() ) {
        if( family.first == checkedFamily ) {
            for( const auto& type : family.second ) {
                if( type == checkedValueType ) {
                    return true;
                }
            }
            return false;
        }
    }
    return false;
}

JsonObject validateCreateItemGroupRequest( const JsonObject& params, bool checkRequiredParams ) {
    JsonObject answer = JSON_EMPTY_OBJ;
    JsonPath path;
    auto gv = path.makeGuard( "params" );

    using namespace HubPlatform::CoreD::Resources;
    using namespace HubPlatform::CoreD;

    try {
        jsonCheckString( params, ItemGroup::Name, checkRequiredParams && true );
        jsonCheckStringArray( params, ItemGroup::ItemNames, checkRequiredParams && true );
        jsonCheckString( params, ItemGroup::ValueType, false );
        jsonCheckStringArray( params, ItemGroup::Enum, false );
        jsonCheckString( params, ItemGroup::ValueTypeFamily, false );
        jsonCheckString( params, ItemGroup::Role, false );
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
        if( json_contains( params, ItemGroup::Persistent ) ) {
            jsonCheckBool( params, ItemGroup::Persistent, true );
        }
    }
    catch( const std::runtime_error& ex ) {
        setErrorWithField( answer, Errors::WRONG_PARAMS, ex.what(), ItemGroup::Persistent );
        return answer;
    }
    try {
        if( json_contains( params, ItemGroup::HasGetter ) ) {
            jsonCheckBool( params, ItemGroup::HasGetter, true );
        }
    }
    catch( const std::runtime_error& ex ) {
        setErrorWithField( answer, Errors::WRONG_PARAMS, ex.what(), ItemGroup::HasGetter );
        return answer;
    }
    try {
        if( json_contains( params, ItemGroup::HasSetter ) ) {
            jsonCheckBool( params, ItemGroup::HasSetter, true );
        }
    }
    catch( const std::runtime_error& ex ) {
        setErrorWithField( answer, Errors::WRONG_PARAMS, ex.what(), ItemGroup::HasSetter );
        return answer;
    }

    auto checkNameLength = [&answer]( const std::string& name ) {
        try {
            jsonCheckString( name );
        }
        catch( ... ) {
            setErrorWithField( answer, Errors::TOO_LONG, "", ItemGroup::Name );
        }
    };

    auto name = json_get_str( params, ItemGroup::Name );

    checkNameLength( name );
    if( !answer.empty() ) {
        return answer;
    }

    if( params.find( ItemGroup::ItemNames ) != params.cend() ) {
        auto& names = params[ ItemGroup::ItemNames ];
        for( auto nameObject : names ) {
            checkNameLength( json_get_str( nameObject ) );
            if( !answer.empty() ) {
                return answer;
            }
        }
    }

    if( params.find( ItemGroup::Enum ) != params.cend() ) {
        auto enumArray = params[ ItemGroup::Enum ];
        if( !enumArray.empty() ) {
            if( params.find( ItemGroup::ValueType ) == params.cend() ) {
                auto gv = path.makeGuard( ItemGroup::ValueType );
                setErrorWithField( answer, Errors::WRONG_PARAMS, "Item value type should be set", path.toString() );
                return answer;
            }
        }
    }

    if( params.find( ItemGroup::ValueType ) == params.cend() ) {
        try {
            jsonCheckString( params, ItemGroup::ValueTypeFamily, checkRequiredParams && true );
        }
        catch( const WrongParamsError& ex ) {
            setErrorWithField( answer, Errors::WRONG_PARAMS, "", ex.what() );
            return answer;
        }
        catch( const WrongFieldError& ex ) {
            setErrorWithField( answer, Errors::WRONG_FIELD, "", ex.what() );
            return answer;
        }
    }

    if( params.find( ItemGroup::Info ) != params.cend() ) {
        try {
            jsonCheckObject( params, ItemGroup::Info, checkRequiredParams && true );
            JsonObject info;
            json_get_obj( params, ItemGroup::Info, info );
            jsonCheckString( info, "description", checkRequiredParams && true );
        }
        catch( const WrongParamsError& ex ) {
            setErrorWithField( answer, Errors::WRONG_PARAMS, "", ex.what() );
            return answer;
        }
        catch( const WrongFieldError& ex ) {
            setErrorWithField( answer, Errors::WRONG_FIELD, "", ex.what() );
            return answer;
        }
    }

    using namespace HubPlatform::CoreD::Model;
    auto valueType = json_get_str( params, ItemGroup::ValueType );
    if( !valueType.empty() ) {
        if( ValueType::fromString( valueType ) == ValueType::Enum::NoValue ) {
            auto gv = path.makeGuard( ItemGroup::ValueType );
            setErrorWithField( answer, Errors::WRONG_PARAMS, "Wrong item value type", path.toString() );
            return answer;
        }
    }

    using namespace CoreD::Model;
    auto valueTypeFamily = json_get_str( params, ItemGroup::ValueTypeFamily );
    if( !valueTypeFamily.empty() ) {
        if( !checkValueTypeFamily( valueTypeFamily ) ||
            ( !valueType.empty() && !checkValueTypeFamilyWithValueType( valueTypeFamily, valueType ) ) ) {
            auto gv = path.makeGuard( ItemGroup::ValueTypeFamily );
            setErrorWithField( answer, Errors::WRONG_PARAMS, "Wrong item value type family", path.toString() );
            return answer;
        }
    }

    return answer;
}

}  // namespace ItemGroupValidator
}  // namespace CoreD
}  // namespace HubPlatform
