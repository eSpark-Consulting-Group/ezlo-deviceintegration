#include "certification_utils/common.h"
#include "certification_utils/man_page.h"

#include "system_utils/environment.h"
#include "base_utils/signal_handler.h"
#include "base_utils/utils.h"
#include "base_utils/zwave/zwave_stick.h"
#include "base_utils/zwave/zwave_serial_api.h"

#include <string>
#include <vector>
#include <iostream>
#include <stdio.h>
#include <csignal>

namespace HubPlatform {
namespace CertificationUtils {
namespace {

#define PROG_NAME "zwave_test"
static const std::string DefaultZwaveUartPortName = "/dev/ttyS1";
static const std::string ZwaveFlasherCmd = Environment::getBinariesFolder().append( "/fb-zwflash -t" );

void help() {
    ManPage page;

    page.section( "NAME" );
    page.subSection( PROG_NAME );
    page.newLine();

    page.section( "SYNOPSIS" );
    page.subSection( PROG_NAME );
    page.subSection( ManPage::join( { PROG_NAME, "{-h | --help}" } ) );
    page.newLine();

    page.section( "DESCRIPTION" );
    page.subSection( "Utility to test Z-Wave module" );

    std::cout << page.content().str() << std::endl << std::flush;
}

void parseArgs( int argc, char* argv[] ) {
    if( argc == 1 )
        return;

    std::vector< std::string > args;
    for( size_t i = 1; i < argc; ++i )
        args.push_back( argv[ i ] );

    if( args.size() == 1 && ( args[ 0 ] == "-h" || args[ 0 ] == "--help" ) ) {
        help();
        exit( 0 );
    }

    help();
    exit( -1 );
}

void startTxStream() {
    SignalHandler sigHandler;
    sigHandler.setupSignalHandlers();
    const volatile sig_atomic_t& exitSignal = sigHandler.gotExitSignalRef();

    std::cout << "Starting Z-Wave test ..." << std::endl;
    system( ZwaveFlasherCmd.c_str() );

    ZwaveSerialApi serialApi{ DefaultZwaveUartPortName, true, exitSignal };

    if( false == serialApi.waitSerialApiStarted() )
        std::cout << "FAILED to start serial API" << std::endl;
    std::cout << "Serial API started" << std::endl;

    serialApi.sendSetDefault();

    std::string homeId;
    if( false == serialApi.getHomeId( homeId ) )
        std::cout << "FAILED to get homeId" << std::endl;
    std::cout << "Z-Wave homeId: " << homeId << std::endl;

    std::string dsk;
    if( false == serialApi.getDsk( dsk ) )
        std::cout << "FAILED to get DSK" << std::endl;
    std::cout << "Z-Wave DSK: " << dsk << std::endl;

    if( !homeId.empty() ) {
        int status = true;
        while( serialApi.sendNif() && false == exitSignal ) {
            status = serialApi.sendNif();
        }
        if( false == status )
            std::cout << "Send NIF FAILED" << std::endl << std::endl;
    }
}

}  // namespace
}  // namespace CertificationUtils
}  // namespace HubPlatform

int main( int argc, char* argv[] ) {
    using namespace HubPlatform::CertificationUtils;
    try {
        parseArgs( argc, argv );
        stopFirmware();
        startTxStream();
    }
    catch( const std::exception& ex ) {
        std::cerr << "Exception: " << ex.what() << std::endl;
    }
    catch( ... ) {
        std::cerr << "Unhandled exception!" << std::endl;
    }
    return 0;
}
