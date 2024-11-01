
#include "common_utils/curl_wrapper.h"

#include "log_utils/ezlo_log.h"
#include "log_utils/logsettings.h"
#include "tiny/tiny_consolecolors.h"

#include <iostream>
#include <mutex>
#include <openssl/ssl.h>

namespace {
constexpr char CannotInitializeCurlHandle[] = "curl send error: cannot initialize handle!";
}

bool CurlWrapper::mailgunSend( const std::string& mailGunUri,
                               const std::string& apiKey,
                               const std::string& fields,
                               std::string& reason ) {
    // lock_type lock( mtx() );
    auto curl = newCurlHandle();
    if( !curl ) {
        reason = CannotInitializeCurlHandle;
        return false;
    }

    curl_easy_setopt( curl.get(), CURLOPT_URL, mailGunUri.c_str() );
    curl_easy_setopt( curl.get(), CURLOPT_USERNAME, "api" );
    curl_easy_setopt( curl.get(), CURLOPT_PASSWORD, apiKey.c_str() );
    curl_easy_setopt( curl.get(), CURLOPT_POSTFIELDS, fields.c_str() );
    curl_easy_setopt( curl.get(), CURLOPT_SSL_VERIFYPEER, 0 );  // l_sergiy: added
    return curlSend( curl.get(), reason );
}

bool CurlWrapper::sendToFcm( const std::string& fcmUri,
                             const std::string& message,
                             std::string& result,
                             const std::string& serverKey ) {
    // lock_type lock( mtx() );
    auto curl = newCurlHandle();
    if( !curl ) {
        result = CannotInitializeCurlHandle;
        return false;
    }
    struct curl_slist* headers = nullptr;
    const std::string autorizationKey = "Authorization: key=" + serverKey;
    headers = curl_slist_append( headers, "Content-Type: application/json" );
    headers = curl_slist_append( headers, autorizationKey.c_str() );
    curl_easy_setopt( curl.get(), CURLOPT_URL, fcmUri.c_str() );
    curl_easy_setopt( curl.get(), CURLOPT_POSTFIELDSIZE, message.size() );
    curl_easy_setopt( curl.get(), CURLOPT_POSTFIELDS, message.c_str() );
    curl_easy_setopt( curl.get(), CURLOPT_HTTPHEADER, headers );
    curl_easy_setopt( curl.get(), CURLOPT_WRITEFUNCTION, writeToResultString );
    curl_easy_setopt( curl.get(), CURLOPT_WRITEDATA, &result );
    curl_easy_setopt( curl.get(), CURLOPT_SSL_VERIFYPEER, 0 );  // l_sergiy: added
    bool bOK = curlSend( curl.get(), result );
    curl_slist_free_all( headers );
    return bOK;
}
bool CurlWrapper::sendToNest( const std::string& nestUri,
                              const std::string& message,
                              std::string& result,
                              const std::string& token ) {
    // lock_type lock( mtx() );
    auto curl = newCurlHandle();
    if( !curl ) {
        result = CannotInitializeCurlHandle;
        return false;
    }

    struct curl_slist* headers = nullptr;
    const std::string authorizationToken = "Authorization: Bearer " + token;
    headers = curl_slist_append( headers, "Content-Type: application/json" );
    headers = curl_slist_append( headers, authorizationToken.c_str() );
    curl_easy_setopt( curl.get(), CURLOPT_URL, nestUri.c_str() );
    curl_easy_setopt( curl.get(), CURLOPT_HTTPHEADER, headers );
    curl_easy_setopt( curl.get(), CURLOPT_FOLLOWLOCATION, 1L );  // handle redirect and follow to it
    curl_easy_setopt( curl.get(), CURLOPT_CUSTOMREQUEST, "PUT" );
    curl_easy_setopt( curl.get(), CURLOPT_POSTFIELDSIZE, message.size() );
    curl_easy_setopt( curl.get(), CURLOPT_POSTFIELDS, message.c_str() );
    curl_easy_setopt( curl.get(), CURLOPT_SSL_VERIFYPEER, 0 );  // l_sergiy: added
    bool bOK = curlSend( curl.get(), result );
    curl_slist_free_all( headers );
    return bOK;
}

