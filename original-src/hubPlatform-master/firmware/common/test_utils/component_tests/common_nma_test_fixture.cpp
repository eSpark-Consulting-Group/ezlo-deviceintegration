#include "test_utils/component_tests/common_nma_test_fixture.h"
#include "test_utils/component_tests/component_test_application.h"

#include "log_utils/ezlo_log.h"
#include "json_utils/json_utils.h"
#include "jsonrpc/message.h"

namespace HubPlatform {
namespace Tests {

void CommonNmaTests::SetUp() {
    ASSERT_TRUE( m_broadcastMessages.empty() ) << "Broadcasts must not be preserved between tests";
    nma().setBroadcastsProcessFunction( [this]( rpc::Message& message ) {
        std::unique_lock< std::mutex > lock( m_broadcastsMutex );
        m_broadcastMessages.push_back( message.getMessage() );
        lock.unlock();
        m_broadcastsCondVar.notify_all();
    } );
    nma().setInternalBroadcastsProcessFunction( [this]( const Services::Message& message ) {
        std::unique_lock< std::mutex > lock( m_internalBroadcastsMutex );
        m_internalBroadcastMessages.push_back( message );
        lock.unlock();
        m_internalBroadcastsCondVar.notify_all();
    } );
    // TODO: check setting value first, don't set if it is already set to right value'
    auto targetLogLevel = Log::PriorityLevel::TRACE;
    if( Log::getLogLevel() != targetLogLevel ) {
        nma().sendtoHubAndGetAnswer( { { "method", "hub.log.local.set" },
                                       { "params", JSON( "severity" << Log::toString( targetLogLevel ) ) } } );

        ignoreBroadcasts(
            []( const Json& broadcast ) { return json_get_str( broadcast, "msg_subclass" ) == "hub.setting.updated"; },
            __FILE__,
            __LINE__,
            std::chrono::milliseconds( 200 ) );
    }
    updatePredefineGroupsForTest( "normal" );
}

void CommonNmaTests::TearDown() {
    nma().resetBroadcastsProcessFunction();
}

std::string CommonNmaTests::getCodeLineString( int codeLine ) const {
    if( m_callingFile.empty() )
        return "code line: " + std::to_string( codeLine );
    return std::string{ "at: " }.append( m_callingFile ).append( ":" ).append( std::to_string( codeLine ) );
}

void CommonNmaTests::callingFile( std::string value ) {
    m_callingFile = std::move( value );
}

JsonObject CommonNmaTests::waitForBroadcast( const std::string& messageSubclass,
                                             const char* file,
                                             int line,
                                             std::chrono::milliseconds timeout ) {
    std::string location = "file: " + std::string( file ) + ", line: " + std::to_string( line );
    return waitForBroadcast( messageSubclass, location, timeout );
}

JsonObject CommonNmaTests::waitForBroadcast( const std::string& messageSubclass,
                                             const std::string& comment,
                                             std::chrono::milliseconds timeout ) {
    std::unique_lock< std::mutex > lock( m_broadcastsMutex );

    while( timeout >= std::chrono::seconds( 0 ) ) {
        for( auto it = m_broadcastMessages.begin(); it != m_broadcastMessages.end(); ++it ) {
            if( json_get_str( *it, "msg_subclass" ) == messageSubclass ) {
                auto broadcast = std::move( *it );
                m_broadcastMessages.erase( it );
                return broadcast;
            }
        }

        if( timeout == std::chrono::seconds( 0 ) )
            break;

        size_t previousSize = m_broadcastMessages.size();
        auto beforeWaiting = std::chrono::steady_clock::now();
        if( !m_broadcastsCondVar.wait_for(
                lock, timeout, [this, &previousSize] { return m_broadcastMessages.size() > previousSize; } ) ) {
            break;
        }
        auto elapsedTime =
            std::chrono::duration_cast< std::chrono::milliseconds >( std::chrono::steady_clock::now() - beforeWaiting );

        timeout = timeout - elapsedTime;
    }

    std::stringstream errorMessage;
    errorMessage << "No " << messageSubclass << " broadcasts recieved!\n";
    errorMessage << "Msg: " << comment << "\n";
    errorMessage << "Queued broadcasts: " << m_broadcastMessages.size() << "\n";
    errorMessage << "----------\n";

    for( auto& broadcast : m_broadcastMessages ) {
        errorMessage << broadcast.dump() << "\n";
        errorMessage << "----------\n";
    }

    ADD_FAILURE() << errorMessage.str();
    throw std::exception();
}

JsonObject CommonNmaTests::waitForAnyOfBroadcasts( std::set< std::string > messageSubclassSet,
                                                   const std::string& comment,
                                                   std::chrono::milliseconds timeout ) {
    auto predicate = [&messageSubclassSet]( const Json& broadcast ) -> bool {
        auto broadcastName = json_get_str( broadcast, "msg_subclass" );
        return messageSubclassSet.find( broadcastName ) != messageSubclassSet.cend();
    };

    return waitForBroadcast( predicate, comment, timeout );
}

void CommonNmaTests::checkNoBroadcastReceived( const std::string& comment,
                                               int codeLine,
                                               std::chrono::milliseconds timeout ) {
    std::unique_lock< std::mutex > lock( m_broadcastsMutex );
    if( !m_broadcastMessages.empty() ) {
        ADD_FAILURE() << "broadcast is received: " << comment << ", " + getCodeLineString( codeLine );
    }
    if( m_broadcastsCondVar.wait_for( lock, timeout, [this] { return !m_broadcastMessages.empty(); } ) ) {
        ADD_FAILURE() << "broadcast is received: " << comment << ", " + getCodeLineString( codeLine );
    }
}

JsonObject CommonNmaTests::waitForBroadcast( CommonNmaTests::Predicate predicate,
                                             const std::string& comment,
                                             std::chrono::milliseconds timeout ) {
    std::unique_lock< std::mutex > lock( m_broadcastsMutex );

    while( timeout >= std::chrono::seconds( 0 ) ) {
        for( auto it = m_broadcastMessages.begin(); it != m_broadcastMessages.end(); ++it ) {
            if( predicate( *it ) ) {
                auto broadcast = std::move( *it );
                m_broadcastMessages.erase( it );
                return broadcast;
            }
        }

        if( timeout == std::chrono::seconds( 0 ) )
            break;

        size_t previousSize = m_broadcastMessages.size();
        auto beforeWaiting = std::chrono::steady_clock::now();
        if( !m_broadcastsCondVar.wait_for(
                lock, timeout, [this, &previousSize] { return m_broadcastMessages.size() > previousSize; } ) ) {
            break;
        }
        auto elapsedTime =
            std::chrono::duration_cast< std::chrono::milliseconds >( std::chrono::steady_clock::now() - beforeWaiting );

        timeout = timeout - elapsedTime;
    }

    std::stringstream errorMessage;
    errorMessage << "No broadcast satisfies the predicate!\n";
    errorMessage << "Msg: " << comment << "\n";
    errorMessage << "Queued broadcasts: " << m_broadcastMessages.size() << "\n";
    errorMessage << "----------\n";

    for( auto& broadcast : m_broadcastMessages ) {
        errorMessage << broadcast.dump() << "\n";
        errorMessage << "----------\n";
    }

    ADD_FAILURE() << errorMessage.str();
    throw std::exception();
}

void CommonNmaTests::ignoreBroadcast( CommonNmaTests::Predicate predicate, std::chrono::seconds timeout ) {
    std::unique_lock< std::mutex > lock( m_broadcastsMutex );
    if( m_broadcastsCondVar.wait_for( lock, timeout, [this] { return !m_broadcastMessages.empty(); } ) ) {
        auto broadcast = m_broadcastMessages.front();
        if( predicate( broadcast ) )
            m_broadcastMessages.pop_front();
    }
}

void CommonNmaTests::ignoreBroadcasts( CommonNmaTests::Predicate predicate,
                                       const char* file,
                                       int line,
                                       std::chrono::seconds period ) {
    ignoreBroadcasts( predicate, file, line, std::chrono::duration_cast< std::chrono::milliseconds >( period ) );
}

void CommonNmaTests::ignoreBroadcasts( CommonNmaTests::Predicate predicate,
                                       const char* file,
                                       int line,
                                       std::chrono::milliseconds period ) {
    std::this_thread::sleep_for( period );
    std::unique_lock< std::mutex > lock( m_broadcastsMutex );
    bool ok = true;
    for( auto& broadcast : m_broadcastMessages ) {
        if( !predicate( broadcast ) ) {
            ADD_FAILURE_AT( file, line ) << "Received broadcast that does not satisfy predicate: "
                                         << cc::j( broadcast );
            ok = false;
        }
    }
    if( !ok ) {
        throw std::exception();
    }
    m_broadcastMessages.clear();
}

Services::Message CommonNmaTests::waitForInternalBroadcast( const char* file, int line, std::chrono::seconds timeout ) {
    std::unique_lock< std::mutex > lock( m_internalBroadcastsMutex );
    if( m_internalBroadcastMessages.empty() ) {
        if( !m_internalBroadcastsCondVar.wait_for(
                lock, timeout, [this] { return !m_internalBroadcastMessages.empty(); } ) ) {
            ADD_FAILURE_AT( file, line ) << "No broadcasts received";
            throw std::exception();
        }
    }
    auto tmp = m_internalBroadcastMessages.front();
    m_internalBroadcastMessages.pop_front();
    return tmp;
}

void CommonNmaTests::checkNoInternalBroadcastReceived( const char* file, int line, std::chrono::seconds timeout ) {
    std::unique_lock< std::mutex > lock( m_broadcastsMutex );
    if( !m_broadcastMessages.empty() ) {
        ADD_FAILURE_AT( file, line ) << "Received unexpected broadcast: " + m_broadcastMessages.front().dump();
    }
    if( m_broadcastsCondVar.wait_for( lock, timeout, [this] { return !m_broadcastMessages.empty(); } ) ) {
        ADD_FAILURE_AT( file, line ) << "Received unexpected broadcast: " + m_broadcastMessages.front().dump();
    }
}

JsonObject CommonNmaTests::replaceTokens( const JsonObject& json, const KeyStringList& replaceList ) {
    return replaceTokens( json.dump(), replaceList );
}

JsonObject CommonNmaTests::replaceTokens( std::string json, const KeyStringList& replaceList ) {
    return JsonObject::parse( replaceTokensInString( std::move( json ), replaceList ) );
}

std::string CommonNmaTests::replaceTokensInString( std::string str, const KeyStringList& replaceList ) {
    for( const auto& replacePair : replaceList ) {
        const std::string& toReplace = replacePair.first;
        const std::string& replaceWith = replacePair.second;
        if( toReplace == replaceWith )
            continue;
        bool replaced = false;
        size_t pos = 0;
        while( ( pos = str.find( toReplace ) ) != std::string::npos ) {
            replaced = true;
            str.replace( pos, toReplace.size(), replaceWith );
        }
        if( !replaced ) {
            ADD_FAILURE() << "Unable to replace token ( " << toReplace << " ) with value ( " << replaceWith << " )";
        }
    }
    return str;
}

std::string CommonNmaTests::replaceTokensInStringNoFail( std::string str, const KeyStringList& replaceList ) {
    for( const auto& replacePair : replaceList ) {
        const std::string& toReplace = replacePair.first;
        const std::string& replaceWith = replacePair.second;
        if( toReplace == replaceWith )
            continue;
        bool replaced = false;
        size_t pos = 0;
        while( ( pos = str.find( toReplace ) ) != std::string::npos ) {
            replaced = true;
            str.replace( pos, toReplace.size(), replaceWith );
        }
    }
    return str;
}

void CommonNmaTests::checkBroadcast( const std::string& id,
                                     const std::string& broadcastName,
                                     const KeyJsonList& fields,
                                     int codeLine ) {
    std::string codeLineStr = getCodeLineString( codeLine );
    checkBroadcast( id, broadcastName, fields, codeLineStr );
}

/** @brief Compares broadcast fields:  name, _id and, possibly, list of fields
 *  For fields, there are $has and $has-not keys, which simply checks if the key passed as a parameter, exists.
 * */
void CommonNmaTests::compareBroadcastFields( const Json& broadcast,
                                             const std::string& id,
                                             const std::string& broadcastName,
                                             const KeyJsonList& fields,
                                             const std::string& message ) {
    JsonObject result;
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), broadcastName ) << message;
    EXPECT_TRUE( json_get_obj( broadcast, "result", result ) ) << message;

