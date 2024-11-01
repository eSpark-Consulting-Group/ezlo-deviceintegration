#include "protect_validator.h"
#include "database_client/dbqueryparser.h"
#include "jsonrpc/jsonrpc.h"
#include "cored/model/values/value_factory.h"
#include "database_client/idbclient.h"
#include "resources.h"
#include "json_utils/json_utils.h"

namespace HubPlatform {
namespace CoreD {
namespace Protect {
using namespace Resources;

JsonObject ProtectValidator::validateAddContactIdAliasRequest( const JsonObject& params ) {
    JsonObject answer = JSON_EMPTY_OBJ;
    JsonPath path;
    auto gv = path.makeGuard( "params" );

    try {
        jsonCheckString( params, CMSSignalAlias::Code, true );
        jsonCheckString( params, CMSSignalAlias::Text, true );
        jsonCheckString( params, CMSSignalAlias::On, true );
        jsonCheckString( params, CMSSignalAlias::Off, false );
    }
    catch( const WrongParamsError& ex ) {
        setErrorWithField( answer, Errors::WRONG_PARAMS, "", ex.what() );
        return answer;
    }
    catch( const WrongFieldError& ex ) {
        setErrorWithField( answer, Errors::WRONG_FIELD, "", ex.what() );
        return answer;
    }

    auto text = json_get_str( params, CMSSignalAlias::Text );
    auto on = json_get_str( params, CMSSignalAlias::On );
    auto off = json_get_str( params, CMSSignalAlias::Off );
    try {
        jsonCheckString( text, JsonStringChecks::RESTRICTED_LENGTH );
    }
    catch( ... ) {
        setErrorWithField( answer, Errors::TOO_LONG, "", CMSSignalAlias::Text );
        return answer;
    }
    try {
        jsonCheckString( on, JsonStringChecks::RESTRICTED_LENGTH );
    }
    catch( ... ) {
        setErrorWithField( answer, Errors::TOO_LONG, "", CMSSignalAlias::On );
        return answer;
    }
    try {
        jsonCheckString( off, JsonStringChecks::RESTRICTED_LENGTH );
    }
    catch( ... ) {
        setErrorWithField( answer, Errors::TOO_LONG, "", CMSSignalAlias::Off );
        return answer;
    }

    return answer;
}

JsonObject ProtectValidator::validateAddSecurityConfigRequest( const JsonObject& params ) {
    JsonObject answer = JSON_EMPTY_OBJ;
    JsonPath path;
    auto gv = path.makeGuard( "params" );

    try {
        jsonCheckString( params, SecurityConfig::Name, true );
        jsonCheckString( params, SecurityConfig::Description, true );
        jsonCheckString( params, SecurityConfig::SoundType, true );
        jsonCheckString( params, SecurityConfig::EntryDelay, true );
        jsonCheckString( params, SecurityConfig::Chime, true );
        jsonCheckString( params, SecurityConfig::Follower, true );
        jsonCheckStringArray( params, SecurityConfig::ArmedWhen, true );
        jsonCheckObject( params, SecurityConfig::Signal, true );
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
        jsonCheckBool( params, SecurityConfig::Alarm, true );
    }
    catch( const std::runtime_error& ex ) {
        setErrorWithField( answer, Errors::WRONG_PARAMS, ex.what(), SecurityConfig::Alarm );
        return answer;
    }
    try {
        jsonCheckArray( params, SecurityConfig::RecommendCategories );
    }
    catch( const std::runtime_error& ex ) {
        setErrorWithField( answer, Errors::WRONG_PARAMS, ex.what(), SecurityConfig::RecommendCategories );
        return answer;
    }

    auto name = json_get_str( params, SecurityConfig::Name );
    auto description = json_get_str( params, SecurityConfig::Description );
    try {
        jsonCheckString( name, JsonStringChecks::RESTRICTED_LENGTH );
    }
    catch( ... ) {
        setErrorWithField( answer, Errors::TOO_LONG, "", SecurityConfig::Name );
        return answer;
    }
    try {
        jsonCheckString( description, JsonStringChecks::RESTRICTED_LENGTH );
    }
    catch( ... ) {
        setErrorWithField( answer, Errors::TOO_LONG, "", SecurityConfig::Description );
        return answer;
    }

    JsonArray recommendCategories;
    json_get_arr( params, SecurityConfig::RecommendCategories, recommendCategories );
    if( !recommendCategories.empty() ) {
        auto categoryGuard = path.makeGuard( SecurityConfig::RecommendCategories );
        for( auto& singleCategory : recommendCategories ) {
            try {
                jsonCheckString( singleCategory, SecurityConfig::Category, true );
                jsonCheckStringArray( singleCategory, SecurityConfig::SubCategory, true );
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
    }
    JsonObject signal;
    if( !json_get_obj( params, SecurityConfig::Signal, signal ) ) {
        auto signalGuard = path.makeGuard( SecurityConfig::Signal );
        try {
            jsonCheckObject( signal, SignalFields::Supervisory, true );
            jsonCheckObject( signal, SignalFields::TrippedArmed, true );
            jsonCheckObject( signal, SignalFields::TamperedArmed, true );
            jsonCheckObject( signal, SignalFields::TamperedDisarmed, true );
            jsonCheckObject( signal, SignalFields::Bypass, true );
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

    return answer;
}
}  // namespace Protect
}  // namespace CoreD
}  // namespace HubPlatform
