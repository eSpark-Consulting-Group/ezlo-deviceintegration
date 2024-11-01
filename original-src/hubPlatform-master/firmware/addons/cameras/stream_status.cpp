#include "cameras/stream_status.h"

#include "base_utils/timeutils.h"

#include <sstream>
#include <algorithm>
#include <cctype>

namespace HubPlatform {
namespace Cameras {

StreamStatus::StreamStatus( std::string pluginId,
                            std::string streamName,
                            std::string description,
                            std::string cameraUrl,
                            Clock::time_point creatingDate )
: status( Status::Stopped )
, pluginId( std::move( pluginId ) )
, streamName( std::move( streamName ) )
, description( std::move( description ) )
, cameraUrl( std::move( cameraUrl ) )
, lastCreatingDate( std::move( creatingDate ) ) {}

std::string StreamStatus::dump( uint8_t indentSize ) const {
    std::string indent( indentSize, ' ' );

    std::string dump;
    auto dumpItem = [&dump, &indent]( const std::string& name, const std::string& value ) {
        dump += indent + name + " : " + value + '\n';
    };

    dumpItem( "Status", toString( status ) );
    dumpItem( "Plugin ID", pluginId );
    dumpItem( "Stream name", streamName );
    dumpItem( "Description", description );
    dumpItem( "Camera URL", cameraUrl );
    dumpItem( "Controller URL", controllerUrl );
    dumpItem( "Last creating date", toString( lastCreatingDate ) );
    dumpItem( "Last removing date", toString( lastRemoveDate ) );
    dumpItem( "Last start date", toString( lastStartDate ) );
    dumpItem( "Last stop date", toString( lastStopDate ) );
    dumpItem( "Last stream creating error", lastStreamCreatingError );
    dumpItem( "Last snapshot creating for UI error", lastSnapshotCreatingForUiError );
    dumpItem( "Last snapshot creating for Cloud error", lastSnapshotCreatingForCloudError );
    dumpItem( "Last preview particle creating error", lastPreviewParticleCreatingError );

    return dump;
}

JsonObject StreamStatus::toJson() const {
    JsonObject json;
    auto statusString = toString( status );
    std::transform( statusString.begin(), statusString.end(), statusString.begin(), []( unsigned char c ) {
        return std::tolower( c );
    } );
    json[ "status" ] = std::move( statusString );

    json[ "pluginId" ] = pluginId;
    json[ "streamName" ] = streamName;
    json[ "cameraUrl" ] = cameraUrl;
    json[ "controllerUrl" ] = controllerUrl;
    return json;
}

std::string StreamStatus::toString( Status status ) {
    switch( status ) {
    case Status::Removed:
        return "Removed";
    case Status::Started:
        return "Started";
    case Status::Stopped:
        return "Stopped";
    }
    return "";
}

std::string StreamStatus::toString( const Clock::time_point& timePoint ) {
    if( timePoint == Clock::time_point() ) {
        return {};
    }
    return TimeUtils::chronoTimePointToString( timePoint );
}

}  // namespace Cameras
}  // namespace HubPlatform
