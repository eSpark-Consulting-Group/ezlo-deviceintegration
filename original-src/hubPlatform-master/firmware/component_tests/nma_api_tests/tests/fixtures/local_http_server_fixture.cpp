#include "local_http_server_fixture.h"

namespace HubPlatform {
namespace Tests {

void LocalHttpServerFixture::SetUpTestCase() {
    using namespace HubPlatform::HttpBridge;
    auto& apiBridge = HubPlatform::HttpBridge::getApiBridge();

    using OfflineAuth = HubPlatform::Uid::HubOfflineCommandsHandler;
    using NmaTestAppApiBridgeHandlers = NmaTestAppApiBridgeHandlers< ComponentTestApplication, OfflineAuth >;
    ApiBridgeHandlersPtr apiBridgeHandlers = ApiBridgeHandlersPtr{ new NmaTestAppApiBridgeHandlers( nma() ) };

    nma().resetApiBridgeHandlers( std::move( apiBridgeHandlers ) );
}

void LocalHttpServerFixture::TearDownTestCase() {
    getPort() = 0;
    getSecurePort() = 0;
    getHostname() = "";
}

void LocalHttpServerFixture::SetUp() {
    HttpBridge::getApiBridge().parallelSessionsLimit() = 100;
    CommonNmaTests::SetUp();

    //    setCredentials( 101130, getUUID(), { SslUtils::sha256( getPassword() ) } );
    //    setCredentials( 1, getUUID(), { SslUtils::sha256( getPassword() ) } );

    int i = 0;
    for( ; i < 300; i++ ) {
        if( authenticateUser( getUUID(), getPassword() ) )
            break;
        std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
    }
    if( i > 0 )
        Log::debug( cc::debug( "Authentication succeeded on try: " ) + cc::num10( i ) );
}

bool LocalHttpServerFixture::authenticateUser( const std::string& user, const std::string& password ) {
    using HubOfflineCommandsHandler = HubPlatform::Uid::HubOfflineCommandsHandler;
    return HubOfflineCommandsHandler::authenticateUser( user, password );
}

void LocalHttpServerFixture::TearDown() {
    CommonNmaTests::TearDown();
}

std::string LocalHttpServerFixture::makeHostName( unsigned port ) {
    return getHostname() + ":" + std::to_string( port > 0 ? port : getPort() );
}

std::string LocalHttpServerFixture::makeUrl( const char* path, const char* hostName, const char* proto ) {
    if( !proto || !*proto )
        proto = "http";
    if( !path || !*path )
        path = "/";
    unsigned port = getPort();
    if( 0 == strcmp( proto, "https" ) )
        port = getSecurePort();
    std::stringstream os;
    os << proto << "://" << ( hostName && *hostName ? hostName : makeHostName( port ) ) << path;
    return os.str();
}

std::string LocalHttpServerFixture::makeInsecureUrl( const char* path, const ::testing::TestInfo* testInfo ) {
    const char* proto = "http";
    if( !path || !*path )
        path = "/";
    unsigned port = getPort();
    std::stringstream os;
    if( strstr( path, "http://" ) != path )
        os << proto << "://" << makeHostName( port );
    os << path;

    if( testInfo ) {
        const bool hasPath = strchr( path, '?' ) != nullptr;
        os << ( hasPath ? '&' : '?' );
        os << "suite=" << testInfo->test_case_name() << "&case=" << testInfo->name();
    }
    return os.str();
}

int& LocalHttpServerFixture::getPort() {
    static int port = 0;
    return port;
}

int& LocalHttpServerFixture::getSecurePort() {
    static int port = 0;
    return port;
}

std::string& LocalHttpServerFixture::getHostname() {
    static std::string hostName;
    return hostName;
}

void LocalHttpServerFixture::setCredentials( int version,
                                             const std::string& controllerUuid,
                                             const std::vector< std::string >& keys ) {
    Json result;
    result[ "version" ] = version;
    result[ "peers" ] = JSON_EMPTY_OBJ;
    if( !controllerUuid.empty() ) {
        result[ "clients" ] = JSON( controllerUuid << keys );
    }
    std::string ms_firmwareCertificate, ms_firmwareKey, ms_firmwareParent, ms_firmwareExpiredParent;
    auto fetchCertificatesFromFiles = [&]() {
        LOGGER_TRACE_RAW( "fetchCertificatesFromFiles" );

        const std::string& firmwareRoot = Environment::getFirmwareRootFolder();
        std::ifstream crtStream( firmwareRoot + "/certs/0.crt" );
        ms_firmwareCertificate = { std::istreambuf_iterator< char >( crtStream ), {} };
        std::ifstream keyStream( firmwareRoot + "/certs/0.key" );
        ms_firmwareKey = { std::istreambuf_iterator< char >( keyStream ), {} };
        std::ifstream parentStream( firmwareRoot + "/certs/root.crt" );
        ms_firmwareParent = { std::istreambuf_iterator< char >( parentStream ), {} };
        std::ifstream expiredParentStream( firmwareRoot + "/certs/root_too_old.crt" );
        ms_firmwareExpiredParent = { std::istreambuf_iterator< char >( expiredParentStream ), {} };
    };
    fetchCertificatesFromFiles();

    result[ "current" ] = JSON_ARRAY( JSON( "certificate" << ms_firmwareCertificate << "private" << ms_firmwareKey
                                                          << "public_parent" << ms_firmwareParent ) );

    auto message = JSON( "method"
                         << "cloud.access_keys_controller_sync" );
    message[ "result" ] = result;

    nma().assignMessageId( message );
    nma().sendtoHub( message, ComponentTestApplication::ConnectionType::NMA );
}

const std::string& LocalHttpServerFixture::getSerial() {
    static std::string serial = "77777777";
    return serial;
}

const std::string& LocalHttpServerFixture::getUUID() {
    static std::string uuid = "77777777-7777-7777-7777-777777777777";
    return uuid;
}

const std::string& LocalHttpServerFixture::getPassword() {
    static std::string password = "password";
    return password;
}

std::string base64( const std::string& str ) {
    return tiny::tools::base64::encode( reinterpret_cast< const unsigned char* >( str.c_str() ), str.size() );
}

std::string LocalHttpServerFixture::basicAuthHeader( const std::string& password ) {
    std::string authHeader = "Authorization: Basic " + base64( getUUID() + ":" + password );
    return authHeader;
}

std::pair< std::string, std::string > LocalHttpServerFixture::basicAuthHeaderPair( const std::string& password ) {
    return { "Authorization", std::string{ "Basic " } + base64( getUUID() + ":" + password ) };
}

LocalHttpServerFixture::HttpResult LocalHttpServerFixture::getUrl( const std::string& url,
                                                                   const LocalHttpServerFixture::Headers& headers,
                                                                   int timeoutSec,
                                                                   std::string& result ) {
    int isOk = CurlWrapper::curlCustomGet( url, headers, "", "", "", result, timeoutSec, timeoutSec );
    int httpStatus = CurlWrapper::lastStatusCode();
    const char* httpText = CurlWrapper::lastStatusCodeText();
    return HttpResult{ httpStatus, httpText ? httpText : "" };
}

LocalHttpServerFixture::HttpResult LocalHttpServerFixture::postUrl( const std::string& url,
                                                                    const Headers& headers,
                                                                    const std::string& payload,
                                                                    int timeoutSec,
                                                                    std::string& result ) {
    int isOk = CurlWrapper::curlCustomPost( url, headers, "", "", "", payload, result, timeoutSec, timeoutSec );
    int httpStatus = CurlWrapper::lastStatusCode();
    const char* httpText = CurlWrapper::lastStatusCodeText();
    return HttpResult{ httpStatus, httpText ? httpText : "" };
}

LocalHttpServerFixture::HttpResult LocalHttpServerFixture::postUrlAsMethod( const std::string& url,
                                                                            const Headers& headers,
                                                                            const std::string& payload,
                                                                            int timeoutSec,
                                                                            std::string& result ) {
    int isOk = CurlWrapper::curlPostDataAsMethod( "GET", url, headers, payload, result, timeoutSec );
    int httpStatus = CurlWrapper::lastStatusCode();
    const char* httpText = CurlWrapper::lastStatusCodeText();
    return HttpResult{ httpStatus, httpText ? httpText : "" };
}

}  // namespace Tests
}  // namespace HubPlatform
