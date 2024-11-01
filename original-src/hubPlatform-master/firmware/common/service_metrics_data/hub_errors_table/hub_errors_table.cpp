#include "service_metrics_data/hub_errors_table/hub_errors_table.h"

#include "json_utils/json_utils.h"
#include "database_client/idbclient.h"
#include "database_client/db_tables_names.h"

#include <ctime>

namespace HubPlatform {
namespace ServiceMetricsData {

namespace {

namespace ErrorNames {
const char* const KernelCrash = "kernel_crash";
const char* const UninterruptibleSleep = "uninterruptible_sleep";
const char* const BCDCError = "bcdc_error";
}  // namespace ErrorNames

namespace Fields {
const char* const ErrorId = "_id";
const char* const Count = "count";
const char* const LastUpdated = "last_updated";
}  // namespace Fields

namespace JsonFields {
const char* const ErrorId = "id";
const char* const Count = "count";
const char* const LastUpdated = "last_updated";
}  // namespace JsonFields

std::string errorTypeToStr( HubErrorsTable::ErrorType errorType ) {
    static const std::map< HubErrorsTable::ErrorType, std::string > map = {
        { HubErrorsTable::ErrorType::KernelCrash, ErrorNames::KernelCrash },
        { HubErrorsTable::ErrorType::UninterruptibleSleep, ErrorNames::UninterruptibleSleep },
        { HubErrorsTable::ErrorType::BCDCError, ErrorNames::BCDCError },
    };
    auto it = map.find( errorType );
    if( it == map.cend() )
        throw HubErrorsTableError( "Unable to convert ErrorType to string. Enum value: " +
                                   std::to_string( static_cast< int >( errorType ) ) );

    return it->second;
}

}  // namespace

HubErrorsTable::HubErrorsTable( IDbClient& dbClient )
: m_dbClient( dbClient ) {}

void HubErrorsTable::incrementError( ErrorType errorType ) {
    auto error = getError( errorType );
    ++error.count;
    error.lastUpdated = std::time( nullptr );

    auto j = JSON( Fields::ErrorId << error.errorId << Fields::Count << error.count << Fields::LastUpdated
                                   << error.lastUpdated );
    m_dbClient.update( HubErrorsDBName, JSON_EMPTY_OBJ, j, true, false );
}

HubErrorsTable::HubError HubErrorsTable::getError( ErrorType errorType ) {
    const auto errorId = errorTypeToStr( errorType );

    HubError hubError;
    hubError.errorId = errorId;

    auto query = JSON( Fields::ErrorId << errorId );
    auto result = m_dbClient.findOne( HubErrorsDBName, query, nullptr );
    if( result.empty() )
        return hubError;

    hubError.count = json_get_int( result, Fields::Count );
    hubError.lastUpdated = json_get_int( result, Fields::LastUpdated );

    return hubError;
}

std::vector< HubErrorsTable::HubError > HubErrorsTable::getAllErrors() {
    auto result = m_dbClient.findAll( HubErrorsDBName, JSON_EMPTY_OBJ, nullptr );
    if( result.empty() )
        return {};

    std::vector< HubErrorsTable::HubError > vec;
    vec.reserve( result.size() );

    for( auto& element : result ) {
        HubErrorsTable::HubError hubError;
        hubError.errorId = json_get_str( element, Fields::ErrorId );
        hubError.count = json_get_int( element, Fields::Count );
        hubError.lastUpdated = json_get_int( element, Fields::LastUpdated );
        vec.emplace_back( std::move( hubError ) );
    }
    return vec;
}

std::vector< HubErrorsTable::HubError > HubErrorsTable::fromJsonArray( const JsonArray& errors ) {
    std::vector< HubErrorsTable::HubError > vec;
    vec.reserve( errors.size() );

    for( auto& element : errors ) {
        HubErrorsTable::HubError hubError;
        hubError.errorId = json_get_str( element, JsonFields::ErrorId );
        hubError.count = json_get_int( element, JsonFields::Count );
        hubError.lastUpdated = json_get_int( element, JsonFields::LastUpdated );
        vec.emplace_back( std::move( hubError ) );
    }
    return vec;
}

JsonArray HubErrorsTable::toJsonArray( const std::vector< HubError >& errors ) {
    auto arr = JSON_EMPTY_ARR;

    for( auto& error : errors ) {
        auto j = JSON( JsonFields::ErrorId << error.errorId << JsonFields::Count << error.count
                                           << JsonFields::LastUpdated << error.lastUpdated );
        arr.push_back( j );
    }
    return arr;
}

}  // namespace ServiceMetricsData
}  // namespace HubPlatform