bool CurlWrapper::sendToAlexa( const std::string& alexaUrl, const std::string& token, std::string& result ) {
    // lock_type lock( mtx() );
    auto curl = newCurlHandle();
    if( !curl ) {
        result = CannotInitializeCurlHandle;
        return false;
    }
    struct curl_slist* headers = nullptr;
    std::string authorizationToken = "Authorization: Bearer " + token;
    headers = curl_slist_append( headers, authorizationToken.c_str() );
    curl_easy_setopt( curl.get(), CURLOPT_URL, alexaUrl.c_str() );
    curl_easy_setopt( curl.get(), CURLOPT_HTTPHEADER, headers );
    curl_easy_setopt( curl.get(), CURLOPT_FOLLOWLOCATION, 1L );  // handle redirect and follow to it
    curl_easy_setopt( curl.get(), CURLOPT_WRITEFUNCTION, writeToResultString );
    curl_easy_setopt( curl.get(), CURLOPT_WRITEDATA, &result );
    curl_easy_setopt( curl.get(), CURLOPT_CONNECTTIMEOUT, 5 );
    curl_easy_setopt( curl.get(), CURLOPT_SSL_VERIFYPEER, 0 );  // l_sergiy: added
    bool bOK = curlSend( curl.get(), result );
    curl_slist_free_all( headers );
    return bOK;
}

bool CurlWrapper::downloadFile( const std::string& url, std::ostream& stream, std::string& result ) {
    auto curl = newCurlHandle();
    if( !curl ) {
        result = "curl send error: cannot initialize handle!";
        return false;
    }

    curl_easy_setopt( curl.get(), CURLOPT_URL, url.c_str() );
    curl_easy_setopt( curl.get(), CURLOPT_FOLLOWLOCATION, 1L );
    curl_easy_setopt( curl.get(), CURLOPT_NOSIGNAL, 1L );  // Prevent "longjmp causes uninitialized stack frame" bug
    curl_easy_setopt( curl.get(), CURLOPT_ACCEPT_ENCODING, "deflate" );
    curl_easy_setopt( curl.get(), CURLOPT_FAILONERROR, 1L );
    curl_easy_setopt( curl.get(), CURLOPT_WRITEFUNCTION, writeDataToStream );
    curl_easy_setopt( curl.get(), CURLOPT_WRITEDATA, &stream );

    return curlSend( curl.get(), result );
}

CurlWrapper::PrepareResult CurlWrapper::prepareCustomRequest( const std::string& url,
                                                              const Headers& headers,
                                                              const std::string& serverCertificateUrl,
                                                              const std::string& clientPublicCertificateUrl,
                                                              const std::string& clientPrivateCertificateUrl,
                                                              std::string& result,
                                                              int connectTimeout,
                                                              int transferTimeout,
                                                              const char* interface ) {
    auto curl = newCurlHandle();
    if( !curl ) {
        result = "curl send error: cannot initialize handle!";
        return PrepareResult{ CurlHandlePtr{ nullptr, &curl_easy_cleanup },
                              CurlHeadersList{ nullptr, &curl_slist_free_all } };
    }

    curl_slist* headersList = nullptr;
    if( !headers.empty() ) {
        for( auto header : headers ) {
            auto newList = curl_slist_append( headersList, header.c_str() );
            if( newList == nullptr ) {
                curl_slist_free_all( headersList );
                result = "Failed to fill http headers";
                return PrepareResult{ CurlHandlePtr{ nullptr, &curl_easy_cleanup },
                                      CurlHeadersList{ nullptr, &curl_slist_free_all } };
            }
            headersList = newList;
        }
        curl_easy_setopt( curl.get(), CURLOPT_HTTPHEADER, headersList );
    }
    curl_easy_setopt( curl.get(), CURLOPT_URL, url.c_str() );
    curl_easy_setopt( curl.get(), CURLOPT_WRITEFUNCTION, writeToResultString );
    curl_easy_setopt( curl.get(), CURLOPT_WRITEDATA, &result );
    curl_easy_setopt( curl.get(), CURLOPT_CONNECTTIMEOUT, connectTimeout );
    curl_easy_setopt( curl.get(), CURLOPT_TIMEOUT, transferTimeout );
    curl_easy_setopt( curl.get(), CURLOPT_SSL_VERIFYPEER, 0 );

    if( !serverCertificateUrl.empty() ) {
        curl_easy_setopt( curl.get(), CURLOPT_CAPATH, serverCertificateUrl.c_str() );
    }

    if( !clientPublicCertificateUrl.empty() && !clientPrivateCertificateUrl.empty() ) {
        curl_easy_setopt( curl.get(), CURLOPT_SSLCERT, clientPublicCertificateUrl.c_str() );
        curl_easy_setopt( curl.get(), CURLOPT_SSLKEY, clientPrivateCertificateUrl.c_str() );
    }

    if( interface && *interface ) {
        std::string interfaceParam = std::string{ "if!" }.append( interface );
        curl_easy_setopt( curl.get(), CURLOPT_INTERFACE, interfaceParam.c_str() );
    }
    return PrepareResult{ std::move( curl ), CurlHeadersList{ headersList, &curl_slist_free_all } };
}

