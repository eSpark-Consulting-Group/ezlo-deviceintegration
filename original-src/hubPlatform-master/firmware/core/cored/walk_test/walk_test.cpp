#include "walk_test.h"
#include "service_interfaces/service.h"
#include "base_utils/timeutils.h"
#include "config_reader/config_keys.h"
#include "cored/house_modes/script_events/walk_test_events.h"
#include "service_interfaces/postponed_task.h"

namespace HubPlatform {
namespace CoreD {
namespace WalkTest {

using namespace Resources;
using namespace HubPlatform::CoreDHouseModes;

WalkTest::WalkTest( Services::Service& service, const ConfigReader& configReader )
: m_service( service ) {
    m_extendedPeriod = getConfigExtendPeriod( configReader );
}

WalkTest::~WalkTest() {
    cleanTerminationStatusTask();
    setDbInfo( State::Finish );
}

void WalkTest::cleanTerminationStatusTask() {
    if( m_terminationStatusTask && m_terminationStatusTask->isActive() ) {
        m_terminationStatusTask->stop();
        m_terminationStatusTask.reset();
    }
}

std::chrono::seconds WalkTest::getConfigExtendPeriod( const ConfigReader& configReader ) {
    Json defaults;
    using namespace Resources::ConfigFields;
    int period = ExtendedPeriodSeconds;

    try {
        period = configReader.getInt( ConfigKeys::WalkTestExtendedPeriod );
    }
    catch( const std::exception& ex ) {
        LOGGER_ERROR( cc::error( "Cannot read config for Walk test reason: " ) + ex.what() );
    }

    return std::chrono::seconds( period );
}

JsonObject WalkTest::protectSystemTestStart( const JsonObject& request ) {
    Json answer = JSON_EMPTY_OBJ;
    std::string operationId;

    if( request.find( Fields::OperationId ) != request.end() ) {
        m_operationId = json_get_str( request, Fields::OperationId );
    }
    else {
        setErrorWithField( answer, Errors::WRONG_PARAMS, request.dump(), Fields::OperationId );
        return answer;
    }

    std::string type;
    if( request.find( Fields::Type ) != request.end() ) {
        type = json_get_str( request, Fields::Type );
        if( type != TestType::Walk ) {
            setErrorWithField( answer, Errors::WRONG_PARAMS, request.dump(), Fields::Type );
            return answer;
        }
    }
    else {
        setErrorWithField( answer, Errors::WRONG_PARAMS, request.dump(), Fields::Type );
        return answer;
    }
    bool protectNotification = false;
    if( request.find( Fields::ProtectNotification ) != request.end() ) {
        protectNotification = json_get_bool( request, Fields::ProtectNotification );
        if( protectNotification != true ) {
            setErrorWithField( answer, Errors::WRONG_PARAMS, request.dump(), Fields::ProtectNotification );
            return answer;
        }
    }
    else {
        setErrorWithField( answer, Errors::WRONG_PARAMS, request.dump(), Fields::ProtectNotification );
        return answer;
    }

    setDbInfo( State::Start );

    cleanTerminationStatusTask();

    m_terminationStatusTask = scheduleTerminationStatus(
        m_service, getTerminationIntervalSeconds(), [this]() { emitTerminationProgressStatus(); } );

    return answer;
}

JsonObject WalkTest::protectSystemTestStop( const JsonObject& request ) {
    Json answer = JSON_EMPTY_OBJ;
    std::string operationId;

    if( !isStarted() ) {
        setError( answer, Errors::WALK_TEST_NOT_STARTED );
        return answer;
    }

    if( request.find( Fields::OperationId ) != request.end() ) {
        m_operationId = json_get_str( request, Fields::OperationId );
    }
    else {
        setErrorWithField( answer, Errors::WRONG_PARAMS, request.dump(), Fields::OperationId );
        return answer;
    }

    if( m_operationId != getCurrentOperationId() ) {
        setError( answer, Errors::WALK_TEST_INCORRECT_OPERATION_ID );
        return answer;
    }

    if( request.find( Fields::ProtectNotification ) != request.end() ) {
        bool protectNotification = json_get_bool( request, Fields::ProtectNotification );
        if( protectNotification != true ) {
            setErrorWithField( answer, Errors::WRONG_PARAMS, request.dump(), Fields::ProtectNotification );
            return answer;
        }
    }
    else {
        setErrorWithField( answer, Errors::WRONG_PARAMS, request.dump(), Fields::ProtectNotification );
        return answer;
    }

    return answer;
}

JsonObject WalkTest::protectSystemTestExtend( const JsonObject& request ) {
    Json answer = JSON_EMPTY_OBJ;
    std::string operationId;
    if( request.find( Fields::OperationId ) == request.end() ) {
        setErrorWithField( answer, Errors::WRONG_PARAMS, request.dump(), Fields::OperationId );
        return answer;
    }

    if( m_operationId != getCurrentOperationId() ) {
        setError( answer, Errors::WALK_TEST_INCORRECT_OPERATION_ID );
        return answer;
    }

    auto now = TimeUtils::currentSystemTime();
    auto startedAt = getStartedTimeForTest( operationId );
    auto duration = getCurrentDuration();
    if( duration - ( now - startedAt ) > m_extendedPeriod ) {
        setError( answer, Errors::WALK_TEST_OUTSIDE_EXTEND_PERIOD );
        return answer;
    }

    auto extendedPeriodNotificationPeriod =
        duration - ( now - startedAt ) + std::chrono::seconds( m_testExtendPeriod ) - m_extendedPeriod;
    LOGGER_DEBUG( "m_duration before extend: " + cc::num10( m_duration.count() ) );
    LOGGER_DEBUG( "extendedPeriodNotificationPeriod: " + cc::num10( extendedPeriodNotificationPeriod.count() ) );

    extendPeriod();

    LOGGER_DEBUG( "m_duration after extend: " + cc::num10( m_duration.count() ) );

    if( m_terminationStatusTask && m_terminationStatusTask->isActive() ) {
        m_terminationStatusTask->stop();
        m_terminationStatusTask.reset();
    }

    m_terminationStatusTask = scheduleTerminationStatus(
        m_service, extendedPeriodNotificationPeriod, [this]() { emitTerminationProgressStatus(); } );
    return answer;
}

std::chrono::seconds WalkTest::getCurrentDuration() {
    auto& db = getDBClient();

    Json record = db.findOne( TempSystemWalkTestProgress, JSON_EMPTY_OBJ, nullptr );
    int duration = json_get_int( record, "duration" );
    return std::chrono::seconds( duration );
}

void WalkTest::extendPeriod() {
    m_duration += std::chrono::seconds( m_testExtendPeriod );
    updateDbRecord();
    sendProgressBroadcast( ProgressStatus::Extended );
    ScriptEvents::WalkTestEventExtended luaEvent(
        getCurrentOperationId(), TimeUtils::currentSystemTime(), getLeftTime() );
    m_service.emit( luaEvent );
}

bool WalkTest::isStarted() {
    auto& db = getDBClient();
    Json record;
    record = db.findOne( TempSystemWalkTestProgress, JSON_EMPTY_OBJ, nullptr );
    return !record.empty();
}

std::chrono::seconds WalkTest::getStartedTimeForTest( const std::string& operationId ) {
    auto& db = getDBClient();
    Json record;
    record = db.findOne( TempSystemWalkTestProgress, JSON_EMPTY_OBJ, nullptr );
    int timePoint = json_get_int( record, "startedAt" );
    return std::chrono::seconds( timePoint );
}

void WalkTest::setPeriodSeconds( std::chrono::seconds period ) {
    m_duration = period;
}

std::chrono::seconds WalkTest::getLeftTime() {
    auto& db = getDBClient();
    Json record;
    record = db.findOne( TempSystemWalkTestProgress, JSON_EMPTY_OBJ, nullptr );
    int startedAt = json_get_int( record, "startedAt" );
    auto duration = getCurrentDuration();
    std::chrono::seconds now = TimeUtils::currentSystemTime();
    auto result = duration.count() - ( now.count() - startedAt );
    return std::chrono::seconds( result );
}

std::chrono::seconds WalkTest::getDuration() {
    return m_duration;
}

std::chrono::seconds WalkTest::getIntervalSeconds() {
    return m_duration;
}

std::chrono::seconds WalkTest::getTerminationIntervalSeconds() {
    return m_duration - m_extendedPeriod;
}

void WalkTest::emitTerminationProgressStatus() {
    try {
        sendProgressBroadcast( ProgressStatus::TerminationProgress );
        ScriptEvents::WalkTestEventTerminationProgress luaEvent(
            getCurrentOperationId(), TimeUtils::currentSystemTime(), getLeftTime() );
        m_service.emit( luaEvent );
    }
    catch( const std::exception& ex ) {
        LOGGER_ERROR( "emitTerminationProgressStatus exception: " + ex.what() );
    }
}

void WalkTest::setDbInfo( State state ) {
    if( state == State::Start ) {
        addDbRecord();
        sendProgressBroadcast( ProgressStatus::Started );
        ScriptEvents::WalkTestEventStarted luaEvent(
            getCurrentOperationId(), TimeUtils::currentSystemTime(), getLeftTime() );
        m_service.emit( luaEvent );
    }
    if( state == State::Finish ) {
        sendProgressBroadcast( ProgressStatus::Finished );
        ScriptEvents::WalkTestEventFinished luaEvent(
            getCurrentOperationId(), TimeUtils::currentSystemTime(), getLeftTime() );
        m_service.emit( luaEvent );
        clearDbRecord();
    }
    if( state == State::Update ) {
        updateDbRecord();
    }
}

void WalkTest::addDbRecord() {
    auto& db = getDBClient();
    db.clear( TempSystemWalkTestProgress );

    Json info = JSON( "type"
                      << "walk"
                      << "operationId" << m_operationId << "duration" << m_duration.count() << "startedAt"
                      << TimeUtils::currentSystemTime().count() );
    db.insert( TempSystemWalkTestProgress, info );
}

void WalkTest::updateDbRecord() {
    auto& db = getDBClient();
    Json info = JSON( "$set" << JSON( "duration" << m_duration.count() ) );
    db.update( TempSystemWalkTestProgress, JSON_EMPTY_OBJ, info );
}

std::string WalkTest::getCurrentOperationId() {
    auto& db = getDBClient();
    Json record;
    record = db.findOne( TempSystemWalkTestProgress, JSON_EMPTY_OBJ, nullptr );

    if( !record.empty() ) {
        return json_get_str( record, "operationId" );
    }

    return "";
}

void WalkTest::clearDbRecord() {
    auto tableName = TempSystemWalkTestProgress;
    getDBClient().clear( tableName );
}

IDbClient& WalkTest::getDBClient() {
    return m_service.getDBClient();
}

CompletedValues WalkTest::completedValueByStatus( const std::string& status ) {
    if( status == ProgressStatus::Started ) {
        return CompletedValues::Started;
    }
    else if( status == ProgressStatus::TerminationProgress ) {
        return CompletedValues::TerminationProgress;
    }
    else if( status == ProgressStatus::Extended ) {
        return CompletedValues::Extended;
    }
    else if( status == ProgressStatus::Finished ) {
        return CompletedValues::Finished;
    }
    else if( status == ProgressStatus::Failed ) {
        return CompletedValues::Failed;
    }

    return CompletedValues::Failed;
}

void WalkTest::sendProgressBroadcast( const std::string& status ) {
    auto operationId = getCurrentOperationId();
    bool protectNotification = false;
    if( status == ProgressStatus::Started || status == ProgressStatus::Failed || status == ProgressStatus::Finished ) {
        protectNotification = true;
    }

    JsonObject payload = JSON( "operationId" << operationId << "type" << TestType::Walk << "completed"
                                             << static_cast< int >( completedValueByStatus( status ) ) << "status"
                                             << status << "protectNotification" << protectNotification );

    sendProgress( payload );
}

void WalkTest::sendProgress( const Json& payload ) {
    m_service.sendBroadcast( WALK_TEST_PROGRESS, payload, GroupsNames::BROADCAST_GROUP );
}

Services::PostponedTaskPtr WalkTest::scheduleTerminationStatus(
    Services::Service& service,
    std::chrono::seconds interval,
    const WalkTestTerminationCallback& onWalkTestTermination ) {
    Services::PostponedTaskPtr task = service.createPostponedTask( [onWalkTestTermination]( Services::PostponedTask& ) {
        if( onWalkTestTermination ) {
            onWalkTestTermination();
        }
    } );
    task->start( interval, false );
    return task;
}

void WalkTest::setDurationSeconds( std::chrono::seconds period ) {
    m_duration = period;
}

void WalkTest::setExtendSeconds( std::chrono::seconds period ) {
    m_testExtendPeriod = period;
}

}  // namespace WalkTest
}  // namespace CoreD
}  // namespace HubPlatform
