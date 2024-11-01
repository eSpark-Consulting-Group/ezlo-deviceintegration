#include "cloud_storage/controller_token.h"
#include "uid/cloud_storage/cloud_storage.h"
#include "cloud_storage/cloud_storage_utils.h"
#include "service_interfaces/service.h"
#include "log_utils/ezlo_log.h"
#include "base_utils/timeutils.h"
#include <iosfwd>
#include "jsonrpc/jsonrpc.h"
#include "tiny/tiny_consolecolors.h"
#include "json_utils/json_utils.h"

namespace HubPlatform {
namespace Uid {

const std::string CloudStorage::CurrentApiVersion = "1";
const std::chrono::seconds CloudStorage::UrlAndTokenRequestPeriod = std::chrono::seconds( 90 );
const std::chrono::seconds CloudStorage::ShiftBeforeTokenExpirationDate = std::chrono::minutes( 60 );
constexpr std::chrono::minutes CloudStorage::StatusFirstPrintTime;
constexpr std::chrono::hours CloudStorage::StatusPrintPeriod;

CloudStorage::CloudStorage( Services::Service& service )
: m_service( service )
, m_tokenUpdateTimer(
      m_service.createPostponedTask( [this]( Services::PostponedTask& ) { sendUrlAndTokenRequest(); } ) )
, m_tokenExpirationTimer( m_service.createPostponedTask( [this]( Services::PostponedTask& ) {
    m_service.sendBroadcast( HUB_STORAGE_CONTROLLER_TOKEN_EXPIRED, JSON_EMPTY_OBJ, GroupsNames::BROADCAST_GROUP );
} ) )
, m_isInternetAvailable( false )
, m_currentStatusPrinter( m_service.createPostponedTask( [this]( Services::PostponedTask& ) {
    LOGGER_INFO_RAW( cc::info( "Cloud storage token status: \n" ) + cc::note( m_currentStatus.dump() ) );
} ) ) {
    m_currentStatus.setInternetStatus( "Unavailable" );
}

void CloudStorage::sendUrlAndTokenRequest() {
    LOGGER_DEBUG( "Send request for token" );
    m_currentStatus.setRequestStatus( "Request created at " + TimeUtils::chronoTimePointToString( Clock::now() ) );
    if( m_tokenRequestTask && m_tokenRequestTask->isActive() ) {
        m_tokenRequestTask->stop();
        m_currentStatus.addRequestStatus( "Previous request is stopped" );
    }
    m_tokenRequestTask = m_service.createPostponedTask( [this]( Services::PostponedTask& ) {
        JsonObject storageTokenRequest =
            Common::CloudStorageInterface::storageControllerTokenRequest( CurrentApiVersion.c_str() );
        m_service.sendAsyncRequest(
            GroupsNames::UI_GROUP,
            storageTokenRequest,
            CLOUD_MESSAGE,
            [this]( const JsonObject& message ) {
                if( handleResponse( message ) ) {
                    LOGGER_DEBUG_RAW( "Cloud token is valid" );
                    m_tokenRequestTask.reset();
                    m_currentStatus.setRequestStatus( "Token is received successfully at " +
                                                      TimeUtils::chronoTimePointToString( Clock::now() ) );
                }
                else {
                    LOGGER_DEBUG_RAW( "Cloud token is not valid" );
                    m_tokenRequestTask->start( UrlAndTokenRequestPeriod, false );
                    m_currentStatus.setRequestStatus( "Incorrect token is received at " +
                                                      TimeUtils::chronoTimePointToString( Clock::now() ) );
                }
            },
            [this]() {
                m_tokenRequestTask->start( std::chrono::seconds( 0 ), false );
                m_currentStatus.setRequestStatus( "Request timeout at " +
                                                  TimeUtils::chronoTimePointToString( Clock::now() ) );
            },
            UrlAndTokenRequestPeriod );
    } );
    if( m_isInternetAvailable ) {
        m_currentStatus.addRequestStatus( "Request is sent" );
        m_tokenRequestTask->start( std::chrono::seconds( 0 ), false );
    }
}

void CloudStorage::updateInitStatus( const ExpirationTime& tokenExpirationDateSec ) {
    auto now = Clock::now();
    auto updateTokenTime = tokenExpirationDateSec -
                           std::chrono::duration_cast< std::chrono::seconds >( now.time_since_epoch() ) -
                           ShiftBeforeTokenExpirationDate;

    m_currentStatus.setInitStatus( "The token is loaded from DB at " + TimeUtils::chronoTimePointToString( now ) );
    m_currentStatus.addInitStatus( "Expiration: " +
                                   TimeUtils::chronoTimePointToString( Clock::time_point( tokenExpirationDateSec ) ) );
}

Json CloudStorage::fetchTokenRecord() {
    using namespace Common::CloudStorageInterface;
    ControllerToken controllerToken;
    if( ControllerToken::fetchDbRecord( m_service.getDBClient(), controllerToken ) ) {
        controllerToken.getRawData();
    }
    return {};
}

bool CloudStorage::handleResponse( const JsonObject& result ) {
    auto now = Clock::now();
    m_currentStatus.setResponseStatus( "Response is received at " + TimeUtils::chronoTimePointToString( now ) );
    LOGGER_DEBUG_RAW( "Handle received token" );

    if( hasError( result ) ) {
        LOGGER_DEBUG_RAW( "Token request error" );
        m_currentStatus.addResponseStatus( "Error: " + result.dump() );
        return false;
    }

    Common::CloudStorageInterface::ControllerToken controllerToken;
    controllerToken.fromApiRecord( result );

    auto tokenExpirationDateSec = controllerToken.getExpirationTime();
    auto expirationTokenTime =
        tokenExpirationDateSec - std::chrono::duration_cast< std::chrono::seconds >( now.time_since_epoch() );
    auto updateTokenTime = expirationTokenTime - ShiftBeforeTokenExpirationDate;

    if( updateTokenTime.count() <= 0 ) {
        LOGGER_ERROR_RAW( cc::error( "Received token is expired: " ) + cc::num10( tokenExpirationDateSec.count() ) );
        m_currentStatus.addResponseStatus(
            "Token is expired: " + TimeUtils::chronoTimePointToString( Clock::time_point( tokenExpirationDateSec ) ) );
        return false;
    }

    Common::CloudStorageInterface::ControllerToken::updateDbRecord( m_service.getDBClient(), controllerToken );

    auto tokenRecord = fetchTokenRecord();
    updateInitStatus( tokenExpirationDateSec );

    m_service.sendBroadcast( HUB_STORAGE_CONTROLLER_TOKEN_UPDATED, JSON_EMPTY_OBJ, GroupsNames::BROADCAST_GROUP );

    scheduleTokenExpiration( expirationTokenTime );
    scheduleTokenUpdate( updateTokenTime );

    m_currentStatus.addResponseStatus(
        "Token is good! Next update in " +
        TimeUtils::chronoTimePointToString( Clock::time_point( now + updateTokenTime ) ) );

    return true;
}

void CloudStorage::setInternetAvailable( bool available ) {
    m_isInternetAvailable = available;
    if( m_tokenRequestTask ) {
        if( available ) {
            m_currentStatus.setRequestStatus( "Request is sent at " +
                                              TimeUtils::chronoTimePointToString( Clock::now() ) );
            m_tokenRequestTask->start( std::chrono::seconds( 0 ), false );
        }
        else {
            m_currentStatus.setRequestStatus( "Request is stopped at " +
                                              TimeUtils::chronoTimePointToString( Clock::now() ) );
            m_tokenRequestTask->stop();
        }
    }
    if( available ) {
        m_currentStatus.setInternetStatus( "Available" );
    }
    else {
        m_currentStatus.setInternetStatus( "Unavailable" );
    }
}

void CloudStorage::scheduleTokenUpdate( const std::chrono::seconds& timeInterval ) {
    LOGGER_DEBUG( "Start token update timer: " + std::to_string( timeInterval.count() ) + " sec" );
    if( m_tokenUpdateTimer->isActive() ) {
        m_tokenUpdateTimer->stop();
    }
    m_tokenUpdateTimer->start( timeInterval, false );
}

void CloudStorage::scheduleTokenExpiration( const std::chrono::seconds& timeInterval ) {
    LOGGER_DEBUG( "Start token expiration timer: " + std::to_string( timeInterval.count() ) + " sec" );
    if( m_tokenExpirationTimer->isActive() ) {
        m_tokenExpirationTimer->stop();
    }
    m_tokenExpirationTimer->start( timeInterval, false );
}

void CloudStorage::initStorage() {
    m_currentStatusPrinter->start( StatusFirstPrintTime, StatusPrintPeriod );
    m_service.addTask( [this]() { rereadFromStorage(); } );
}

void CloudStorage::rereadFromStorage() {
    using namespace Common::CloudStorageInterface;
    Json result;
    ControllerToken controllerToken;
    if( ControllerToken::fetchDbRecord( m_service.getDBClient(), controllerToken ) ) {
        result = controllerToken.getRawData();
    }
    if( result.empty() ) {
        LOGGER_DEBUG_RAW( "No token found" );
        initStorageTokenRecord( result );
    }
    else {
        LOGGER_DEBUG_RAW( cc::debug( "Cloud storage token data: " ) + cc::j( result ) );
        time_t expirationTimePoint = json_get_int( result, "tokenExpirationDate", 0 );
        if( expirationTimePoint > 0 ) {
            LOGGER_DEBUG_RAW( cc::debug( "Cloud storage token data.tokenExpirationDate: " ) +
                              TimeUtils::timestampToIsoDate( expirationTimePoint ) );
        }

        auto now = Clock::now();
        m_currentStatus.setInitStatus( "The token is loaded from DB at " + TimeUtils::chronoTimePointToString( now ) );
        std::unique_lock< std::mutex > lock( m_mutex );
        auto tokenExpirationDateSec = controllerToken.getExpirationTime();
        auto expirationTokenTime =
            tokenExpirationDateSec - std::chrono::duration_cast< std::chrono::seconds >( now.time_since_epoch() );
        auto updateTokenTime = expirationTokenTime - ShiftBeforeTokenExpirationDate;
        m_currentStatus.addInitStatus(
            "Expiration: " + TimeUtils::chronoTimePointToString( Clock::time_point( tokenExpirationDateSec ) ) );

        scheduleTokenExpiration( expirationTokenTime );

        if( updateTokenTime.count() <= 0 ) {
            lock.unlock();
            sendUrlAndTokenRequest();
            m_currentStatus.addInitStatus( "New token requested" );
            return;
        }
        else {
            scheduleTokenUpdate( updateTokenTime );
            m_currentStatus.addInitStatus(
                "Update token time: " +
                TimeUtils::chronoTimePointToString( Clock::time_point( now.time_since_epoch() + updateTokenTime ) ) );
        }
    }
}

void CloudStorage::initStorageTokenRecord( Json& result ) {
    Common::CloudStorageInterface::ControllerToken::initDbRecord( m_service.getDBClient() );

    result = fetchTokenRecord();
    sendUrlAndTokenRequest();
    m_currentStatus.setInitStatus( "New token requested at " + TimeUtils::chronoTimePointToString( Clock::now() ) );
}

void CloudStorage::stopTasks() {
    if( m_tokenRequestTask )
        m_tokenRequestTask->stop();
    if( m_tokenUpdateTimer )
        m_tokenUpdateTimer->stop();
    if( m_tokenExpirationTimer )
        m_tokenExpirationTimer->stop();
    if( m_currentStatusPrinter )
        m_currentStatusPrinter->stop();
}

CloudStorage::~CloudStorage() {
    stopTasks();
}

std::string CloudStorage::CurrentStatus::dump( int8_t indentSize ) const {
    std::string indent( indentSize, ' ' );

    std::stringstream os;
    auto dumpItem = [&os, &indent]( const std::string& name, const Records& records ) {
        os << indent << name << " : " << std::endl;
        for( const auto& record : records ) {
            os << indent << indent << record << std::endl;
        }
    };
    dumpItem( "Initialization", initStatus );
    dumpItem( "Internet", internetStatus );
    dumpItem( "Token request", requestStatus );
    dumpItem( "Token response", responseStatus );

    return os.str();
}

void CloudStorage::CurrentStatus::setMessage( CloudStorage::CurrentStatus::Records& collection, std::string message ) {
    collection.clear();
    addMessage( collection, std::move( message ) );
}

void CloudStorage::CurrentStatus::addMessage( CloudStorage::CurrentStatus::Records& collection, std::string message ) {
    collection.emplace_back( std::move( message ) );
}

void CloudStorage::CurrentStatus::addInitStatus( std::string message ) {
    addMessage( initStatus, std::move( message ) );
}

void CloudStorage::CurrentStatus::setInitStatus( std::string message ) {
    setMessage( initStatus, std::move( message ) );
}

void CloudStorage::CurrentStatus::addInternetStatus( std::string message ) {
    addMessage( internetStatus, std::move( message ) );
}

void CloudStorage::CurrentStatus::setInternetStatus( std::string message ) {
    setMessage( internetStatus, std::move( message ) );
}

void CloudStorage::CurrentStatus::addRequestStatus( std::string message ) {
    addMessage( requestStatus, std::move( message ) );
}

void CloudStorage::CurrentStatus::setRequestStatus( std::string message ) {
    setMessage( requestStatus, std::move( message ) );
}

void CloudStorage::CurrentStatus::addResponseStatus( std::string message ) {
    addMessage( responseStatus, std::move( message ) );
}

void CloudStorage::CurrentStatus::setResponseStatus( std::string message ) {
    setMessage( responseStatus, std::move( message ) );
}

}  // namespace Uid
}  // namespace HubPlatform