long CurlWrapper::curlCustomPost( const std::string& url,
                                  const Headers& headers,
                                  const std::string& serverCertificateUrl,
                                  const std::string& clientPublicCertificateUrl,
                                  const std::string& clientPrivateCertificateUrl,
                                  const std::string& message,
                                  std::string& result,
                                  int connectTimeout,
                                  int transferTimeout,
                                  const char* interface ) {
    auto prepared = prepareCustomRequest( url,
                                          headers,
                                          serverCertificateUrl,
                                          clientPublicCertificateUrl,
                                          clientPrivateCertificateUrl,
                                          result,
                                          connectTimeout,
                                          transferTimeout,
                                          interface );
    auto& curl = prepared.first;

    if( curl == nullptr )
        return false;

    curl_easy_setopt( curl.get(), CURLOPT_CUSTOMREQUEST, "POST" );
    if( !message.empty() ) {
        curl_easy_setopt( curl.get(), CURLOPT_POSTFIELDSIZE, message.size() );
        curl_easy_setopt( curl.get(), CURLOPT_POSTFIELDS, message.c_str() );
    }

    long statusCode;
    bool bOK = curlSend( curl.get(), result, &statusCode );
    return bOK ? statusCode : 0;
}

/** Performs custom GET http request, blocks execution till the response is received or timeout.
 * @params url - URL to make HTTP request to;
 * @params header - list of headers to pass;
 * @params serverCertificateUrl - server certificate to pass along with request, for TLS requests;
 * @params clientPublicCertificateUrl - client certificate public key;
 * @params clientPrivateCertificateUrl - client private certificate;
 * @params [out] result - HTTP response body;
 * @params connectTimeout - connect timeout, in seconds;
 * @params transferTimeout, - transfer timeout, in seconds;
 * @params interface - if not empty, use the network interface to perform the HTTP request.
 * @returns HTTP status code, but only on case when HTTP request succeeds. Otherwise, returns 0. Use lastStatusCode()
 * instead to get HTTP status code.  */
long CurlWrapper::curlCustomGet( const std::string& url,
                                 const Headers& headers,
                                 const std::string& serverCertificateUrl,
                                 const std::string& clientPublicCertificateUrl,
                                 const std::string& clientPrivateCertificateUrl,
                                 std::string& result,
                                 int connectTimeout,
                                 int transferTimeout,
                                 const char* interface ) {
    auto prepared = prepareCustomRequest( url,
                                          headers,
                                          serverCertificateUrl,
                                          clientPublicCertificateUrl,
                                          clientPrivateCertificateUrl,
                                          result,
                                          connectTimeout,
                                          transferTimeout,
                                          interface );
    auto& curl = prepared.first;

    if( curl == nullptr )
        return false;

    long statusCode;
    CURLcode curlCode = CURLE_OK;
    bool bOK = curlSend( curl.get(), result, &statusCode, &curlCode );

    return bOK ? statusCode : 0;
}