    if( !id.empty() ) {
        EXPECT_EQ( json_get_str( result, "_id" ), id ) << message;
    }
    for( const auto& field : fields ) {
        JsonObject value;
        if( field.first == Resources::CheckBroadcast::HasNot ) {
            EXPECT_EQ( result.count( field.second ), 0 )
                << "Should have no field '" << field.second << "', " << message;
        }
        else if( field.first == Resources::CheckBroadcast::Has ) {
            EXPECT_GT( result.count( field.second ), 0 ) << "Should have a field '" << field.second << "', " << message;
        }
        else {
            EXPECT_TRUE( json_get( result, field.first, value ) )
                << message << ", field: " << field.first << " from: " << result;
            EXPECT_EQ( value.dump(), field.second.dump() )
                << message << ", field: " << field.first << " from: " << result;
        }
    }
}

bool CommonNmaTests::compareBroadcastAndLookingForFields( const Json& broadcast,
                                                          const std::string& id,
                                                          const std::string& broadcastName,
                                                          const KeyJsonList& fields,
                                                          const std::string& message ) {
    JsonObject result;
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), broadcastName ) << message;
    EXPECT_TRUE( json_get_obj( broadcast, "result", result ) ) << message;

    if( !id.empty() ) {
        EXPECT_EQ( json_get_str( result, "_id" ), id ) << message;
    }
    for( const auto& field : fields ) {
        JsonObject value;

        if( json_get( result, field.first, value ) )
            return true;
    }

    return false;
}

