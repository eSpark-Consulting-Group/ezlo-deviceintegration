
#include "test_utils/component_tests/test_application.h"

#include "system_utils/environment.h"
#include "system_utils/filesystem.h"
#include "system_utils/find_free_local_port.h"
#include "system_utils/process/shared_mutex.h"
#include "test_utils/test_environment.h"

#include "tiny/tiny_consolecolors.h"
#include "log_utils/ezlo_log.h"

#include <unistd.h>
#include <fstream>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>

#include <gtest/gtest.h>
#include <json_utils/json_utils.h>

namespace HubPlatform {
namespace Tests {

TestApplication::TestApplication( const std::string& etalonRootFolderPath )
: TestApplication::TestApplication( Tests::TemporaryClonedRootFolderWrapper{ etalonRootFolderPath } ) {}

TestApplication::TestApplication( Tests::TemporaryClonedRootFolderWrapper rootFolderWrapper )
: Service( "component_test" )
, m_spreadId( 0 )
, m_rootFolderWrapper( std::move( rootFolderWrapper ) ) {
    filesystem::createFolder( getLogFolderPath() );
    filesystem::createFolder( Environment::getBinariesFolder() + "/addons" );
}

TestApplication::~TestApplication() {
    waitAllChildApplications();

    if( m_testsThread.joinable() ) {
        m_testsThread.join();
    }
}

void TestApplication::setUpChildApplications() {}

void TestApplication::executeSpread() {
    execute( Tests::getToolchainFolder() + "/usr/sbin/spread",
             { "-c", HubPlatform::Environment::getConfigsFolder() + "/spread.conf" },
             getLogFolderPath() + "/spread.log" );
}

void TestApplication::executeCoreService( const std::string& serviceName,
                                          const std::string& serviceId,
                                          std::vector< std::string > parameters ) {
    executeService( Tests::getBuildBinFolder() + "/" + serviceName, serviceName, serviceId, std::move( parameters ) );
}

void TestApplication::executeAddonService( const std::string& addonName,
                                           const std::string& serviceId,
                                           std::vector< std::string > parameters ) {
    executeService(
        Tests::getBuildAddonsBinFolder() + "/" + addonName, addonName, "addon." + serviceId, std::move( parameters ) );
}

void TestApplication::addStartingServiceOnBus( const std::string& serviceId ) {
    std::unique_lock< std::mutex > lock( m_startingServicesMutex );
    m_startingServices.insert( serviceId );
}

void TestApplication::executeService( const std::string& servicePath,
                                      const std::string& logFileName,
                                      const std::string& serviceId,
                                      std::vector< std::string > parameters ) {
    addStartingServiceOnBus( serviceId );

    parameters.push_back( std::string( "--bus=" ) + std::to_string( m_spreadId ) );
    parameters.push_back( std::string( "--root=" ) + HubPlatform::Environment::getFirmwareRootFolder() );
    parameters.push_back( std::string( "--tmp=" ) + HubPlatform::Environment::getTempDataFolder() );
    parameters.push_back( std::string( "--spec=" ) + HubPlatform::Environment::getProvisionDataFolder() );
    parameters.push_back( std::string( "--logs=" ) + HubPlatform::Environment::getLogsFolder() );

    execute( servicePath, std::move( parameters ), getLogFolderPath() + "/" + logFileName + ".log" );
}

void TestApplication::createLinkToAddonInTestFolder( const std::string& addonBinary ) {
    filesystem::createSymbolicLink( getBuildAddonsBinFolder() + "/" + addonBinary,
                                    getCurrentTestPath() + "/bin/addons/" + addonBinary );
}

void TestApplication::setUp() {
    LOGGER_DEBUG( "getUniqueFreeLocalPort TestApplication pending..." );
    Process::SharedMutex processMutex( "uniquePortMutex" );
    std::unique_lock< Process::SharedMutex > lock( processMutex );

    LOGGER_DEBUG( "getUniqueFreeLocalPort TestApplication" );

    m_spreadId = system_utils::getUniqueFreeLocalPort();
    generateConfigForSpread( m_spreadId );
    Environment::setBusName( std::to_string( m_spreadId ) );
    LOGGER_DEBUG( "spreadId: " + std::to_string( m_spreadId ) );

    executeSpread();

    lock.unlock();

    Service::setUp();

    join( GroupsNames::BROADCAST_GROUP );

    addStartingServiceOnBus( getName() );

    try {
        setUpChildApplications();
    }
    catch( const std::runtime_error& ex ) {
        Log::error( cc::info( "Exception during setting up applications: " ) + cc::error( ex.what() ) );
        Service::setUp();  // to init m_busWrapper
        addTask( [this]() { stop(); } );
        return;
    }

    m_testsThread = std::thread( [this]() {
        checkEverythingBeforeTestsStart();

        m_testsResult = RUN_ALL_TESTS();
        if( m_testsResult != 0 ) {
            m_rootFolderWrapper.keepContent();
        }

        stop();
    } );
}

void TestApplication::tearDown() {
    waitAllChildApplications();

    Service::tearDown();

    if( m_testsResult != 0 ) {
        throw TestApplicationError( cc::error( "Tests were failed! You can check logs here: " ) +
                                    cc::p( getLogFolderPath() ) );
    }
}

void TestApplication::checkEverythingBeforeTestsStart() {
    waitForAllChildApplicationsStart();
}

void TestApplication::generateConfigForSpread( const std::uint16_t spreadId ) {
    const auto spreadRuntimeDir = HubPlatform::Environment::getTempDataFolder() + "/spread";
    const auto spreadConfPath = HubPlatform::Environment::getConfigsFolder() + "/spread.conf";

    filesystem::createFolder( spreadRuntimeDir );

    std::fstream spreadConf( spreadConfPath, std::ios_base::out | std::ios_base::trunc );

    spreadConf << "Spread_Segment 127.0.0.255:" << spreadId << " {" << std::endl;
    spreadConf << "\tlocalhost 127.0.0.1" << std::endl;
    spreadConf << "}" << std::endl;

    spreadConf << "DebugFlags = { ALL }" << std::endl
               << "EventPriority = pDEBUG" << std::endl
               << "MaxSessionMessages = 5000" << std::endl
               << "DangerousMonitor = true" << std::endl
               << "DaemonUser = root" << std::endl
               << "DaemonGroup = root" << std::endl
               << "TokenTimeout = 1250" << std::endl
               << "HurryTimeout = 500" << std::endl
               << "AliveTimeout = 250" << std::endl
               << "JoinTimeout = 250" << std::endl
               << "RepTimeout = 625" << std::endl
               << "SegTimeout = 500" << std::endl
               << "GatherTimeout = 1250" << std::endl
               << "FormTimeout = 1250" << std::endl
               << "LookupTimeout = 30000" << std::endl
               << "EventLogFile = " << getLogFolderPath() << "/spread1.log" << std::endl
               << "RuntimeDir = " << spreadRuntimeDir << std::endl;

    if( !spreadConf.good() ) {
        throw TestApplicationError( std::string( "Cannot write config file for spread: " ) + spreadConfPath );
    }

    spreadConf.close();
}

void TestApplication::execute( const std::string& appPath,
                               std::vector< std::string > parameters,
                               const std::string& logFilePath ) {
    std::lock_guard< std::mutex > guard( m_applicationsMutex );

    const auto threadId = ++m_applicationsIdCounter;
    m_applications.push_back( { threadId, std::thread( [this, threadId, appPath, parameters, logFilePath]() {
                                    executeApplication( appPath, std::move( parameters ), logFilePath, threadId );
                                    onApplicationClosed( threadId );
                                } ) } );
}

void TestApplication::waitAllChildApplications() {
    bool isAllApplicationsClosed = false;

    do {
        std::this_thread::sleep_for( std::chrono::milliseconds( 30 ) );

        {
            std::lock_guard< std::mutex > guard( m_applicationsMutex );
            isAllApplicationsClosed = m_applications.empty();
        }

    } while( !isAllApplicationsClosed );
}

void TestApplication::onApplicationClosed( std::uint64_t applicationId ) {
    if( isRunning() ) {
        m_rootFolderWrapper.keepContent();
        Log::warn( cc::info( "Application closed before tests were finished!(" ) + cc::note( "AppID: " ) +
                   cc::debug( std::to_string( applicationId ) ) + cc::info( ")" ) );
    }

    {
        std::lock_guard< std::mutex > guard( m_applicationsMutex );
        auto itFind = std::find_if(
            m_applications.begin(), m_applications.end(), [applicationId]( const ApplicationInfo& appInfo ) {
                return appInfo.applicationId == applicationId;
            } );

        if( itFind != m_applications.end() ) {
            if( itFind->watchdogThread.joinable() ) {
                itFind->watchdogThread.detach();
            }
        }

        m_applications.erase( itFind );
    }
}

void TestApplication::executeApplication( const std::string& appPath,
                                          std::vector< std::string > parameters,
                                          const std::string& logFilePath,
                                          std::uint64_t applicationId ) {
    std::cout << "Run: " << cc::p( appPath );
    for( const auto& param : parameters ) {
        std::cout << " " << cc::warn( param );
    }
    std::cout << " > " << cc::p( logFilePath ) << " 2>&1" << std::endl;

    const auto childPid = ::fork();
    if( childPid == 0 ) {
        int logFileHandle = open( logFilePath.c_str(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR );
        dup2( logFileHandle, STDOUT_FILENO );
        dup2( logFileHandle, STDERR_FILENO );
        close( logFileHandle );

        std::unique_ptr< const char* [] > argv( new const char*[ parameters.size() + 2 ] );
        argv[ 0 ] = appPath.c_str();

        for( size_t i = 0; i < parameters.size(); ++i ) {
            std::cout << " " << cc::warn( parameters[ i ] );
            argv[ i + 1 ] = parameters[ i ].c_str();
        }

        argv[ parameters.size() + 1 ] = nullptr;

        ::execv( appPath.c_str(), (char**) argv.get() );
    }

    uint8_t killAttempts = 0;
    pid_t childProcessState = 0;
    bool needToKill = false;

    auto leaveLogsAndPrintErrorMessage = [&]() {
        m_rootFolderWrapper.keepContent();
        std::cerr << cc::p( appPath ) << cc::error( " failed" ) << ". Check log file: " << cc::p( logFilePath )
                  << std::endl;
        std::cerr << cc::error( "################################################################################" )
                  << std::endl;
        system( ( std::string( "tail -n 64 " ) + logFilePath ).c_str() );
        std::cerr << cc::error( "################################################################################" )
                  << std::endl;
    };

    do {
        int status = 0;
        childProcessState = waitpid( childPid, &status, WNOHANG );
        if( childProcessState == 0 ) {
            std::this_thread::sleep_for( std::chrono::seconds( 1 ) );
        }
        else if( childProcessState == -1 ) {
            leaveLogsAndPrintErrorMessage();
            break;
        }
        else {
            const auto returnValue = WEXITSTATUS( status );
            if( returnValue == 0 ) {
                std::cout << cc::p( appPath ) << " exited " << cc::success( "successfully" ) << "." << std::endl;
            }
            else {
                leaveLogsAndPrintErrorMessage();
            }
            break;
        }

        if( needToKill || !isRunning() ) {
            if( !needToKill ) {
                if( applicationId != 0 ) {
                    std::lock_guard< std::mutex > guard( m_applicationsMutex );
                    if( m_applications.empty() ) {
                        needToKill = true;
                    }
                    else {
                        needToKill = ( m_applications.back().applicationId == applicationId );
                    }
                }
                else {
                    needToKill = true;
                }
            }

            if( needToKill ) {
                if( killAttempts == 0 ) {
                    kill( childPid, SIGTERM );
                }
                else if( killAttempts >= 5 ) {
                    kill( childPid, SIGKILL );
                }

                ++killAttempts;
                std::this_thread::sleep_for( std::chrono::seconds( 3 ) );
            }
        }

    } while( true );
}

const std::string& TestApplication::getLogFolderPath() const {
    static const auto logFolderPath = Environment::getFirmwareRootFolder() + "/log";
    return logFolderPath;
}

const std::string& TestApplication::getCurrentTestPath() const {
    return m_rootFolderWrapper.getPath();
}

void TestApplication::handleMessageFromBus( const Services::Message& message, const std::string& sender ) {
    Services::Service::handleMessageFromBus( message, sender );
    if( message.msgClass == EVENT ) {
        if( message.msgSubClass == SERVICE_STARTED ) {
            std::unique_lock< std::mutex > lock( m_startingServicesMutex );
            auto itFind = m_startingServices.find( json_get_str( message.data, "id" ) );
            if( itFind != m_startingServices.end() ) {
                m_startingServices.erase( itFind );
            }
            else {
                Log::warn( cc::warn( "Not expected service was started: " ) + cc::j( message.data ) );
            }
        }
        else {
            handleInternalBroadcast( message );
        }
    }
}

void TestApplication::handleMethodCall( const JsonObject& json,
                                        MessageSubClasses requestType,
                                        const std::string& sender ) {
    Services::Service::handleMethodCall( json, requestType, sender );
    handleInternalMethodCall( json, requestType, sender );
}

void TestApplication::handleInternalBroadcast( const Services::Message& message ) {
    // Do nothing
}

void TestApplication::handleInternalMethodCall( const JsonObject& json,
                                                MessageSubClasses requestType,
                                                const std::string& sender ) {}

void TestApplication::waitForAllChildApplicationsStart() {
    bool isAllApplicationsStarted = false;

    do {
        std::this_thread::sleep_for( std::chrono::milliseconds( 30 ) );

        {
            std::lock_guard< std::mutex > guard( m_startingServicesMutex );
            isAllApplicationsStarted = m_startingServices.empty();
        }

    } while( !isAllApplicationsStarted );
}

}  // namespace Tests
}  // namespace HubPlatform
