#pragma once

#include "json_utils/json_fwd.h"

#include <string>
#include <cstdint>
#include <vector>
#include <stdexcept>

class IDbClient;

namespace HubPlatform {
namespace ServiceMetricsData {

class HubErrorsTableError : public std::runtime_error {
public:
    using runtime_error::runtime_error;
    explicit HubErrorsTableError( const std::string& msg )
    : runtime_error( msg ){};
};

class HubErrorsTable {
public:
    struct HubError {
        std::string errorId;
        uint32_t count = 0;
        uint32_t lastUpdated = 0;
    };

    enum class ErrorType { KernelCrash, UninterruptibleSleep, BCDCError };

    HubErrorsTable( IDbClient& dbClient );

    void incrementError( ErrorType errorType );
    HubError getError( ErrorType errorType );
    std::vector< HubError > getAllErrors();

    static std::vector< HubError > fromJsonArray( const JsonArray& errors );
    static JsonArray toJsonArray( const std::vector< HubError >& errors );

private:
    IDbClient& m_dbClient;
};

}  // namespace ServiceMetricsData
}  // namespace HubPlatform