/** @brief Waits for broadcast and checks its fields. @see compareBroadcastFields.
 * */
void CommonNmaTests::checkBroadcast( const std::string& id,
                                     const std::string& broadcastName,
                                     const KeyJsonList& fields,
                                     const std::string& message ) {
    JsonObject broadcast, result;
    broadcast = waitForBroadcast( broadcastName, message );
    compareBroadcastFields( broadcast, id, broadcastName, fields, message );
}

void CommonNmaTests::checkBroadcasts( const BroadcastCheckers& broadcastCheckers, int codeLine ) {
    std::string codeLineStr = getCodeLineString( codeLine );
    auto predicate = [&broadcastCheckers]( const Json& broadcast ) -> bool {
        auto broadcastName = json_get_str( broadcast, "msg_subclass" );
        auto broadcastIt = broadcastCheckers.find( broadcastName );
        return broadcastIt != broadcastCheckers.end();
    };

    for( std::size_t i = 0; i < broadcastCheckers.size(); ++i ) {
        JsonObject broadcast = waitForBroadcast( predicate, codeLineStr );
        auto broadcastName = json_get_str( broadcast, "msg_subclass" );
        EXPECT_FALSE( broadcastName.empty() ) << codeLineStr;
        auto broadcastIt = broadcastCheckers.find( broadcastName );
        if( broadcastIt == broadcastCheckers.end() ) {
            FAIL() << "no broadcast name received " << codeLineStr;
        }
        broadcastIt->second( broadcast, codeLine );
    }
}

