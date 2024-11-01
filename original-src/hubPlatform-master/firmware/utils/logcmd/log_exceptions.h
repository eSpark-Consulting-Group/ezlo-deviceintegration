#pragma once

#include <string>
#include <stdexcept>

namespace HubPlatform {
namespace LogCmd {

class LogCmdException : public std::runtime_error {
public:
    using runtime_error::runtime_error;
    explicit LogCmdException( const std::string& msg )
    : runtime_error( msg ){};
};

class FilesystemError final : public LogCmdException {
public:
    explicit FilesystemError( const std::string& msg )
    : LogCmdException( msg ){};
};

class LogFileError final : public LogCmdException {
public:
    explicit LogFileError( const std::string& msg )
    : LogCmdException( msg ){};
};

class LogArgError final : public LogCmdException {
public:
    explicit LogArgError( const std::string& msg )
    : LogCmdException( msg ){};
};

class LogDirectoryError final : public LogCmdException {
public:
    explicit LogDirectoryError( const std::string& msg )
    : LogCmdException( msg ){};
};

class LogDirectoryDoesNotExistError final : public LogCmdException {
public:
    explicit LogDirectoryDoesNotExistError( const std::string& msg )
    : LogCmdException( msg ){};
};

class LogArchiveDirectoryDoesNotExistError final : public LogCmdException {
public:
    explicit LogArchiveDirectoryDoesNotExistError( const std::string& msg )
    : LogCmdException( msg ){};
};

class LogDirectoryFilesNotFoundError final : public LogCmdException {
public:
    explicit LogDirectoryFilesNotFoundError( const std::string& msg )
    : LogCmdException( msg ){};
};

class LogArchiveFailed final : public LogCmdException {
public:
    explicit LogArchiveFailed( const std::string& msg )
    : LogCmdException( msg ){};
};

class LogCreateDirFailed final : public LogCmdException {
public:
    explicit LogCreateDirFailed( const std::string& msg )
    : LogCmdException( msg ){};
};

class LogMoveFailed final : public LogCmdException {
public:
    explicit LogMoveFailed( const std::string& msg )
    : LogCmdException( msg ){};
};

class LogCopyFailed final : public LogCmdException {
public:
    explicit LogCopyFailed( const std::string& msg )
    : LogCmdException( msg ){};
};

class LogUploadError final : public LogCmdException {
public:
    explicit LogUploadError( const std::string& msg )
    : LogCmdException( msg ){};
};

class LogDatabaseError final : public LogCmdException {
public:
    explicit LogDatabaseError( const std::string& msg )
    : LogCmdException( msg ){};
};

class LogUtilsError final : public LogCmdException {
public:
    explicit LogUtilsError( const std::string& msg )
    : LogCmdException( msg ){};
};

}  // namespace LogCmd
}  // namespace HubPlatform
