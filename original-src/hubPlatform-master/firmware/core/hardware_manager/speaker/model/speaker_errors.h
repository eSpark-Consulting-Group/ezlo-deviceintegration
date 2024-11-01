#pragma once
#include <stdexcept>

namespace HubPlatform {
namespace HardwareManager {
namespace Speaker {

class SpeakerError : public std::runtime_error {
public:
    explicit SpeakerError( const std::string& reason );
};

namespace ErrorInfo {

enum class Code {
    NoError = 0,

    CannotStartPlaying,
    PlayingFinishedInNotStandardWay,
};

const char* toString( Code );

extern const char* RequiredParameter;
extern const char* NoFile;
extern const char* PluginPermissions;
extern const char* WrongParameters;
extern const char* ParameterLimitReached;
extern const char* ParallelPlayingLimitReached;
extern const char* IdDoesNotExist;
extern const char* CannotStartPlaying;
extern const char* PlayingFinishedInNotStandardWay;
}  // namespace ErrorInfo

class RequiredParameterError : public SpeakerError {
public:
    explicit RequiredParameterError( const std::string& name );
};

class NoFileError : public SpeakerError {
public:
    explicit NoFileError( const std::string& path );
};

class PluginPermissionsError : public SpeakerError {
public:
    PluginPermissionsError( const std::string& type, const std::string& param );
};

class FilePermissionsError : public PluginPermissionsError {
public:
    explicit FilePermissionsError( const std::string& path );
};

class RequestPermissionsError : public PluginPermissionsError {
public:
    explicit RequestPermissionsError( const std::string& commandId );
};

class WrongParameterFormatError : public SpeakerError {
public:
    explicit WrongParameterFormatError( const std::string& name );
};

class ParameterLimitError : public SpeakerError {
public:
    explicit ParameterLimitError( const std::string& name );
};

class PlayingSoundsLimitError : public SpeakerError {
public:
    explicit PlayingSoundsLimitError();
};

class CommandIdError : public SpeakerError {
public:
    explicit CommandIdError( const std::string& id );
};

}  // namespace Speaker
}  // namespace HardwareManager
}  // namespace HubPlatform