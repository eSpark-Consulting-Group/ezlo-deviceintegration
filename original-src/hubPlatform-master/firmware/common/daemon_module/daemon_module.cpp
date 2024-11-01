#include "daemon_module.h"

#include "system_utils/process/forker.h"
#include "system_utils/environment.h"
#include "system_utils/logger.h"
#include "tiny/tiny_command_line_parser.h"

#include <cassert>
#include <cstdlib>
#include <csignal>
#include <syslog.h>
#include <unistd.h>
#include <iostream>

namespace HubPlatform {

DaemonModule* DaemonModule::m_instance = nullptr;

DaemonModule::DaemonModule() {
    assert( m_instance == nullptr );
    m_instance = this;
}

DaemonModule::~DaemonModule() {
    assert( m_instance != nullptr );
    m_instance = nullptr;
}

void DaemonModule::defineDaemonParameters( tiny::command_line::parser& /*parser*/ ) {}

int DaemonModule::run( int argc, char* argv[] ) {
    using namespace SystemUtils;
    auto exitResult = EXIT_SUCCESS;
    try {
        tiny::command_line::parser parser;
        std::string tempBackupFolder;
        parser.on( "daemon", "System\nDaemonize", &Process::daemonize );
        parser.on( "restore_backup",
                   "System\nRestore data from specified folder and exit.",
                   [&tempBackupFolder]( const std::string& value ) { tempBackupFolder = value; } );
        parser.on( "root", "System\nDefine firmware root folder", []( const std::string& rootPath ) {
            Environment::setFirmwareRootFolder( rootPath );
            logInfo( &std::cout ) << "Root folder was changed: " << rootPath << std::endl;
        } );
        parser.on( "logs", "System\nDefine logs folder", []( const std::string& logsPath ) {
            Environment::setLogsFolder( logsPath );
            logInfo( &std::cout ) << "Logs folder was changed: " << logsPath << std::endl;
        } );
        parser.on( "bus", "System\nDefine bus name", []( const std::string& busName ) {
            Environment::setBusName( busName );
            logInfo( &std::cout ) << "Bus name was changed: " << busName << std::endl;
        } );
        parser.on( "tmp", "System\nDefine folder for temporary files", []( const std::string& tmpPath ) {
            Environment::setTempDataFolder( tmpPath );
            logInfo( &std::cout ) << "Temporary folder was changed: " << tmpPath << std::endl;
        } );
        parser.on( "spec", "System\nInternal usage folder", []( const std::string& provPath ) {
            Environment::setProvisionDataFolder( provPath );
            logInfo( &std::cout ) << "Internal folder was changed: " << provPath << std::endl;
        } );
        parser.on( "help", "System\nShow help info", [&parser]() {
            logInfo( &std::cout ) << parser.help_text() << "\n";
            ::_exit( 0 );
        } );

        defineDaemonParameters( parser );

        parser.parse( argc, argv );

        openlog( parser.app_name().c_str(), LOG_PID, LOG_DAEMON );

        signal( SIGTTOU, SIG_IGN );
        signal( SIGTTIN, SIG_IGN );

        signal( SIGHUP, DaemonModule::handleSignal );
        signal( SIGCHLD, DaemonModule::handleSignal );
        signal( SIGTERM, DaemonModule::handleSignal );
        signal( SIGINT, DaemonModule::handleSignal );

        setBinaryName( argv[ 0 ] );

        if( tempBackupFolder.empty() ) {
            runImpl();
        }
        else {
            exitResult = restoreBackup( tempBackupFolder );
        }

        closelog();
    }
    catch( const std::exception& exception ) {
        logError() << "Standard exception was caught(" << typeid( exception ).name() << "): " << exception.what()
                   << std::endl;
        exitResult = EXIT_FAILURE;
    }
    catch( ... ) {
        logError() << "Unknown exception was caught" << std::endl;
        exitResult = EXIT_FAILURE;
    }
    return exitResult;
}

void DaemonModule::handleSignal( int signal ) {
    assert( m_instance );
    using namespace SystemUtils;

    switch( signal ) {
    case SIGHUP:
        logError() << "SIGHUP was unhandled" << std::endl;
        break;

    case SIGCHLD:
        break;

    case SIGTERM:
    case SIGINT:
        logError() << "Terminate instance..." << std::endl;
        m_instance->onTerminate();
        break;

    default:
        logError() << "Unhandled signal: " << signal << std::endl;
        break;
    }
}

}  // namespace HubPlatform
