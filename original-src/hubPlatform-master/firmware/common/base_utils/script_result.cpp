#include "base_utils/script_result.h"

namespace HubPlatform {

ScriptResult ScriptResult::fromJson( const JsonObject& resultJson ) {
    auto errorString = json_get_str( resultJson, "error" );
    if( !errorString.empty() )
        return ScriptResult( std::move( errorString ) );

    Json jsValues;
    json_get_arr( resultJson, "result", jsValues );

    std::vector< Json > values;
    for( const auto& element : jsValues )
        values.push_back( element );

    return ScriptResult( std::move( values ) );
}

JsonObject ScriptResult::toJson() const {
    if( hasError() )
        return JSON( "error" << m_error );
    else {
        return JSON( "result" << m_result );
    }
}

ScriptResult::ScriptResult( std::string error )
: m_error{ std::move( error ) } {}

ScriptResult::ScriptResult( Values values )
: m_result( std::move( values ) ) {}

bool ScriptResult::hasError() const {
    return !m_error.empty();
}

const std::string& ScriptResult::getError() const {
    return m_error;
}

int ScriptResult::getValuesCount() const {
    return m_result.size();
}

const Json& ScriptResult::getValue( int idx ) const {
    return m_result[ idx ];
}

}  // namespace HubPlatform