bool CurlWrapper::curlGet( const std::string& url,
                           const std::string& serverCertificateUrl,
                           const std::string& clientPublicCertificateUrl,
                           const std::string& clientPrivateCertificateUrl,
                           std::string& result,
                           int timeout ) {
    auto curl = newCurlHandle();
    if( !curl ) {
        result = "curl send error: cannot initialize handle!";
        return false;
    }

    curl_easy_setopt( curl.get(), CURLOPT_URL, url.c_str() );
    curl_easy_setopt( curl.get(), CURLOPT_WRITEFUNCTION, writeToResultString );
    curl_easy_setopt( curl.get(), CURLOPT_WRITEDATA, &result );
    curl_easy_setopt( curl.get(), CURLOPT_CONNECTTIMEOUT, timeout );
    curl_easy_setopt( curl.get(), CURLOPT_SSL_VERIFYPEER, 0 );
    curl_easy_setopt( curl.get(), CURLOPT_FAILONERROR, true );

    if( !serverCertificateUrl.empty() ) {
        curl_easy_setopt( curl.get(), CURLOPT_CAPATH, serverCertificateUrl.c_str() );
    }

    if( !clientPublicCertificateUrl.empty() && !clientPrivateCertificateUrl.empty() ) {
        curl_easy_setopt( curl.get(), CURLOPT_SSLCERT, clientPublicCertificateUrl.c_str() );
        curl_easy_setopt( curl.get(), CURLOPT_SSLKEY, clientPrivateCertificateUrl.c_str() );
    }

    return curlSend( curl.get(), result );
}

bool CurlWrapper::curlGet( const std::string& url, std::string& result, int timeout ) {
    // lock_type lock( mtx() );
    auto curl = newCurlHandle();
    if( !curl ) {
        result = CannotInitializeCurlHandle;
        return false;
    }

    curl_easy_setopt( curl.get(), CURLOPT_URL, url.c_str() );
    curl_easy_setopt( curl.get(), CURLOPT_WRITEFUNCTION, writeToResultString );
    curl_easy_setopt( curl.get(), CURLOPT_WRITEDATA, &result );
    curl_easy_setopt( curl.get(), CURLOPT_CONNECTTIMEOUT, timeout );
    curl_easy_setopt( curl.get(), CURLOPT_SSL_VERIFYPEER, 0 );  // l_sergiy: added
    return curlSend( curl.get(), result );
}

bool CurlWrapper::curlGet( const std::string& url,
                           std::string& result,
                           curl_slist* headers,
                           int timeout,
                           const std::string& userName,
                           const std::string& userPassword,
                           long* statusCode,
                           CURLcode* curlCode,
                           bool followLocations ) {
    // lock_type lock( mtx() );
    auto curl = newCurlHandle();
    if( !curl ) {
        result = CannotInitializeCurlHandle;
        return false;
    }

    curl_easy_setopt( curl.get(), CURLOPT_HTTPHEADER, headers );
    curl_easy_setopt( curl.get(), CURLOPT_FOLLOWLOCATION, 1L );  // handle redirect and follow to it
    curl_easy_setopt( curl.get(), CURLOPT_URL, url.c_str() );
    curl_easy_setopt( curl.get(), CURLOPT_WRITEFUNCTION, writeToResultString );
    curl_easy_setopt( curl.get(), CURLOPT_WRITEDATA, &result );
    curl_easy_setopt( curl.get(), CURLOPT_CONNECTTIMEOUT, timeout );
    curl_easy_setopt( curl.get(), CURLOPT_SSL_VERIFYPEER, 0 );  // l_sergiy: added

    if( !userName.empty() && !userPassword.empty() ) {
        curl_easy_setopt( curl.get(), CURLOPT_USERPWD, ( userName + ":" + userPassword ).c_str() );
    }

    if( followLocations ) {
        curl_easy_setopt( curl.get(), CURLOPT_FOLLOWLOCATION, 1L );
    }

    return curlSend( curl.get(), result, statusCode, curlCode );
}

bool CurlWrapper::curlPost( const std::string& url, const std::string& fields, std::string& result, int timeout ) {
    // lock_type lock( mtx() );
    auto curl = newCurlHandle();
    if( !curl ) {
        result = CannotInitializeCurlHandle;
        return false;
    }
    curl_easy_setopt( curl.get(), CURLOPT_URL, url.c_str() );
    curl_easy_setopt( curl.get(), CURLOPT_POSTFIELDS, fields.c_str() );
    curl_easy_setopt( curl.get(), CURLOPT_WRITEFUNCTION, writeToResultString );
    curl_easy_setopt( curl.get(), CURLOPT_WRITEDATA, &result );
    curl_easy_setopt( curl.get(), CURLOPT_CONNECTTIMEOUT, timeout );
    curl_easy_setopt( curl.get(), CURLOPT_SSL_VERIFYPEER, 0 );  // l_sergiy: added
    return curlSend( curl.get(), result );
}

