#include "log_config_reader.h"
#include "log_manager.h"
#include "log_reader/stdin_log_reader.h"
#include "log_utils/ezlo_log.h"
#include "log_writer/stdout_log_writer.h"
#include "log_writer/file_log_writer.h"
#include "log_writer/ubus_log_writer.h"
#include "logreader/log_reader/log_reader_interface.h"
#include "ubus_interface/ubus_impl.h"
#include <cstdlib>
#include <exception>
#include <tiny/tiny_consolecolors.h>

using namespace HubPlatform::LogReader;

int main( int argc, char* argv[] ) {
    LogConfigReader logConfigReader( argc, argv );
    if( !logConfigReader.isValid() ) {
        return EXIT_FAILURE;
    }

    std::vector< LogWriterInterfacePtr > logWriters;

#if !defined( __HUB_TYPE_G150__ ) && !defined( __HUB_TYPE_VHUB__ )
    if( logConfigReader.getUbusLogState() ) {
        logWriters.push_back( std::make_shared< UbusLogWriter >( std::make_unique< HubPlatform::Ubus::UBusImpl >(),
                                                                 logConfigReader.getServiceName(),
                                                                 logConfigReader.getUbusServerName(),
                                                                 logConfigReader.getUbusCallbackName() ) );
    }
#endif
    if( logConfigReader.getFileLogState() ) {
        logWriters.push_back( std::make_shared< FileLogWriter >( logConfigReader.getLogFilePath() ) );
    }
    if( logConfigReader.getStdOutLogState() ) {
        logWriters.push_back( std::make_shared< StdOutLogWriter >() );
    }

    LogManager logger( logWriters );
    CallbackFunc func = [&logger]( const std::string& log ) -> void { logger.produceLog( log ); };
    StdInLogReader reader;
    try {
        reader.run( func );
    }
    catch( const std::exception& ex ) {
        LOGGER_ERROR_RAW( cc::error( "An error caught " ) + cc::note( ex.what() ) );
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
