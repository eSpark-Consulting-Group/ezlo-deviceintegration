#pragma once

#include "json_utils/json_fwd.h"
#include <vector>

#include "json_utils/json_utils.h"

namespace HubPlatform {

class ScriptResult final {
public:
    using Values = std::vector< Json >;

public:
    static ScriptResult fromJson( const JsonObject& resultJson );
    JsonObject toJson() const;

public:
    explicit ScriptResult( std::string error );
    explicit ScriptResult( Values values );

    bool hasError() const;
    const std::string& getError() const;

    int getValuesCount() const;
    const Json& getValue( int idx ) const;

private:
    const std::string m_error;

    const Values m_result;
};

}  // namespace HubPlatform
