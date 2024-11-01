#pragma once

#include "uid/extensions/http_links/errors.h"

namespace HubPlatform {
namespace HttpLinks {

namespace ErrorInfo {
const char* RequiredParameter = "Required parameter didn't define";
const char* NoFile = "The file doesn't exist";
const char* PluginPermissions = "Plugin doesn't have permissions to the ";  // to the file | to the request
const char* WrongParameters = "Wrong format of a parameter";
const char* ParameterLimitReached = "Limit of a parameter is reached";
const char* IdDoesNotExist = "Id doesn't exist";
}  // namespace ErrorInfo

class RequiredParameterError : public HttpLinksError {
public:
    explicit RequiredParameterError( const std::string& name )
    : HttpLinksError( std::string{ ErrorInfo::RequiredParameter } + ": " + name ) {}
};

class NoFileError : public HttpLinksError {
public:
    explicit NoFileError( const std::string& path )
    : HttpLinksError( std::string{ ErrorInfo::NoFile } + ": " + path ) {}
};

class PluginPermissionsError : public HttpLinksError {
public:
    PluginPermissionsError( const std::string& type, const std::string& param )
    : HttpLinksError( std::string{ ErrorInfo::PluginPermissions } + type + ": " + param ) {}
};

class FilePermissionsError : public PluginPermissionsError {
public:
    explicit FilePermissionsError( const std::string& path )
    : PluginPermissionsError( "file", path ) {}
};

class RequestPermissionsError : public PluginPermissionsError {
public:
    explicit RequestPermissionsError( const std::string& commandId )
    : PluginPermissionsError( "request", commandId ) {}
};

class WrongParameterFormatError : public HttpLinksError {
public:
    explicit WrongParameterFormatError( const std::string& name )
    : HttpLinksError( std::string{ ErrorInfo::WrongParameters } + ": " + name ) {}
};

class ParameterLimitError : public HttpLinksError {
public:
    explicit ParameterLimitError( const std::string& name )
    : HttpLinksError( std::string{ ErrorInfo::ParameterLimitReached } + ": " + name ) {}
};

}  // namespace HttpLinks
}  // namespace HubPlatform