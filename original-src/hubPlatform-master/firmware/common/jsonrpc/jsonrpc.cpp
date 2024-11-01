
#include "jsonrpc/jsonrpc.h"

#include <tiny/tiny_utils.h>
#include <tiny/tiny_consolecolors.h>

#include <algorithm>
#include <cmath>
#include <ctype.h>

JsonObject prepareMethod( const std::string& method ) {
    // std::string strSrc = std::to_string( std::time( nullptr ) );
    std::string strSrc = cc::now2string( true,   // isUTC
                                         false,  // isDaysInsteadOfYMD
                                         false   // isColored
    );
    //
    tiny::tools::md5 md5algorithm;
    md5algorithm.update( (unsigned char*) strSrc.c_str(), strSrc.length() );
    md5algorithm.finalize();
    std::string strHash = md5algorithm.hex_digest();
    //
    JsonObject joMessage;
    json_add_str( joMessage, "id", strHash );
    json_add_str( joMessage, "method", method );
    joMessage[ "params" ] = JSON_EMPTY_OBJ;
    return joMessage;
}

bool hasError( const JsonObject& message ) {
    return message.count( Errors::ERROR_FIELD ) == 1 && message.at( Errors::ERROR_FIELD ).is_object();
}

bool checkField( const JsonObject& message, const std::string& field, JsonObject& answer ) {
    if( message.count( field ) > 0 )
        return true;
    JsonObject error_mes;
    Errors::getBadRequest( field, error_mes );
    setErrorObject( answer, error_mes );
    return false;
}

void copySender( const JsonObject& message, JsonObject& answer ) {
    JsonObject joSender = JSON_EMPTY_OBJ;
    std::string stringSenderId;
    if( json_get_obj( message, "sender", joSender ) ) {
        answer[ "sender" ] = joSender;
    }
    else if( json_get( message, "sender", stringSenderId ) ) {
        answer[ "sender" ] = stringSenderId;
    }
}

bool answerPrepare( const JsonObject& message, JsonObject& answer ) {
    if( !answer.is_object() )
        answer = JSON_EMPTY_OBJ;
    json_add_null( answer, Errors::ERROR_FIELD );
    const std::string method = json_get_str( message, "method" );
    if( !method.empty() )
        json_add_str( answer, "method", method );
    answer[ "result" ] = JSON_EMPTY_OBJ;
    std::string id;
    if( !getField( message, "id", answer, id ) ) {
        json_add_str( answer, "id", "nonexistent" );
        return false;
    }
    json_add_str( answer, "id", id );
    return true;
}

void setError( JsonObject& answer, Errors::Error error, const std::string& reason ) {
    JsonObject error_mes;
    Errors::get( error, error_mes );
    setErrorObject( answer, error_mes, reason );
}

void setErrorWithField( JsonObject& answer, Errors::Error error, const std::string& reason, const std::string& field ) {
    JsonObject error_mes;
    Errors::get( error, error_mes );
    setErrorObjectWithField( answer, error_mes, reason, field );
}

std::string getErrorMessage( const JsonObject& answer ) {
    if( answer.count( Errors::ERROR_FIELD ) < 1 )
        return std::string();
    const JsonObject& errorObject = answer.at( Errors::ERROR_FIELD );
    if( errorObject.is_object() )
        return json_get_str( errorObject, "message" );
    if( errorObject.is_string() )
        return errorObject.get< std::string >();
    return std::string();
}

void setErrorObject( JsonObject& answer, const JsonObject& error, const std::string& reason ) {
    answer[ std::string( Errors::ERROR_FIELD ) ] = error;
    if( !reason.empty() ) {
        answer[ std::string( Errors::ERROR_FIELD ) ][ "reason" ] = reason;
    }
}
void setErrorObjectWithField( JsonObject& answer,
                              const JsonObject& error,
                              const std::string& reason,
                              const std::string& field ) {
    answer[ std::string( Errors::ERROR_FIELD ) ] = error;

    if( !reason.empty() ) {
        answer[ std::string( Errors::ERROR_FIELD ) ][ "reason" ] = reason;
    }

    if( !field.empty() ) {
        answer[ std::string( Errors::ERROR_FIELD ) ][ "field" ] = field;
    }
}

bool setErrorIfHas( JsonObject& answer, const JsonObject& message ) {
    if( !hasError( message ) )
        return false;
    answer[ std::string( Errors::ERROR_FIELD ) ] = message.at( Errors::ERROR_FIELD );
    return true;
}

bool getField( const Json& message, const std::string& field, Json& answer, JsonObject& result ) {
    if( json_get_obj( message, field, result ) )
        return true;
    JsonObject error_mes;
    Errors::getBadRequest( field, error_mes );
    setErrorObject( answer, error_mes );
    return false;
}

bool getNonEmptyField( const Json& message,
                       const std::string& field,
                       Json& answer,
                       std::string& result,
                       bool bTrimValue /* = false*/ ) {
    std::string toRet;
    if( !json_get( message, field.c_str(), toRet ) ) {
        JsonObject error_mes;
        Errors::getBadRequest( field, error_mes );
        setErrorObject( answer, error_mes );
        return false;
    }
    if( bTrimValue )
        tiny::tools::trim( toRet );
    if( toRet.empty() ) {
        JsonObject error_mes;
        Errors::getEmptyString( field, error_mes );
        setErrorObject( answer, error_mes );
        return false;
    }
    result = std::move( toRet );
    return true;
}

bool getFieldAsArray( const Json& message, const std::string& field, Json& answer, JsonArray& result ) {
    if( json_get_arr( message, field, result ) )
        return true;
    JsonObject error_mes;
    Errors::getBadRequest( field, error_mes );
    setErrorObject( answer, error_mes );
    return false;
}

bool getOptionalField( const Json& message, const std::string& field, JsonObject& result ) {
    return json_get_obj( message, field, result );
}

bool isPortalType( const JsonObject& answer ) {
    JsonObject sender;
    if( !json_get( answer, "sender", sender ) )
        return false;
    return ( json_get_str( sender, "type" ) == "portal" );
}

bool convert_json_string_value_to_boolean( const std::string& r ) {
    std::string rl = tiny::tools::to_lower( r );
    if( rl.empty() )
        return false;
    char c = rl[ 0 ];
    if( c == 'f' || c == 'n' )
        return false;
    double lf = std::atof( r.c_str() );
    return ( lf != 0.0 ) ? true : false;
}
