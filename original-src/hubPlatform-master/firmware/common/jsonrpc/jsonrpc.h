#ifndef JSONRPC_H
#define JSONRPC_H

#include <string>

#include "json_utils/json_utils.h"
#include "error_codes.h"

JsonObject prepareMethod( const std::string& method );
bool hasError( const JsonObject& message );
bool checkField( const JsonObject& message, const std::string& field, JsonObject& answer );
void copySender( const JsonObject& message, JsonObject& answer );
bool answerPrepare( const JsonObject& message, JsonObject& answer );
void setError( JsonObject& answer, Errors::Error error, const std::string& reason = "" );
void setErrorWithField( JsonObject& answer, Errors::Error error, const std::string& reason, const std::string& field );
void setErrorObject( JsonObject& answer, const JsonObject& error, const std::string& reason = "" );
void setErrorObjectWithField( JsonObject& answer,
                              const JsonObject& error,
                              const std::string& reason,
                              const std::string& field );
bool setErrorIfHas( JsonObject& answer, const JsonObject& message );
std::string getErrorMessage( const JsonObject& answer );
bool getField( const Json& message, const std::string& field, Json& answer, JsonObject& result );

///@return true if @field is found and @result is not empty
bool getNonEmptyField( const Json& message,
                       const std::string& field,
                       Json& answer,
                       std::string& result,
                       bool bTrimValue = false );

bool getFieldAsArray( const Json& message, const std::string& field, Json& answer, JsonArray& result );
bool getOptionalField( const Json& message, const std::string& field, Json& answer, JsonObject& result );
bool isPortalType( const JsonObject& answer );
bool convert_json_string_value_to_boolean( const std::string& r );

template < typename T >
bool getField( const Json& message, const std::string& field, Json& answer, T& result ) {
    if( json_get( message, field.c_str(), result ) )
        return true;
    JsonObject error_mes;
    Errors::getBadRequest( field, error_mes );
    setErrorObject( answer, error_mes );
    return false;
}

template < bool >
bool getField( const Json& message, const std::string& field, Json& answer, bool& result ) {
    std::string r;
    if( json_get( message, field.c_str(), r ) ) {
        result = convert_json_string_value_to_boolean( r );
        return true;
    }
    JsonObject error_mes;
    Errors::getBadRequest( field, error_mes );
    setErrorObject( answer, error_mes );
    return false;
}

template < typename T >
inline bool getFieldVerbose( const Json& message, const std::string& field, Json& answer, T& result ) {
    const auto it = message.find( field );
    if( it == message.end() ) {
        JsonObject error;
        Errors::getBadRequest( field, error );
        setErrorObject( answer, error );
        return false;
    }

    try {
        result = it->get< T >();
    }
    catch( ... ) {
        JsonObject error;
        Errors::getWrongParams( field, error );
        setErrorObject( answer, error );
        return false;
    }

    return true;
}

template <>
inline bool getFieldVerbose< std::string >( const Json& message,
                                            const std::string& field,
                                            Json& answer,
                                            std::string& result ) {
    const auto it = message.find( field );
    if( it == message.end() ) {
        JsonObject error;
        Errors::getBadRequest( field, error );
        setErrorObject( answer, error );
        return false;
    }

    if( !it->is_string() ) {
        JsonObject error;
        Errors::getWrongParams( field, error );
        setErrorObject( answer, error );
        return false;
    }

    auto resultStr = it->get< std::string >();
    if( resultStr.empty() ) {
        JsonObject error;
        Errors::getEmptyString( field, error );
        setErrorObject( answer, error );
        return false;
    }

    result = std::move( resultStr );
    return true;
}

template < typename T >
bool getOptionalField( const Json& message, const std::string& field, T& result ) {
    return json_get( message, field.c_str(), result );
}

template < typename T >
const T getFieldSafe( const Json& jo, const std::string& strMemberPropertyName ) {
    try {
        if( strMemberPropertyName.empty() )
            return T();
        T retVal( jo.count( strMemberPropertyName ) ? jo[ strMemberPropertyName ].get< T >() : T() );
        return retVal;
    }
    catch( ... ) {
        return T();
    }
}
template < bool >
bool getFieldSafe( const Json& jo, const std::string& strMemberPropertyName ) {
    try {
        if( strMemberPropertyName.empty() )
            return false;
        std::string r( jo.count( strMemberPropertyName ) ? jo[ strMemberPropertyName ].get< std::string >() :
                                                           std::string( "false" ) );
        return convert_json_string_value_to_boolean( r );
    }
    catch( ... ) {
        return false;
    }
}