bool CurlWrapper::curlPost( const std::string& url,
                            curl_slist* headers,
                            const std::string& fields,
                            std::string& result,
                            int timeout,
                            const std::string& userName,
                            const std::string& userPassword,
                            long* statusCode,
                            CURLcode* curlCode,
                            bool followLocations ) {
    // lock_type lock( mtx() );
    auto curl = newCurlHandle();
    if( !curl ) {
        result = CannotInitializeCurlHandle;
        return false;
    }
    curl_easy_setopt( curl.get(), CURLOPT_HTTPHEADER, headers );
    curl_easy_setopt( curl.get(), CURLOPT_URL, url.c_str() );
    curl_easy_setopt( curl.get(), CURLOPT_POSTFIELDS, fields.c_str() );
    curl_easy_setopt( curl.get(), CURLOPT_WRITEFUNCTION, writeToResultString );
    curl_easy_setopt( curl.get(), CURLOPT_WRITEDATA, &result );
    curl_easy_setopt( curl.get(), CURLOPT_CONNECTTIMEOUT, timeout );
    curl_easy_setopt( curl.get(), CURLOPT_SSL_VERIFYPEER, 0 );  // l_sergiy: added

    if( !userName.empty() && !userPassword.empty() )
        curl_easy_setopt( curl.get(), CURLOPT_USERPWD, ( userName + ":" + userPassword ).c_str() );

    if( followLocations )
        curl_easy_setopt( curl.get(), CURLOPT_FOLLOWLOCATION, 1L );

    return curlSend( curl.get(), result, statusCode, curlCode );
}

bool CurlWrapper::curlPostData( const std::string& url,
                                const std::vector< char >& resource,
                                std::string& result,
                                int timeout ) {
    // lock_type lock( mtx() );
    auto curl = newCurlHandle();
    if( !curl ) {
        result = CannotInitializeCurlHandle;
        return false;
    }
    curl_easy_setopt( curl.get(), CURLOPT_URL, url.c_str() );
    curl_easy_setopt( curl.get(), CURLOPT_POSTFIELDSIZE, resource.size() );
    curl_easy_setopt( curl.get(), CURLOPT_POSTFIELDS, resource.data() );
    curl_easy_setopt( curl.get(), CURLOPT_WRITEFUNCTION, writeToResultString );
    curl_easy_setopt( curl.get(), CURLOPT_WRITEDATA, &result );
    curl_easy_setopt( curl.get(), CURLOPT_CONNECTTIMEOUT, timeout );
    curl_easy_setopt( curl.get(), CURLOPT_SSL_VERIFYPEER, 0 );  // l_sergiy: added
    return curlSend( curl.get(), result );
}

bool CurlWrapper::curlPostDataAsMethod( const std::string& httpMethod,
                                        const std::string& url,
                                        const Headers& headers,
                                        const std::string& resource,
                                        std::string& result,
                                        int timeout ) {
    auto prepared = prepareCustomRequest( url, headers, "", "", "", result, timeout, timeout );
    auto& curl = prepared.first;
    if( !curl ) {
        result = CannotInitializeCurlHandle;
        return false;
    }

    if( !resource.empty() ) {
        curl_easy_setopt( curl.get(), CURLOPT_POSTFIELDSIZE, resource.size() );
        curl_easy_setopt( curl.get(), CURLOPT_POSTFIELDS, resource.data() );
    }

    // curl docs: Using CURLOPT_POSTFIELDS implies setting CURLOPT_POST to 1.
    if( !httpMethod.empty() ) {
        curl_easy_setopt( curl.get(), CURLOPT_CUSTOMREQUEST, httpMethod.c_str() );
    }

    return curlSend( curl.get(), result );
}

bool CurlWrapper::sendLogToServer( const std::string& file ) {
    const std::string command = prepareCommand( file, "ezlo/sendlog" );
    return std::system( command.c_str() ) == 0;
}

size_t CurlWrapper::writeToResultString( void* contents, size_t size, size_t count, void* stream ) {
    ( (std::string*) stream )->append( (char*) contents, size * count );
    return size * count;
}

