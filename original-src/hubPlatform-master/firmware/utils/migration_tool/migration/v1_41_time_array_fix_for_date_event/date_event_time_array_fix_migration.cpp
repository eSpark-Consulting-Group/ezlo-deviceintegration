#include "migration_tool/migration/v1_41_time_array_fix_for_date_event/date_event_time_array_fix_migration.h"

#include "database_client/idbclient.h"
#include "database_client/dbqueryparser.h"
#include "log_utils/ezlo_log.h"
#include "json_utils/json_utils.h"

namespace HubPlatform {
namespace MigrationTool {
namespace v1_41_date_event_time_array_fix {

namespace Fields {
const char* Value = "value";
const char* Type = "type";
const char* Id = "_id";
const char* Fields = "fields";
const char* Name = "name";

const char* BlockOptions = "blockOptions";
const char* Method = "method";
const char* WhenBlock = "when";
}  // namespace Fields

namespace MethodNames {
const char* IsDate = "isDate";
const char* Function = "function";
const char* And = "and";
const char* Not = "not";
const char* Or = "or";
const char* Xor = "xor";
}  // namespace MethodNames

DateEventTimeFixMigration::DateEventTimeFixMigration( IDbClient& dbClient )
: m_dbClient( dbClient )
, m_updatedBlocksCount( 0 ) {}

bool DateEventTimeFixMigration::migrate() {
    auto scenes = m_dbClient.findAll( ScenesDb, JSON_EMPTY_OBJ, nullptr );
    for( auto& scene : scenes ) {
        try {
            m_updatedBlocksCount = 0;
            processScene( scene );
            if( m_updatedBlocksCount > 0 ) {
                LOGGER_INFO( cc::trace( "Scene is to be updated: " ) + json_get_str( scene, Fields::Id ) +
                             cc::trace( ", changed item values: " ) + cc::num10( m_updatedBlocksCount ) );
                LOGGER_TRACE( cc::j( scene ) );
                saveScene( scene );
            }
            else {
                LOGGER_INFO( "Scene to be left unchanged" );
            }
        }
        catch( const std::exception& err ) {
            LOGGER_ERROR( cc::error( std::string{ "Exception has caught: " } + err.what() ) );
            LOGGER_ERROR( cc::error( "Migration has failed" ) );
            return false;
        }
    }

    return true;
}

void DateEventTimeFixMigration::processScene( Json& scene ) {
    auto whenBlocksIt = scene.find( Fields::WhenBlock );
    if( whenBlocksIt != scene.end() && whenBlocksIt->is_array() && !whenBlocksIt->empty() ) {
        Json& whenBlocks = *whenBlocksIt;
        for( size_t i = 0; i < whenBlocks.size(); ++i ) {
            const std::string newKeyPath = "/ when / " + std::to_string( i );
            processBlock( whenBlocks[ i ], newKeyPath );
        }
    }
}

void DateEventTimeFixMigration::processBlock( Json& block, const std::string& keyPath ) {
    if( !block.is_object() ) {
        LOGGER_ERROR( cc::error( "when block is not an object" ) );
        return;
    }

    const std::string methodName = fetchBlockMethodName( block );
    if( methodName == MethodNames::IsDate ) {
        processDateEventBlock( block );
    }
    else if( isOperatorBlockWithChildren( methodName ) ) {
        LOGGER_DEBUG_RAW( cc::error( "Dive in fields for method " + methodName ) );

        auto fieldsIt = block.find( Fields::Fields );
        if( fieldsIt != block.end() && fieldsIt->is_array() )
            iterateCompoundBlockFields( *fieldsIt, keyPath );
    }
}

void DateEventTimeFixMigration::iterateCompoundBlockFields( Json& fields, const std::string& keyPath ) {
    for( Json& field : fields ) {
        if( !field.is_object() )
            continue;

        // iterate over blocks in fields / value
        auto typeIt = field.find( Fields::Type );
        auto valueIt = field.find( Fields::Value );
        if( typeIt != field.end() && valueIt != field.end() ) {
            if( typeIt.value() == "block" && valueIt->is_object() ) {  // NOT operator
                const std::string newKeyPath = keyPath + " / fields / value";
                processBlock( *valueIt, keyPath );
            }
            else if( typeIt.value() == "blocks" && valueIt->is_array() ) {  // all other operators
                Json& childBlocks = *valueIt;
                for( size_t i = 0; i != childBlocks.size(); ++i ) {
                    const std::string newKeyPath = keyPath + " / fields / value / " + std::to_string( i );
                    processBlock( childBlocks[ i ], newKeyPath );
                }
            }
        }
    }
}

void DateEventTimeFixMigration::processDateEventBlock( Json& block ) {
    if( !block.is_object() )
        return;

    auto fieldsIt = block.find( Fields::Fields );
    if( fieldsIt == block.end() )
        return;

    std::string type;
    Json* timeValuePtr = nullptr;
    for( Json& field : *fieldsIt ) {
        if( !field.is_object() )
            continue;

        auto nameIt = field.find( Fields::Name );
        auto valueIt = field.find( Fields::Value );
        if( nameIt != field.end() && valueIt != field.end() ) {
            if( *nameIt == "type" && valueIt->is_string() )
                type = valueIt->get< std::string >();
            if( *nameIt == "time" )
                timeValuePtr = &valueIt.value();
        }
    }

    if( type != "weekly" && type != "monthly" )
        return;

    if( timeValuePtr == nullptr ) {
        LOGGER_ERROR( cc::error( "Date event doesn't have a value for 'time', which is mandatory" ) );
        return;
    }

    Json& timeValue = *timeValuePtr;
    bool isTimeValueValid = timeValue.is_array() && std::all_of( timeValue.begin(), timeValue.end(), isTimeEntryValid );
    if( !isTimeValueValid ) {
        timeValue = JSON_EMPTY_ARR;
        m_updatedBlocksCount++;
    }
}

void DateEventTimeFixMigration::saveScene( const Json& updatedScene ) {
    m_dbClient.update( ScenesDb,
                       JSON( db::ID_FIELD_NAME << updatedScene[ db::ID_FIELD_NAME ] ),
                       JSON( "$set" << updatedScene ),
                       false,
                       false );
}

std::string DateEventTimeFixMigration::fetchBlockMethodName( const Json& scene ) {
    using namespace Fields;
    Json found;
    if( json_get( scene, std::vector< Json >{ BlockOptions, Method, Name }, found ) ) {
        return found.get< std::string >();
    }
    return std::string{};
}

bool DateEventTimeFixMigration::isOperatorBlockWithChildren( const std::string& methodName ) {
    using namespace MethodNames;
    return methodName == Or || methodName == And || methodName == Not || methodName == Xor || methodName == Function;
}

bool DateEventTimeFixMigration::isTimeEntryValid( const Json& entry ) {
    if( !entry.is_string() )
        return false;

    const std::string entryStr = json_get_str( entry );
    const size_t secondsDelimiter = entryStr.find( ':' );
    if( secondsDelimiter == std::string::npos )
        return false;

    try {
        const std::string hoursStr = entryStr.substr( 0, secondsDelimiter );
        const std::string secondsStr = entryStr.substr( secondsDelimiter + 1 );
        const int hours = std::stoi( hoursStr );  // check if the conversion is possible
        const int seconds = std::stoi( secondsStr );
    }
    catch( std::exception& ex ) {
        LOGGER_ERROR( cc::error( ex.what() ) );
        return false;
    }

    return true;
}

}  // namespace v1_41_date_event_time_array_fix
}  // namespace MigrationTool
}  // namespace HubPlatform
