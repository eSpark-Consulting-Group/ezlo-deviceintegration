#pragma once
#include "service_interfaces/postponed_task.h"
#include "json_utils/json_fwd.h"
#include <mutex>
#include <string>
#include <set>
#include <chrono>
#include <atomic>
#include <utility>
#include <vector>

namespace HubPlatform {
namespace Services {
class Service;
}

namespace Uid {

class CloudStorage {
public:
    using Url = std::string;
    using Urls = std::vector< std::string >;
    using Token = std::string;
    using Xapp = std::string;
    using Path = std::string;
    using FileName = std::string;
    using FileNames = std::set< FileName >;
    using ExpirationTime = std::chrono::seconds;
    using Response = std::function< void( const Json& response ) >;
    using RequestId = std::string;
    using Responses = std::map< RequestId, Response >;
    using FileKey = JsonObject;
    using Clock = std::chrono::system_clock;

    CloudStorage( Services::Service& service );
    ~CloudStorage();

    void initStorage();
    void sendUrlAndTokenRequest();
    bool handleResponse( const JsonObject& response );
    void setInternetAvailable( bool available );
    void stopTasks();

private:
    void scheduleTokenUpdate( const std::chrono::seconds& timeInterval );
    void scheduleTokenExpiration( const std::chrono::seconds& timeInterval );

    void rereadFromStorage();
    void initStorageTokenRecord( Json& result );
    void updateInitStatus( const ExpirationTime& tokenExpirationDateSec );

    Json fetchTokenRecord();

private:
    const static std::string CurrentApiVersion;
    const static std::chrono::seconds UrlAndTokenRequestPeriod;
    const static std::chrono::seconds ShiftBeforeTokenExpirationDate;
    static constexpr std::chrono::minutes StatusFirstPrintTime{ 1 };
    static constexpr std::chrono::hours StatusPrintPeriod{ 1 };

    struct CurrentStatus {
        using Records = std::vector< std::string >;

        std::string dump( int8_t indentSize = 4 ) const;

        static void setMessage( Records& collection, std::string message );
        static void addMessage( Records& collection, std::string message );

        void addInitStatus( std::string message );
        void setInitStatus( std::string message );
        void addInternetStatus( std::string message );
        void setInternetStatus( std::string message );
        void addRequestStatus( std::string message );
        void setRequestStatus( std::string message );
        void addResponseStatus( std::string message );
        void setResponseStatus( std::string message );

    private:
        Records initStatus;
        Records internetStatus;
        Records requestStatus;
        Records responseStatus;
    };

private:
    std::mutex m_mutex;
    Services::Service& m_service;
    Services::PostponedTaskPtr m_tokenRequestTask;
    Services::PostponedTaskPtr m_tokenUpdateTimer;
    Services::PostponedTaskPtr m_tokenExpirationTimer;
    std::atomic_bool m_isInternetAvailable;
    CurrentStatus m_currentStatus;
    Services::PostponedTaskPtr m_currentStatusPrinter;
};

}  // namespace Uid
}  // namespace HubPlatform
