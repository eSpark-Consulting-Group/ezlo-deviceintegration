#include "system_utils/crontab/crontab_utils.h"

#include <sstream>
#include "common_utils/cpp14.h"

namespace HubPlatform {
namespace system_utils {
namespace crontab_utils {

std::unique_ptr< Crontab::Record > parseLine( const std::string& line ) {
    if( line.empty() )
        return {};

    auto firstChar = line.find_first_not_of( " \t\n" );
    if( firstChar == std::string::npos )
        return {};

    if( line[ firstChar ] == '#' )
        return {};

    auto record = std::make_unique< Crontab::Record >();
    Crontab::Schedule& schedule = record->first;
    Crontab::Command& command = record->second;

    std::istringstream stream( line );
    for( auto& field : schedule ) {
        if( !( stream >> field ) || field.empty() )
            throw CrontabInvalidFileError( "Invalid crontab file: incomplete schedule" );
    }

    std::getline( stream, command );

    auto commandPos = command.find_first_not_of( " \t\n" );
    if( commandPos == std::string::npos )
        throw CrontabInvalidFileError( "Invalid crontab file: empty command" );

    if( commandPos > 0 )
        command = command.substr( commandPos );

    if( command.empty() )
        throw CrontabInvalidFileError( "Invalid crontab file: empty command" );

    return record;
}

}  // namespace crontab_utils
}  // namespace system_utils
}  // namespace HubPlatform
