#pragma once

#include "nm_ip_version.h"
#include "json_utils/json_fwd.h"
#include <string>
#include <vector>
#include <chrono>

namespace HubPlatform {
namespace Network {

class Statistics {
public:
    Statistics() = default;
    virtual ~Statistics() = default;

    void registerSuccess();
    void registerFail();
    size_t succeeded() const;
    size_t failed() const;
    size_t sequentialFailures() const;
    size_t total() const;

private:
    size_t m_succeeded{ 0 };
    size_t m_failed{ 0 };
    size_t m_sequentialFailures{ 0 };
    bool m_lastFailed{ false };
};

class InternetAvailabilityCheck {
public:
    InternetAvailabilityCheck() = default;
    virtual ~InternetAvailabilityCheck() = default;
};

struct HttpMethodOptions {
public:
    bool m_useProvisioningUrls{ false };
    bool m_useExternalUrls{ true };
    std::vector< std::string > m_externalUrls;
    std::chrono::seconds m_transferTimeout{ 10 };
    std::chrono::seconds m_connectTimeout{ 10 };

public:
    bool empty() const;

    bool configure( const JsonObject& config );
};

class TcpInternetAvailabilityCheck : public InternetAvailabilityCheck {
public:
    enum HttpCodes { RequestFailed = 699 };

public:
    virtual bool checkCloudAvailability( const std::string& interface, IPVersion ipVersion, Statistics* statistics );

    bool makeHttpRequest( const std::string& hostName, const std::string& interface );

    void setOptions( const HttpMethodOptions& options );

private:
    HttpMethodOptions m_httpMethodOptions;
};

}  // namespace Network
}  // namespace HubPlatform