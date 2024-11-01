#include "hardware_manager/speaker/model/speaker_errors.h"

namespace HubPlatform {
namespace HardwareManager {
namespace Speaker {

/** Lua Errors:
1. Required parameter didn't define: <parameter_name>
2. The file doesn't exist: <file_path>
3. Plugin doesn't have permissions to the file: <file_path>
4. Wrong format of parameter: <parameter_name>
5. Limit of a parameter is reached: <parameter_name>
6. Limit of playing sounds is reached
7. Plugin doesn't have permissions to the request: <id_of_request>
8. Id doesn't exist
*/

namespace ErrorInfo {
const char* RequiredParameter = "Required parameter didn't define";
const char* NoFile = "The file doesn't exist";
const char* PluginPermissions = "Plugin doesn't have permissions to the ";  // to the file | to the request
const char* WrongParameters = "Wrong format of a parameter";
const char* ParameterLimitReached = "Limit of a parameter is reached";
const char* ParallelPlayingLimitReached = "Limit of playing sounds is reached";
const char* IdDoesNotExist = "Id doesn't exist";
const char* CannotStartPlaying = "Internal error. Cannot start playing.";
const char* PlayingFinishedInNotStandardWay = "Playing finished in not standard way";

const char* toString( Code code ) {
    switch( code ) {
    case Code::NoError:
        return "";

    case Code::CannotStartPlaying:
        return CannotStartPlaying;

    case Code::PlayingFinishedInNotStandardWay:
        return PlayingFinishedInNotStandardWay;

    default:
        break;
    };

    return "";
}

}  // namespace ErrorInfo

using namespace ErrorInfo;

SpeakerError::SpeakerError( const std::string& reason )
: std::runtime_error( reason ) {}

RequiredParameterError::RequiredParameterError( const std::string& name )
: SpeakerError( std::string{ ErrorInfo::RequiredParameter } + ": " + name ) {}

NoFileError::NoFileError( const std::string& path )
: SpeakerError( std::string{ NoFile } + ": " + path ) {}

PluginPermissionsError::PluginPermissionsError( const std::string& type, const std::string& param )
: SpeakerError( std::string{ PluginPermissions } + type + ": " + param ) {}

FilePermissionsError::FilePermissionsError( const std::string& path )
: PluginPermissionsError( "file", path ) {}

RequestPermissionsError::RequestPermissionsError( const std::string& commandId )
: PluginPermissionsError( "request", commandId ) {}

WrongParameterFormatError::WrongParameterFormatError( const std::string& name )
: SpeakerError( std::string{ WrongParameters } + ": " + name ) {}

ParameterLimitError::ParameterLimitError( const std::string& name )
: SpeakerError( std::string{ ParameterLimitReached } + ": " + name ) {}

PlayingSoundsLimitError::PlayingSoundsLimitError()
: SpeakerError( std::string{ ParallelPlayingLimitReached } ) {}

CommandIdError::CommandIdError( const std::string& id )
: SpeakerError( std::string{ IdDoesNotExist } ) {}

}  // namespace Speaker
}  // namespace HardwareManager
}  // namespace HubPlatform