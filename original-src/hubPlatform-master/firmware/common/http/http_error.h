#pragma once
#include <string>
#include <stdexcept>

namespace HubPlatform {
namespace CommonHttp {

namespace Resources {
extern const char* const HTTP_ERROR_CONNECTION_LIMIT_REACHED;
extern const char* const HTTP_ERROR_CONNECTION_DOESNT_EXIST;
extern const char* const HTTP_ERROR_PARAM_NOT_DEFINED;
extern const char* const HTTP_ERROR_PLUGIN_PERMISSIONS;
extern const char* const HTTP_ERROR_FILE_DOESNT_EXIST;
}  // namespace Resources

class HttpError : public std::runtime_error {
public:
    explicit HttpError( const std::string& reason ) noexcept;
    HttpError( const HttpError& e ) noexcept;
    ~HttpError() override = default;

    void setReason( const std::string& reason );
    virtual const std::string& reason() const;

protected:
    std::string m_reason;
};

class HttpConnectionDoesntExistError : public HttpError {
public:
    explicit HttpConnectionDoesntExistError( const std::string& reason );
};

class HttpConnectionTransferPending : public HttpError {
public:
    explicit HttpConnectionTransferPending( const std::string& reason );
};

class HttpConnectionError : public HttpError {
public:
    explicit HttpConnectionError( const std::string& reason );
};

class HttpPluginPermissionsError : public HttpError {
public:
    explicit HttpPluginPermissionsError( const std::string& message, std::string fileName = "" );
    const std::string& getFileName() const;

private:
    std::string m_fileName;
};

class HttpFileDoesntExist : public HttpError {
public:
    explicit HttpFileDoesntExist( const std::string& reason, std::string fileName = "" );
    const std::string& getFileName() const;

private:
    std::string m_fileName;
};

}  // namespace CommonHttp
}  // namespace HubPlatform
