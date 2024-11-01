
#include "uid/firmware.h"
#include "uid/broadcast.h"
#include "uid/ui_service.h"

#include "log_utils/logsettings.h"

#include "base_utils/utils.h"

#include "hardware_manager/remote_interface/hm_remote_interface.h"

#include <fstream>
#include <mutex>
#include <stdlib.h>

namespace firmware {
typedef utils::MailBoxOne< std::string > MailBoxOne;

std::chrono::high_resolution_clock::time_point startTime;
MailBoxOne mailBoxOne;

std::string downloadPath() {
    return "/opt/firmware/fw/";
}

bool createEmptyFile( const std::string& file_name ) {
    std::ofstream file( file_name );
    return file.is_open();
}

std::size_t writeFilePiece( const std::string& file_name, std::size_t position, const std::string& data ) {
    std::ofstream file( file_name, std::ofstream::binary | std::ofstream::out | std::ofstream::in );
    file.seekp( position );
    file.write( data.data(), data.size() );
    return file.tellp();
}

void setFileDownloadProgress( std::size_t position, std::size_t fileSize ) {
    std::ostringstream ss;
    std::size_t percentage = std::size_t( float( position ) / float( fileSize ) * 100.0f );
    std::chrono::nanoseconds pieceTransmitDuration = std::chrono::high_resolution_clock::now() - startTime;
    ss.width( std::to_string( fileSize / 1024 ).size() + 1 );
    ss << position / 1024;
    ss << "K .......... .......... .......... .......... .......... ";
    ss.width( 3 );
    ss << std::to_string( percentage ) << "% ";
    ss << cc::duration2string( pieceTransmitDuration );
    mailBoxOne.send( ss.str() );
}

void setFileDownloadFinished() {
    mailBoxOne.send( "Finished" );
}

bool updateFile( const std::string& fileName, std::ofstream& log ) {
    if( !HubPlatform::Uid::Service::getInstance().getNmaClient() )
        return false;
    if( !createEmptyFile( downloadPath() + fileName ) )
        return false;
    startTime = std::chrono::high_resolution_clock::now();
    JsonObject message;
    message[ "id" ] = "ezlo_internal_broadcast";
    message[ "msg_subclass" ] = "InitiateFileDownload";
    message[ "result" ] = JSON( "name" << fileName << "position" << std::to_string( 0 ) );
    HubPlatform::Uid::Service::getInstance().getNmaClient()->postMessage( json_write( message ) );
    while( MailBoxOne::MessageOpt msg = mailBoxOne.wait( std::chrono::seconds( 10 ) ) ) {
        log << *msg << std::endl << std::flush;
        if( *msg == "Finished" ) {
            return true;
        }
    }
    return false;
}

}  // namespace firmware