void CommonNmaTests::checkBroadcasts( const BroadcastCheckerVector& broadcastCheckers, int codeLine ) {
    std::string codeLineStr = getCodeLineString( codeLine );
    for( std::size_t i = 0; i < broadcastCheckers.size(); ++i ) {
        auto predicate = [&broadcastCheckers]( const Json& broadcast ) -> bool {
            auto broadcastName = json_get_str( broadcast, "msg_subclass" );
            for( const auto& checker : broadcastCheckers ) {
                if( checker( broadcast ) ) {
                    return true;
                }
            }
            return false;
        };
        waitForBroadcast( predicate, codeLineStr );
    }
}

void CommonNmaTests::sendRequest( const Json& request, int codeLine ) {
    auto answer = nma().sendtoHubAndGetAnswer( request );
    auto error = answer[ "error" ];
    ASSERT_TRUE( error.empty() ) << "code line: " << codeLine << std::endl << error.dump( 4 );
}

void CommonNmaTests::assertTrue( bool condition, const std::string& comment ) {
    EXPECT_TRUE( condition ) << comment;
    if( !condition ) {
        throw std::exception();
    }
}

Json CommonNmaTests::safeParseJson( const char* file, int line, const std::string& jsonStr ) {
    Json result = ::parseJson( jsonStr );
    if( result.empty() ) {
        ADD_FAILURE_AT( file, line ) << "You probably have malformed json, because parsed result is empty";
        throw std::exception();
    }
    return result;
}

ComponentTestApplication& CommonNmaTests::nma() {
    return ComponentTestApplication::get();
}

const std::string& CommonNmaTests::getCallingFile() const {
    return m_callingFile;
}

void CommonNmaTests::updatePredefineGroupsForTest( const std::string& entryDelayValue ) {
    auto deviceGroup = nma().getDBClient().findOne( "db.firmware.cored.device_groups",
                                                    JSON( "_id"
                                                          << "life_safety" ),
                                                    nullptr );
    deviceGroup[ "entryDelay" ] = entryDelayValue;
    nma().getDBClient().update( "db.firmware.cored.device_groups",
                                JSON( "_id"
                                      << "life_safety" ),
                                deviceGroup );

    deviceGroup = nma().getDBClient().findOne( "db.firmware.cored.device_groups",
                                               JSON( "_id"
                                                     << "fire_sensors" ),
                                               nullptr );
    deviceGroup[ "entryDelay" ] = entryDelayValue;
    nma().getDBClient().update( "db.firmware.cored.device_groups",
                                JSON( "_id"
                                      << "fire_sensors" ),
                                deviceGroup );
}

}  // namespace Tests
}  // namespace HubPlatform
