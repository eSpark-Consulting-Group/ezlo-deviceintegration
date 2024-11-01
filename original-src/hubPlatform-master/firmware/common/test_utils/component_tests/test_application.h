
#pragma once

#include "service_interfaces/service.h"

#include "test_utils/temporary_cloned_root_folder_wrapper.h"

#include <list>

namespace HubPlatform {
namespace Tests {

class TestApplicationError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class TestApplication : public Services::Service {
public:
    struct ApplicationInfo {
        std::uint64_t applicationId;
        std::thread watchdogThread;
    };

    using ExecutedApplications = std::list< ApplicationInfo >;
    using StartingServices = std::set< std::string >;

public:
    TestApplication( const std::string& etalonRootFolderPath );

    TestApplication( Tests::TemporaryClonedRootFolderWrapper rootFolderWrapper );

    ~TestApplication() override;

    void generateConfigForSpread( const std::uint16_t spreadId );

    void execute( const std::string& appPath, std::vector< std::string > parameters, const std::string& logFilePath );

protected:
    void setUp() override;

    virtual void setUpChildApplications();

    virtual void checkEverythingBeforeTestsStart();

    void tearDown() override;

    void onApplicationClosed( std::uint64_t applicationId );

protected:
    void handleMessageFromBus( const Services::Message& message, const std::string& sender ) override;
    void handleMethodCall( const JsonObject& json, MessageSubClasses requestType, const std::string& sender ) override;

    virtual void handleInternalBroadcast( const Services::Message& message );
    virtual void handleInternalMethodCall( const JsonObject& json,
                                           MessageSubClasses requestType,
                                           const std::string& sender );

    void addStartingServiceOnBus( const std::string& serviceId );

    const std::string& getLogFolderPath() const;

    const std::string& getCurrentTestPath() const;

    void executeSpread();

    void executeCoreService( const std::string& serviceName,
                             const std::string& serviceId,
                             std::vector< std::string > parameters = {} );

    void executeAddonService( const std::string& addonName,
                              const std::string& serviceId,
                              std::vector< std::string > parameters = {} );

    void executeService( const std::string& servicePath,
                         const std::string& logFileName,
                         const std::string& serviceId,
                         std::vector< std::string > parameters = {} );

    // @brief Extention manage requires link to load addon successfully
    void createLinkToAddonInTestFolder( const std::string& addonBinary );

    void waitForAllChildApplicationsStart();

private:
    void executeApplication( const std::string& appPath,
                             std::vector< std::string > parameters,
                             const std::string& logFilePath,
                             std::uint64_t applicationId = 0 );

    void waitAllChildApplications();

private:
    std::uint16_t m_spreadId;
    std::atomic_int m_testsResult{ 0 };

    Tests::TemporaryClonedRootFolderWrapper m_rootFolderWrapper;

    std::mutex m_applicationsMutex;
    ExecutedApplications m_applications;
    std::uint64_t m_applicationsIdCounter = 0;

    std::mutex m_startingServicesMutex;
    StartingServices m_startingServices;

    std::thread m_testsThread;
};

}  // namespace Tests
}  // namespace HubPlatform
