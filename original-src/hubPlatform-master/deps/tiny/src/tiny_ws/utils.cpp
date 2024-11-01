namespace utils {

bool is_json( const char* strProbablyJSON, nlohmann::json* p_jo /*= nullptr*/ ) {
    if( strProbablyJSON == nullptr || strProbablyJSON[ 0 ] == '\0' )
        return false;
    try {
        nlohmann::json jo = nlohmann::json::parse( strProbablyJSON );
        if( p_jo )
            ( *p_jo ) = jo;
        return true;
    }
    catch( ... ) {
    }
    if( p_jo )
        ( *p_jo ) = nlohmann::json();
    return false;
}
bool is_json( const std::string& strProbablyJSON, nlohmann::json* p_jo /*= nullptr*/ ) {
    if( strProbablyJSON.empty() )
        return false;
    return is_json( strProbablyJSON.c_str(), p_jo );
}
bool is_json( const char* strProbablyJSON, const char* strMustHavePropertyName, const char* strMustHavePropertyValue ) {
    if( strProbablyJSON == nullptr || strProbablyJSON[ 0 ] == '\0' )
        return false;
    nlohmann::json jo;
    if( !is_json( strProbablyJSON, &jo ) )
        return false;
    return is_json( jo, strMustHavePropertyName, strMustHavePropertyValue );
}
bool is_json( const std::string& strProbablyJSON,
              const char* strMustHavePropertyName,
              const char* strMustHavePropertyValue ) {
    if( strProbablyJSON.empty() )
        return false;
    return is_json( strProbablyJSON.c_str(), strMustHavePropertyName, strMustHavePropertyValue );
}
bool is_json( const nlohmann::json& jo, const char* strMustHavePropertyName, const char* strMustHavePropertyValue ) {
    if( strMustHavePropertyName == nullptr || strMustHavePropertyName[ 0 ] == 0 )
        return true;
    try {
        if( jo.count( strMustHavePropertyName ) == 0 )
            return false;
        const nlohmann::json& jo_result = jo[ std::string( strMustHavePropertyName ) ];
        if( !jo_result.is_string() )
            return false;
        std::string strResult = jo_result.get< std::string >();
        if( tiny::tools::to_lower( strResult ) == tiny::tools::to_lower( strMustHavePropertyValue ) )
            return true;
    }
    catch( ... ) {
    }
    return false;
}
bool is_ping( const char* strProbablyJSON ) {
    return is_json( strProbablyJSON, "method", "ping" ) || is_json( strProbablyJSON, "result", "ping" );
}
bool is_ping( const std::string& strProbablyJSON ) {
    return is_json( strProbablyJSON, "method", "ping" ) || is_json( strProbablyJSON, "result", "ping" );
}
bool is_ping( const nlohmann::json& jo ) {
    return is_json( jo, "method", "ping" ) || is_json( jo, "result", "ping" );
}
bool is_pong( const char* strProbablyJSON ) {
    return is_json( strProbablyJSON, "method", "pong" ) || is_json( strProbablyJSON, "result", "pong" );
}
bool is_pong( const std::string& strProbablyJSON ) {
    return is_json( strProbablyJSON, "method", "pong" ) || is_json( strProbablyJSON, "result", "pong" );
}
bool is_pong( const nlohmann::json& jo ) {
    return is_json( jo, "method", "pong" ) || is_json( jo, "result", "pong" );
}

bool is_ping_or_pong( const char* strProbablyJSON ) {
    if( strProbablyJSON == nullptr || strProbablyJSON[ 0 ] == '\0' )
        return false;
    nlohmann::json jo;
    if( !is_json( strProbablyJSON, &jo ) )
        return false;
    return is_ping_or_pong( jo );
}
bool is_ping_or_pong( const std::string& strProbablyJSON ) {
    if( strProbablyJSON.empty() )
        return false;
    return is_ping_or_pong( strProbablyJSON.c_str() );
}
bool is_ping_or_pong( const nlohmann::json& jo ) {
    if( is_json( jo, "method", "ping" ) || is_json( jo, "result", "ping" ) || is_json( jo, "result", "pong" ) ||
        is_json( jo, "method", "pong" ) )
        return true;
    return false;
}

}  // namespace utils