template < typename T >
const T getFieldSafe( const Json& jo, const char* c_strMemberPropertyName ) {
    try {
        if( c_strMemberPropertyName == nullptr || c_strMemberPropertyName[ 0 ] == '\0' )
            return T();
        std::string strMemberPropertyName = c_strMemberPropertyName;
        T retVal( jo.count( strMemberPropertyName ) ? jo[ strMemberPropertyName ].get< T >() : T() );
        return retVal;
    }
    catch( ... ) {
        return T();
    }
}
template < bool >
bool getFieldSafe( const Json& jo, const char* c_strMemberPropertyName ) {
    try {
        if( c_strMemberPropertyName == nullptr || c_strMemberPropertyName[ 0 ] == '\0' )
            return false;
        std::string strMemberPropertyName = c_strMemberPropertyName;
        std::string r( jo.count( strMemberPropertyName ) ? jo[ strMemberPropertyName ].get< std::string >() :
                                                           std::string( "false" ) );
        return convert_json_string_value_to_boolean( r );
    }
    catch( ... ) {
        return false;
    }
}

template < typename T >
const T getFieldSafe( const Json& jo, const std::string& strMemberPropertyName, const T& defaultValue ) {
    try {
        if( strMemberPropertyName.empty() )
            return defaultValue;
        T retVal( jo.count( strMemberPropertyName ) ? jo[ strMemberPropertyName ].get< T >() : defaultValue );
        return retVal;
    }
    catch( ... ) {
        return defaultValue;
    }
}
template < bool >
bool getFieldSafe( const Json& jo, const std::string& strMemberPropertyName, bool defaultValue ) {
    try {
        if( strMemberPropertyName.empty() )
            return false;
        std::string r( jo.count( strMemberPropertyName ) ?
                           jo[ strMemberPropertyName ].get< std::string >() :
                           ( defaultValue ? std::string( "true" ) : std::string( "false" ) ) );
        return convert_json_string_value_to_boolean( r );
    }
    catch( ... ) {
        return defaultValue;
    }
}

template < typename T >
const T getFieldSafe( const Json& jo, const char* c_strMemberPropertyName, const T& defaultValue ) {
    try {
        if( c_strMemberPropertyName == nullptr || c_strMemberPropertyName[ 0 ] == '\0' )
            return defaultValue;
        std::string strMemberPropertyName = c_strMemberPropertyName;
        T retVal( jo.count( strMemberPropertyName ) ? jo[ strMemberPropertyName ].get< T >() : defaultValue );
        return retVal;
    }
    catch( ... ) {
        return defaultValue;
    }
}
template < bool >
bool getFieldSafe( const Json& jo, const char* c_strMemberPropertyName, bool defaultValue ) {
    try {
        if( c_strMemberPropertyName == nullptr || c_strMemberPropertyName[ 0 ] == '\0' )
            return defaultValue;
        std::string strMemberPropertyName = c_strMemberPropertyName;
        std::string r( jo.count( strMemberPropertyName ) ?
                           jo[ strMemberPropertyName ].get< std::string >() :
                           ( defaultValue ? std::string( "true" ) : std::string( "false" ) ) );
        return convert_json_string_value_to_boolean( r );
    }
    catch( ... ) {
        return defaultValue;
    }
}

template < typename T >
const T getFieldSafe( const Json& jo,
                      const std::string& strMemberPropertyName,
                      const T& defaultValue,
                      const T& minValue,
                      const T& maxValue ) {
    try {
        T retVal = getFieldSafe( jo, strMemberPropertyName, defaultValue );
        if( retVal < minValue )
            retVal = minValue;
        if( retVal > maxValue )
            retVal = maxValue;
        return retVal;
    }
    catch( ... ) {
        return defaultValue;
    }
}
template < typename T >
const T getFieldSafe( const Json& jo,
                      const char* c_strMemberPropertyName,
                      const T& defaultValue,
                      const T& minValue,
                      const T& maxValue ) {
    try {
        T retVal = getFieldSafe( jo, c_strMemberPropertyName, defaultValue );
        if( retVal < minValue )
            retVal = minValue;
        if( retVal > maxValue )
            retVal = maxValue;
        return retVal;
    }
    catch( ... ) {
        return defaultValue;
    }
}
#endif  // JSONRPC_H
