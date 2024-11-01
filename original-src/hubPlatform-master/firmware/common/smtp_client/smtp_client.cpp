#include "smtp_client.h"

#include "log_utils/ezlo_log.h"
#include "tiny/tiny_consolecolors.h"
#include "system_utils/filesystem.h"

#include <curl/curl.h>

#include <cstring>
#include <sstream>
#include <ctime>
#include <iomanip>

namespace HubPlatform {

namespace {

struct PayloadContext {
    std::string::const_iterator beg;
    std::string::const_iterator end;
};

size_t payloadReader( char* ptr, size_t size, size_t nmemb, void* userp ) {
    PayloadContext& ctx = *(PayloadContext*) userp;

    size_t curlBufferSize = size * nmemb;

    if( size == 0 || nmemb == 0 ) {
        return 0;
    }

    size_t len = std::distance( ctx.beg, ctx.end );
    if( !len )
        return 0;

    const char* data = &( *ctx.beg );

    if( data ) {
        if( curlBufferSize < len )
            len = curlBufferSize;
        std::memcpy( ptr, data, len );
        std::advance( ctx.beg, len );
        return len;
    }

    return 0;
}

// NOTE: Date must comply with RFC 5322
// Example: Fri, 09 Aug 2024 20:21:21 +0300
std::string getRFC5322Date( std::time_t date ) {
    std::tm* tm = std::localtime( &date );

    char dateBuffer[ 100 ];
    std::strftime( dateBuffer, sizeof( dateBuffer ), "%a, %d %b %Y %H:%M:%S", tm );

    char tzBuffer[ 6 ];  // for "+HHMM" or "-HHMM"
    std::strftime( tzBuffer, sizeof( tzBuffer ), "%z", tm );

    std::ostringstream oss;
    oss << dateBuffer << " " << tzBuffer;

    return oss.str();
}

}  // namespace

SMTPClient::SMTPClient( Settings settings )
: m_settings( std::move( settings ) ) {
    if( m_settings.smtpServerUrl.empty() )
        throw SMTPClientSettingsError( "SMTP Client got empty SMTP server URL" );

    if( !m_settings.serverCertificateAuthorityFile.empty() ) {
        if( m_settings.encryption == false )
            throw SMTPClientSettingsError(
                "SMTP Client encryption is disabled while server authority file is provided" );

        if( !filesystem::isFileExist( m_settings.serverCertificateAuthorityFile ) )
            throw SMTPClientSettingsError( "SMTP Client is unable to locate server authority file: " +
                                           m_settings.serverCertificateAuthorityFile );
    }

    if( !m_settings.clientCertificateFile.empty() ) {
        if( m_settings.encryption == false )
            throw SMTPClientSettingsError(
                "SMTP Client encryption is disabled while client certificate file is provided" );

        if( !filesystem::isFileExist( m_settings.clientCertificateFile ) )
            throw SMTPClientSettingsError( "SMTP Client is unable to locate client certificate file: " +
                                           m_settings.clientCertificateFile );
    }

    if( !m_settings.clientCertificateKeyFile.empty() ) {
        if( m_settings.encryption == false )
            throw SMTPClientSettingsError( "SMTP Client encryption is disabled while client key file is provided" );

        if( !filesystem::isFileExist( m_settings.clientCertificateKeyFile ) )
            throw SMTPClientSettingsError( "SMTP Client is unable to locate client key file: " +
                                           m_settings.clientCertificateKeyFile );
    }

    if( m_settings.encryption ) {
        auto pos = m_settings.smtpServerUrl.find( "smtps" );
        if( pos != 0 ) {
            throw SMTPClientSettingsError(
                "SMTP Client encryption is enabled but server URL does not start with 'smtps' prefix" );
        }
    }
}

bool SMTPClient::send( const std::string& sender,
                       const std::string& recipient,
                       const std::string& topic,
                       const std::string& body,
                       const std::time_t date ) {
    std::string payload;

    payload += "Date: " + getRFC5322Date( date ) + "\r\n";
    payload += "To: " + recipient + "\r\n";
    payload += "From: " + sender + "\r\n";
    payload += "Subject: " + topic + "\r\n";
    payload += "\r\n";  // Header and body must be divided by empty line: RFC 5322
    payload += body + "\r\n";

    PayloadContext payloadContext;
    payloadContext.beg = payload.cbegin();
    payloadContext.end = payload.cend();

    struct upload_status {
        size_t bytes_read;
    };

    CURL* curl;
    CURLcode res = CURLE_OK;
    struct curl_slist* recipients = NULL;

    curl = curl_easy_init();
    if( !curl ) {
        LOGGER_ERROR( cc::error( "Unable to send SMTP message: failed to init curl" ) );
        return false;
    }

    curl_easy_setopt( curl, CURLOPT_URL, m_settings.smtpServerUrl.c_str() );

    if( !m_settings.clientUsername.empty() && !m_settings.clientPassword.empty() ) {
        curl_easy_setopt( curl, CURLOPT_USERNAME, m_settings.clientUsername.c_str() );
        curl_easy_setopt( curl, CURLOPT_PASSWORD, m_settings.clientPassword.c_str() );
        // NOTE: potentially it is possible to enable CRAM-MD5 auth mechanism
        curl_easy_setopt( curl, CURLOPT_LOGIN_OPTIONS, "AUTH=PLAIN" );
    }

    if( m_settings.encryption ) {
        curl_easy_setopt( curl, CURLOPT_USE_SSL, CURLUSESSL_ALL );

        // Ensure that server certificate is signed by this CA certificate
        if( !m_settings.serverCertificateAuthorityFile.empty() ) {
            curl_easy_setopt( curl, CURLOPT_CAINFO, m_settings.serverCertificateAuthorityFile.c_str() );
        }

        // Server certificate is signed by ca-certificate known to client
        curl_easy_setopt( curl, CURLOPT_SSL_VERIFYPEER, m_settings.verifyServerCertificateOrigin );

        // Verify server hostname in the certificate CN or SAN fields
        curl_easy_setopt( curl, CURLOPT_SSL_VERIFYHOST, m_settings.verifyServerHostname );

        // Allow server to verify client certificate
        if( !m_settings.clientCertificateFile.empty() && !m_settings.clientCertificateKeyFile.empty() ) {
            curl_easy_setopt( curl, CURLOPT_SSLCERT, m_settings.clientCertificateFile.c_str() );
            curl_easy_setopt( curl, CURLOPT_SSLKEY, m_settings.clientCertificateKeyFile.c_str() );
        }
    }

    curl_easy_setopt( curl, CURLOPT_MAIL_FROM, sender.c_str() );

    recipients = curl_slist_append( recipients, recipient.c_str() );
    curl_easy_setopt( curl, CURLOPT_MAIL_RCPT, recipients );

    curl_easy_setopt( curl, CURLOPT_READFUNCTION, payloadReader );
    curl_easy_setopt( curl, CURLOPT_READDATA, &payloadContext );
    curl_easy_setopt( curl, CURLOPT_UPLOAD, 1L );

    if( m_settings.verbose ) {
        curl_easy_setopt( curl, CURLOPT_VERBOSE, 1L );
    }

    curl_easy_setopt( curl, CURLOPT_TIMEOUT_MS, m_settings.timeout.count() );

    std::vector< char > resultError( CURL_ERROR_SIZE, '\0' );
    curl_easy_setopt( curl, CURLOPT_ERRORBUFFER, resultError.data() );

    // Send message
    res = curl_easy_perform( curl );

    if( res != CURLE_OK ) {
        LOGGER_ERROR( cc::error( "Unable to send SMTP message: curl error: " ) +
                      cc::note( curl_easy_strerror( res ) + std::string( " : " ) + resultError.data() ) );
    }

    curl_slist_free_all( recipients );
    curl_easy_cleanup( curl );

    return res == CURLE_OK;
}

}  // namespace HubPlatform
