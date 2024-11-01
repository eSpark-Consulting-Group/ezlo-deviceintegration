
#pragma once

#include <curl/curl.h>
#include <string>
#include <vector>

#include <memory>

class CurlWrapper {
public:
    using Headers = std::vector< std::string >;

    static bool mailgunSend( const std::string& mailGunUri,
                             const std::string& apiKey,
                             const std::string& fields,
                             std::string& reasons );
    static bool sendToFcm( const std::string& fcmUri,
                           const std::string& message,
                           std::string& result,
                           const std::string& serverKey );
    static bool sendToNest( const std::string& nestUri,
                            const std::string& message,
                            std::string& result,
                            const std::string& token );

    static long curlCustomPost( const std::string& url,
                                const Headers& headers,
                                const std::string& serverCertificateUrl,
                                const std::string& clientPublicCertificateUrl,
                                const std::string& clientPrivateCertificateUrl,
                                const std::string& message,
                                std::string& result,
                                int connectTimeout = 10,
                                int transferTimeout = 1200,
                                const char* interface = "" );
    static long curlCustomGet( const std::string& url,
                               const Headers& headers,
                               const std::string& serverCertificateUrl,
                               const std::string& clientPublicCertificateUrl,
                               const std::string& clientPrivateCertificateUrl,
                               std::string& result,
                               int connectTimeout = 10,
                               int transferTimeout = 1200,
                               const char* interface = "" );

    static bool curlGet( const std::string& url,
                         const std::string& serverCertificateUrl,
                         const std::string& clientPublicCertificateUrl,
                         const std::string& clientPrivateCertificateUrl,
                         std::string& result,
                         int timeout = 5 );

    static bool curlGet( const std::string& url, std::string& result, int timeout = 5 );

    static bool curlGet( const std::string& url,
                         std::string& result,
                         curl_slist* headers,
                         int timeout = 5,
                         const std::string& userName = "",
                         const std::string& userPassword = "",
                         long* statusCode = nullptr,
                         CURLcode* curlCode = nullptr,
                         bool followLocations = false );
    static bool curlPost( const std::string& url, const std::string& fields, std::string& result, int timeout = 5 );
    static bool curlPost( const std::string& url,
                          curl_slist* headers,
                          const std::string& fields,
                          std::string& result,
                          int timeout = 5,
                          const std::string& userName = "",
                          const std::string& userPassword = "",
                          long* statusCode = nullptr,
                          CURLcode* curlCode = nullptr,
                          bool followLocations = false );
    static bool curlPostData( const std::string& url,
                              const std::vector< char >& resource,
                              std::string& result,
                              int timeout = 5 );
    static bool curlPostDataAsMethod( const std::string& httpMethod,
                                      const std::string& url,
                                      const Headers& headers,
                                      const std::string& resource,
                                      std::string& result,
                                      int timeout );
    static bool sendLogToServer( const std::string& file );
    static bool sendToAlexa( const std::string& alexaUrl, const std::string& token, std::string& result );

    static bool downloadFile( const std::string& url, std::ostream& stream, std::string& result );
    static int& lastStatusCode();
    static char* lastStatusCodeText( unsigned* length = nullptr );
    static void lastStatusReset();

private:
    using CurlHandlePtr = std::unique_ptr< CURL, void ( * )( CURL* ) >;
    using CurlHeadersDeleter = void ( * )( curl_slist* );
    using CurlHeadersList = std::unique_ptr< curl_slist, CurlHeadersDeleter >;

    static size_t writeToResultString( void* ptr, size_t size, size_t count, void* stream );
    static CurlHandlePtr newCurlHandle();
    static bool curlSend( CURL* ch, std::string& result, long* statusCode = nullptr, CURLcode* curlCode = nullptr );
    static std::string prepareCommand( const std::string& name, const std::string& target );
    static std::size_t writeDataToStream( void* ptr, size_t size, size_t nmemb, void* stream );

    using PrepareResult = std::pair< CurlHandlePtr, CurlHeadersList >;
    static PrepareResult prepareCustomRequest( const std::string& url,
                                               const Headers& headers,
                                               const std::string& serverCertificateUrl,
                                               const std::string& clientPublicCertificateUrl,
                                               const std::string& clientPrivateCertificateUrl,
                                               std::string& result,
                                               int connectTimeout = 10,
                                               int transferTimeout = 600,
                                               const char* interface = "" );

};  /// class CurlWrapper
