#include "zigbee/model/enums/command_type.h"
#include "zigbee/resources/zigbee_resources.h"

#include <cassert>

namespace HubPlatform {
namespace Zigbee {

struct CommandTypeData {
    CommandType typeId;
    const char* name;
};

static const CommandTypeData commandTypeData[] = {
    { CommandType::Generated, Resources::Fields::Command::CommandType::Generated },
    { CommandType::Received, Resources::Fields::Command::CommandType::Received }
};

const char* toString( CommandType type ) {
    for( const auto& data : commandTypeData ) {
        if( data.typeId == type ) {
            return data.name;
        }
    }

    assert( !"Unsupported command type" );

    return Resources::Fields::Command::CommandType::Unknown;
}

CommandType commandTypeFromString( const std::string& type ) {
    for( const auto& data : commandTypeData ) {
        if( type == data.name ) {
            return data.typeId;
        }
    }

    assert( !"Unsupported command type name" );

    return CommandType::Unknown;
}

}  // namespace Zigbee
}  // namespace HubPlatform