CurlWrapper::CurlHandlePtr CurlWrapper::newCurlHandle() {
    static std::mutex globalInitLock;

    {
        std::lock_guard< std::mutex > scopeLock( globalInitLock );

        // Curl calls EVP_cleanup in different strange cases. EVP_cleanup removes all SSL digests and curl doesn't work.
        // It's a dirty fix of this case.
        const auto sslDigestAlgorithm = EVP_get_digestbynid( NID_ecdsa_with_SHA1 );
        if( sslDigestAlgorithm == nullptr ) {
            Log::debug( cc::debug( "curl : didn't find SHA1 SSL digest. All SSL digests will be added." ) );
            OpenSSL_add_all_digests();
        }

        curl_global_init( CURL_GLOBAL_DEFAULT );
    }
    lastStatusCode() = 0;

    auto* handle = curl_easy_init();

    /* we use libcurl in a multithread environment, so error handling mechanism using signals should be turned off */
    curl_easy_setopt( handle, CURLOPT_NOSIGNAL, 1L );

    return CurlHandlePtr{ handle, ::curl_easy_cleanup };
}

std::string CurlWrapper::prepareCommand( const std::string& name, const std::string& target ) {
    const std::string path = ezlo::log_settings::getLogDirectory() + name;
    const std::string compress = "gzip -c " + path + " > " + path + ".gz";
    const std::string sendLog = "curl -f --form serial=NMA --form pass=sillyPassword "
                                "--form upload=@" +
                                path + ".gz https://log.ezlo.com/" + target;
    return compress + " && " + sendLog + "&";
}

std::size_t CurlWrapper::writeDataToStream( void* ptr, size_t size, size_t nmemb, void* stream ) {
    std::string data( static_cast< const char* >( ptr ), size * nmemb );
    *( static_cast< std::ostream* >( stream ) ) << data;
    return size * nmemb;
}

bool CurlWrapper::curlSend( CURL* curl, std::string& result, long* statusCode, CURLcode* curlCode ) {
    result.clear();
    if( !curl ) {
        result = CannotInitializeCurlHandle;
        return false;
    }

#ifdef _DEBUG
    curl_easy_setopt( curl, CURLOPT_VERBOSE, 1L );
#endif
    lastStatusReset();

    curl_easy_setopt( curl, CURLOPT_FAILONERROR, true );

    char errorBuffer[ CURL_ERROR_SIZE ] = "";
    curl_easy_setopt( curl, CURLOPT_ERRORBUFFER, errorBuffer );

    const auto requestResult = curl_easy_perform( curl );
    const auto isOK = ( requestResult == CURLE_OK );
    if( !isOK ) {
        result = std::string( "curl send error: " ) +
                 ( *errorBuffer ? errorBuffer : curl_easy_strerror( requestResult ) ) + "(" +
                 std::to_string( requestResult ) + ")";

        if( *errorBuffer ) {
            unsigned bufLen = 0;
            char* errBuf = lastStatusCodeText( &bufLen );
            unsigned toCopy = bufLen < sizeof( errorBuffer ) ? bufLen : sizeof( errorBuffer );
            strncpy( errBuf, errorBuffer, bufLen );
        }
    }

    curl_easy_setopt( curl, CURLOPT_ERRORBUFFER, nullptr );

    // Fetch status code anyway, not only on CURLE_OK!
    long localStatusCode = 0;
    curl_easy_getinfo( curl, CURLINFO_RESPONSE_CODE, &localStatusCode );
    lastStatusCode() = localStatusCode;

    if( isOK && statusCode ) {
        *statusCode = localStatusCode;
    }

    if( curlCode ) {
        *curlCode = requestResult;
    }

    return isOK;
}

int& CurlWrapper::lastStatusCode() {
    static int lastStatus = 0;
    return lastStatus;
}

char* CurlWrapper::lastStatusCodeText( unsigned int* length ) {
    static char buf[ CURL_ERROR_SIZE ] = { 0 };
    if( length )
        *length = sizeof( buf );
    return buf;
}

/** Resets status code and text from previous requests; sets last code to 0, last error text to empty string. */
void CurlWrapper::lastStatusReset() {
    lastStatusCode() = 0;
    if( lastStatusCodeText() )
        lastStatusCodeText()[ 0 ] = '\0';
}
