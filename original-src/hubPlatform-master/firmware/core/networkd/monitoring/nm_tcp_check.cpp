#include "networkd/monitoring/nm_tcp_check.h"
#include "networkd/monitoring/nm_aux_servers.h"
#include "networkd/resources/nm_resources.h"
#include <tiny/tiny_consolecolors.h>
#include "log_utils/ezlo_log.h"
#include "common_utils/curl_wrapper.h"
#include "json_utils/json_utils.h"
#include <chrono>
#include <random>

namespace HubPlatform {
namespace Network {

namespace {

template < typename Iter >
void shuffleCollection( Iter start, Iter finish ) {
    std::random_device randomDevice;
    std::mt19937 generator( randomDevice() );
    std::shuffle( start, finish, generator );
}

std::vector< std::string > makeAuxServerUrls() {
    const auto& auxServers = namedAuxServers();
    std::vector< std::string > urls;

    // fetch nma server domain, and construct https url from it.
    JsonObject server;
    if( json_get_obj( auxServers, Resources::ServerRoles::NmaServer, server ) ) {
        auto nmaServer = json_get_str( server, Resources::AuxServerFields::Server );
        if( !nmaServer.empty() ) {
            std::string url = "https://" + nmaServer + "/v1/request";
            urls.push_back( url );
        }
    }

    // fetch full url of cloud coordinator, with the port specified
    JsonObject coordinator = JSON_EMPTY_OBJ;
    if( json_get( auxServers,
                  { Resources::ServerRoles::CloudCoordinator, Resources::AuxServerFields::FullUrl },
                  coordinator ) ) {
        auto cloudCoordinator = json_get_str( coordinator );
        if( !cloudCoordinator.empty() )
            urls.push_back( cloudCoordinator );
    }

    LOGGER_DEBUG( "Urls: " + cc::num10( urls.size() ) + ", list: " + cc::j( urls ) );

    return urls;
}

}  // namespace

bool TcpInternetAvailabilityCheck::checkCloudAvailability( const std::string& interface,
                                                           IPVersion ipVersion,
                                                           Statistics* statistics ) {
    LOGGER_TRACE( "interface: " + interface );

    bool isOk = false;
    std::vector< std::string > urls;

    if( m_httpMethodOptions.m_useProvisioningUrls ) {
        urls = makeAuxServerUrls();
    }
    if( m_httpMethodOptions.m_useExternalUrls ) {
        auto& collection = m_httpMethodOptions.m_externalUrls;
        urls.insert( urls.end(), begin( collection ), end( collection ) );
        shuffleCollection( urls.begin(), urls.end() );
    }

    // Limit number fo http tests with 2
    const size_t TestsLimit = 2;
    for( int i = 0; i < std::min( TestsLimit, urls.size() ); i++ ) {
        auto& url = urls[ i ];
        LOGGER_DEBUG( cc::debug( "Test HTTP url: " ) + url + cc::str( " on " ) + interface );

        isOk = makeHttpRequest( url, interface );
        if( isOk ) {
            if( statistics )
                statistics->registerSuccess();
            break;
        }
        else {
            if( statistics )
                statistics->registerFail();
        }
    }
    return isOk;
}

bool TcpInternetAvailabilityCheck::makeHttpRequest( const std::string& url, const std::string& interface ) {
    std::string requestContent;
    long httpStatusCode = RequestFailed;

    LOGGER_INFO( cc::info( "Test internet with HTTP request: " ) + url );

    auto connectTimeout = m_httpMethodOptions.m_connectTimeout.count();
    auto transferTimeout = m_httpMethodOptions.m_transferTimeout.count();
    // Make insecure HTTP(s) request on a specific interface
    int statusOrError = CurlWrapper::curlCustomGet(
        url, {}, "", "", "", requestContent, connectTimeout, transferTimeout, interface.c_str() );
    httpStatusCode = CurlWrapper::lastStatusCode();

    bool isRegularResponse = httpStatusCode >= 200 && httpStatusCode < 299;
    bool isWrongRequestResponse = httpStatusCode >= 400 && httpStatusCode < 599;

    if( isRegularResponse || isWrongRequestResponse ) {
        LOGGER_DEBUG_RAW( cc::debug( "Success: http request reached the HTTP server, http status returned: " ) +
                          cc::num10( (int) httpStatusCode ) );
        return true;
    }
    else {
        Log::error( cc::error( "HTTP Check failed: http status=" ) + cc::num10( (int64_t) httpStatusCode ) +
                    ", url: " + url + ", interface: " + interface + ", last status code=" +
                    cc::num10( CurlWrapper::lastStatusCode() ) + ", text: " + CurlWrapper::lastStatusCodeText() );
    }

    return false;
}

void TcpInternetAvailabilityCheck::setOptions( const HttpMethodOptions& options ) {
    m_httpMethodOptions = options;
}

void Statistics::registerSuccess() {
    m_succeeded++;
    m_lastFailed = false;
}

void Statistics::registerFail() {
    bool lastFailed = m_lastFailed;
    if( !lastFailed )
        m_sequentialFailures++;
    m_failed++;
    m_lastFailed = true;
}

size_t Statistics::succeeded() const {
    return m_succeeded;
}

size_t Statistics::failed() const {
    return m_failed;
}

size_t Statistics::sequentialFailures() const {
    return m_sequentialFailures;
}

size_t Statistics::total() const {
    return m_succeeded + m_failed;
}

bool HttpMethodOptions::empty() const {
    return m_externalUrls.empty();
}

bool HttpMethodOptions::configure( const JsonObject& config ) {
    if( !config.is_object() || config.empty() )
        return false;

    m_useProvisioningUrls = json_get_bool( config, "useProvisioningUrls", false );
    m_useExternalUrls = json_get_bool( config, "useExternalUrls", false );

    JsonArray strings;
    json_get_arr( config, "externalUrls", strings );

    try {
        fromJsonStringArray( strings, [this]( const std::string& value ) { m_externalUrls.push_back( value ); } );
    }
    catch( ... ) {
        LOGGER_ERROR( "Failed to fetch network/internetAvailability/httpMethod/externalUrls" );
        return false;
    }

    auto timeout = json_get_int( config, "transferTimeout", -1 );
    if( timeout > -1 ) {
        m_transferTimeout = std::chrono::seconds( timeout );
    }

    timeout = json_get_int( config, "connectionsTimeout", -1 );
    if( timeout > -1 ) {
        m_connectTimeout = std::chrono::seconds( timeout );
    }

    return true;
}

}  // namespace Network
}  // namespace HubPlatform