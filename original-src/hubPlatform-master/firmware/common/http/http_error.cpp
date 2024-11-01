#include "http/http_error.h"
#include <utility>

namespace HubPlatform {
namespace CommonHttp {

namespace Resources {
const char* const HTTP_ERROR_CONNECTION_LIMIT_REACHED = "Limit of connections is reached";
const char* const HTTP_ERROR_CONNECTION_DOESNT_EXIST = "Connection doesn't exist";
const char* const HTTP_ERROR_PREVIOUS_REQUEST_PENDING = "Sending of previous request data didn't finish";
const char* const HTTP_ERROR_PARAM_NOT_DEFINED = "Required parameter isn't defined";

const char* const HTTP_ERROR_PLUGIN_PERMISSIONS = "Plugin doesn't have permissions to the file";
const char* const HTTP_ERROR_FILE_DOESNT_EXIST = "The file doesn't exist";
}  // namespace Resources

HttpError::HttpError( const std::string& reason ) noexcept
: std::runtime_error( "HttpError: " + reason ) {
    setReason( reason );
}

HttpError::HttpError( const HttpError& e ) noexcept
: std::runtime_error( e )
, m_reason( e.m_reason ) {}

void HttpError::setReason( const std::string& reason ) {
    m_reason = reason;
}

const std::string& HttpError::reason() const {
    return m_reason;
}

HttpConnectionDoesntExistError::HttpConnectionDoesntExistError( const std::string& reason )
: HttpError( Resources::HTTP_ERROR_CONNECTION_DOESNT_EXIST ) {
    setReason( reason );
}

HttpConnectionTransferPending::HttpConnectionTransferPending( const std::string& reason )
: HttpError( Resources::HTTP_ERROR_PREVIOUS_REQUEST_PENDING ) {
    setReason( reason );
}

HttpConnectionError::HttpConnectionError( const std::string& reason )
: HttpError( reason ) {
    setReason( reason );
}

HttpPluginPermissionsError::HttpPluginPermissionsError( const std::string& message, std::string fileName )
: HttpError( Resources::HTTP_ERROR_PLUGIN_PERMISSIONS )
, m_fileName( std::move( fileName ) ) {
    setReason( message );
}

const std::string& HttpPluginPermissionsError::getFileName() const {
    return m_fileName;
}

HttpFileDoesntExist::HttpFileDoesntExist( const std::string& reason, std::string fileName )
: HttpError( Resources::HTTP_ERROR_FILE_DOESNT_EXIST )
, m_fileName( std::move( fileName ) ) {
    setReason( reason );
}

const std::string& HttpFileDoesntExist::getFileName() const {
    return m_fileName;
}

}  // namespace CommonHttp
}  // namespace HubPlatform
