#include "device_firmware_update_manager/device_firmware_update_manager.h"
#include "common_utils/cpp14.h"
#include "test_utils/temporary_folder_wrapper.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <unistd.h>

namespace HubPlatform {

static bool operator==( const DeviceFirmwareUpdateManager::ExtendedLink& a,
                        const DeviceFirmwareUpdateManager::ExtendedLink& b ) {
    return a.image == b.image && a.checksum == b.checksum;
}

namespace UT {

using ::testing::_;

namespace {

bool fileExists( const DeviceFirmwareUpdateManager::Path& path ) {
    return access( path.c_str(), F_OK ) == 0;
}

class TestDownloader : public DeviceFirmwareUpdateManager::Downloader {
public:
    TestDownloader( DeviceFirmwareUpdateManager::Path path,
                    DeviceFirmwareUpdateManager::Downloader::ResultCallback resultCallback,
                    std::function< void() > onDelete )
    : m_path( std::move( path ) )
    , m_resultCallback( std::move( resultCallback ) )
    , m_onDelete( std::move( onDelete ) ) {
        EXPECT_NE( m_resultCallback, nullptr );
    }

    ~TestDownloader() override {
        m_onDelete();
    }

    bool isOk() const override {
        return m_isOk;
    }

    uint8_t getProgress() const override {
        return m_progress;
    }

    const DeviceFirmwareUpdateManager::Path& getPath() const {
        return m_path;
    }

    void update( bool isOk, uint8_t progress ) {
        m_isOk = isOk;
        m_progress = progress;

        m_resultCallback( *this );
    }

private:
    bool m_isOk = false;
    uint8_t m_progress = 0;

    DeviceFirmwareUpdateManager::Path m_path;
    DeviceFirmwareUpdateManager::Downloader::ResultCallback m_resultCallback;
    std::function< void() > m_onDelete;
};

class TestDelegate : public DeviceFirmwareUpdateManager::Delegate {
public:
    DeviceFirmwareUpdateManager::DownloaderConstUPtr createFirmwareDownloader(
        const DeviceFirmwareUpdateManager::ExtendedLink& link,
        const DeviceFirmwareUpdateManager::Path& destination,
        DeviceFirmwareUpdateManager::Downloader::ResultCallback resultCallback ) override {
        if( link.image == "broken_link" ) {
            return {};
        }

        auto downloader = std::make_unique< TestDownloader >(
            destination, resultCallback, [this, link]() { m_downloaders.erase( link.image ); } );

        m_downloaders.emplace( link.image, *downloader );

        return std::move( downloader );
    }

    DeviceFirmwareUpdateManager::Path getDestinationForLink( const DeviceFirmwareUpdateManager::Link& ) {
        auto fileName = m_folder.getPath() + "/fw.test.XXXXXX";
        close( mkstemp( &fileName[ 0 ] ) );
        return fileName;
    }

    bool startFirmwareUpdate( const DeviceFirmwareUpdateManager::Device& device,
                              const DeviceFirmwareUpdateManager::Target& target,
                              const DeviceFirmwareUpdateManager::Path& destination,
                              std::function< void( bool ) > resultCallback ) override {
        EXPECT_EQ( m_resultCallback, nullptr );
        m_resultCallback = std::move( resultCallback );

        return true;
    }

    MOCK_METHOD3( onFirmwareDownloadStatus, void( const DeviceFirmwareUpdateManager::ExtendedLink&, bool, uint8_t ) );

    MOCK_METHOD2( onFirmwareUpdateFinished,
                  void( const DeviceFirmwareUpdateManager::Device&,
                        const DeviceFirmwareUpdateManager::TargetsUpdateResults& ) );

    MOCK_METHOD1( onFirmwareUpdateStarted, void( const DeviceFirmwareUpdateManager::Device& ) );

    void addFirmwareUpdateTask( std::function< void() > task ) override {
        m_tasks.push_back( std::move( task ) );
    }

    bool onUpdateFinished( bool result ) {
        if( auto cb = std::move( m_resultCallback ) ) {
            cb( result );
            return true;
        }

        return false;
    }

    size_t runAllTasks() {
        size_t result = 0;
        while( !m_tasks.empty() ) {
            auto task = std::move( *m_tasks.begin() );
            m_tasks.erase( m_tasks.begin() );
            task();

            ++result;
        }

        return result;
    }

    bool hasDownloader( const DeviceFirmwareUpdateManager::Link& link ) const {
        return m_downloaders.count( link ) != 0;
    }

    DeviceFirmwareUpdateManager::Path getDownloaderPath( const DeviceFirmwareUpdateManager::Link& link ) const {
        auto it = m_downloaders.find( link );
        return it == m_downloaders.end() ? DeviceFirmwareUpdateManager::Path() : it->second.get().getPath();
    }

    void updateDownloader( const DeviceFirmwareUpdateManager::Link& link, bool isOk, uint8_t progress ) {
        EXPECT_EQ( m_downloaders.count( link ), 1 ) << link;
        auto it = m_downloaders.find( link );

        if( it != m_downloaders.end() ) {
            it->second.get().update( isOk, progress );
        }
    }

private:
    Tests::TemporaryFolderWrapper m_folder;
    std::function< void( bool ) > m_resultCallback;
    std::list< std::function< void() > > m_tasks;
    std::map< DeviceFirmwareUpdateManager::Link, std::reference_wrapper< TestDownloader > > m_downloaders;
};

}  // namespace

class DeviceFirmwareUpdateManagerTestSuite : public testing::Test {
protected:
};

TEST_F( DeviceFirmwareUpdateManagerTestSuite, oneScope_oneDevice_oneTarget ) {
    TestDelegate testDelegate;
    DeviceFirmwareUpdateManager manager( testDelegate );

    manager.addUpdateScope( { 10 }, { { 1, { "http://image.bin", "http://image.checksum" } } } );

    ASSERT_TRUE( testDelegate.hasDownloader( "http://image.bin" ) );
    const auto path = testDelegate.getDownloaderPath( "http://image.bin" );

    EXPECT_TRUE( fileExists( path ) );

    {
        EXPECT_CALL( testDelegate, onFirmwareDownloadStatus( _, _, _ ) ).Times( 0 );
        EXPECT_CALL( testDelegate, onFirmwareUpdateFinished( _, _ ) ).Times( 0 );
        EXPECT_CALL( testDelegate, onFirmwareUpdateStarted( _ ) ).Times( 0 );

        testDelegate.updateDownloader( "http://image.bin", true, 40 );
    }

    {
        EXPECT_CALL(
            testDelegate,
            onFirmwareDownloadStatus(
                DeviceFirmwareUpdateManager::ExtendedLink{ "http://image.bin", "http://image.checksum" }, true, 40 ) )
            .Times( 1 );

        EXPECT_CALL( testDelegate, onFirmwareUpdateStarted( _ ) ).Times( 0 );

        EXPECT_EQ( testDelegate.runAllTasks(), 1 );
    }

    {
        EXPECT_CALL( testDelegate,
                     onFirmwareDownloadStatus(
                         DeviceFirmwareUpdateManager::ExtendedLink{ "http://image.bin", "http://image.checksum" },
                         true,
                         MaxProgress ) )
            .Times( 1 );
        EXPECT_CALL( testDelegate, onFirmwareUpdateFinished( _, _ ) ).Times( 0 );
        EXPECT_CALL( testDelegate, onFirmwareUpdateStarted( 10 ) ).Times( 1 );

        testDelegate.updateDownloader( "http://image.bin", true, MaxProgress );
        EXPECT_EQ( testDelegate.runAllTasks(), 1 );

        EXPECT_TRUE( fileExists( path ) );
    }

    {
        const DeviceFirmwareUpdateManager::TargetsUpdateResults expectedUpdateResults = {
            { 1, DeviceFirmwareUpdateManager::TargetUpdateResult::UpdateDone },
        };

        EXPECT_CALL( testDelegate, onFirmwareDownloadStatus( _, _, _ ) ).Times( 0 );
        EXPECT_CALL( testDelegate, onFirmwareUpdateFinished( 10, expectedUpdateResults ) ).Times( 1 );
        EXPECT_CALL( testDelegate, onFirmwareUpdateStarted( _ ) ).Times( 0 );

        EXPECT_TRUE( testDelegate.onUpdateFinished( true ) );
        EXPECT_EQ( testDelegate.runAllTasks(), 0 );

        EXPECT_FALSE( fileExists( path ) );
    }
}

TEST_F( DeviceFirmwareUpdateManagerTestSuite, twoScopes_twoDevices_threeTargets ) {
    TestDelegate testDelegate;
    DeviceFirmwareUpdateManager manager( testDelegate, 2 );

    const DeviceFirmwareUpdateManager::TargetsUpdateResults expectedUpdateResults = {
        { 1, DeviceFirmwareUpdateManager::TargetUpdateResult::UpdateDone },
        { 2, DeviceFirmwareUpdateManager::TargetUpdateResult::UpdateDone },
        { 3, DeviceFirmwareUpdateManager::TargetUpdateResult::UpdateDone },
    };

    manager.addUpdateScope( { 10, 20 },
                            {
                                { 1, { "http://image_11.bin", "" } },
                                { 2, { "http://image_12.bin", "" } },
                                { 3, { "http://image_13.bin", "" } },
                            } );

    manager.addUpdateScope( { 11, 21 },
                            {
                                { 1, { "http://image_21.bin", "" } },
                                { 2, { "http://image_22.bin", "" } },
                                { 3, { "http://image_13.bin", "" } }  // the same as 1st scope
                            } );

    ASSERT_TRUE( testDelegate.hasDownloader( "http://image_11.bin" ) );
    const auto path_11 = testDelegate.getDownloaderPath( "http://image_11.bin" );

    ASSERT_TRUE( testDelegate.hasDownloader( "http://image_11.bin" ) );
    const auto path_12 = testDelegate.getDownloaderPath( "http://image_12.bin" );

    EXPECT_TRUE( fileExists( path_11 ) );
    EXPECT_TRUE( fileExists( path_12 ) );

    ASSERT_FALSE( testDelegate.hasDownloader( "http://image_13.bin" ) );
    ASSERT_FALSE( testDelegate.hasDownloader( "http://image_21.bin" ) );
    ASSERT_FALSE( testDelegate.hasDownloader( "http://image_22.bin" ) );

    EXPECT_CALL( testDelegate, onFirmwareUpdateStarted( _ ) ).Times( 4 );

    // download image_11.bin, download image_12.bin
    {
        EXPECT_CALL( testDelegate, onFirmwareDownloadStatus( _, true, MaxProgress ) ).Times( 2 );
        EXPECT_CALL( testDelegate, onFirmwareUpdateFinished( _, _ ) ).Times( 0 );

        testDelegate.updateDownloader( "http://image_11.bin", true, MaxProgress );
        testDelegate.updateDownloader( "http://image_12.bin", true, MaxProgress );

        EXPECT_EQ( testDelegate.runAllTasks(), 2 );
    }

    ASSERT_TRUE( testDelegate.hasDownloader( "http://image_13.bin" ) );
    const auto path_13 = testDelegate.getDownloaderPath( "http://image_13.bin" );

    ASSERT_TRUE( testDelegate.hasDownloader( "http://image_21.bin" ) );
    const auto path_21 = testDelegate.getDownloaderPath( "http://image_21.bin" );

    // update 10.1, 10.2
    {
        EXPECT_CALL( testDelegate, onFirmwareDownloadStatus( _, _, _ ) ).Times( 0 );
        EXPECT_CALL( testDelegate, onFirmwareUpdateFinished( _, _ ) ).Times( 0 );

        EXPECT_TRUE( testDelegate.onUpdateFinished( true ) );
        EXPECT_TRUE( testDelegate.onUpdateFinished( true ) );
        EXPECT_EQ( testDelegate.runAllTasks(), 0 );

        EXPECT_TRUE( fileExists( path_11 ) );
        EXPECT_TRUE( fileExists( path_12 ) );
    }

    // download image_13.bin, download image_21.bin
    {
        EXPECT_CALL( testDelegate, onFirmwareDownloadStatus( _, true, MaxProgress ) ).Times( 2 );
        EXPECT_CALL( testDelegate, onFirmwareUpdateFinished( _, _ ) ).Times( 0 );

        testDelegate.updateDownloader( "http://image_13.bin", true, MaxProgress );
        testDelegate.updateDownloader( "http://image_21.bin", true, MaxProgress );

        EXPECT_EQ( testDelegate.runAllTasks(), 2 );
    }

    // update 10.3
    {
        EXPECT_CALL( testDelegate, onFirmwareDownloadStatus( _, _, _ ) ).Times( 0 );
        EXPECT_CALL( testDelegate, onFirmwareUpdateFinished( 10, expectedUpdateResults ) ).Times( 1 );

        EXPECT_TRUE( testDelegate.onUpdateFinished( true ) );
        EXPECT_EQ( testDelegate.runAllTasks(), 0 );

        EXPECT_TRUE( fileExists( path_11 ) );
        EXPECT_TRUE( fileExists( path_12 ) );
        EXPECT_TRUE( fileExists( path_13 ) );
    }

    // update 20.1
    {
        EXPECT_CALL( testDelegate, onFirmwareDownloadStatus( _, _, _ ) ).Times( 0 );
        EXPECT_CALL( testDelegate, onFirmwareUpdateFinished( _, _ ) ).Times( 0 );

        EXPECT_TRUE( testDelegate.onUpdateFinished( true ) );
        EXPECT_EQ( testDelegate.runAllTasks(), 0 );

        EXPECT_FALSE( fileExists( path_11 ) );
        EXPECT_TRUE( fileExists( path_12 ) );
        EXPECT_TRUE( fileExists( path_13 ) );
    }

    // download image_22.bin
    {
        EXPECT_CALL( testDelegate,
                     onFirmwareDownloadStatus(
                         DeviceFirmwareUpdateManager::ExtendedLink{ "http://image_22.bin", "" }, true, MaxProgress ) )
            .Times( 1 );
        EXPECT_CALL( testDelegate, onFirmwareUpdateFinished( _, _ ) ).Times( 0 );

        testDelegate.updateDownloader( "http://image_22.bin", true, MaxProgress );

        EXPECT_EQ( testDelegate.runAllTasks(), 1 );
    }

    // update 20.2, 20.3
    {
        EXPECT_CALL( testDelegate, onFirmwareDownloadStatus( _, _, _ ) ).Times( 0 );
        EXPECT_CALL( testDelegate, onFirmwareUpdateFinished( 20, expectedUpdateResults ) ).Times( 1 );

        EXPECT_TRUE( testDelegate.onUpdateFinished( true ) );
        EXPECT_TRUE( testDelegate.onUpdateFinished( true ) );
        EXPECT_EQ( testDelegate.runAllTasks(), 0 );

        EXPECT_FALSE( fileExists( path_11 ) );
        EXPECT_FALSE( fileExists( path_12 ) );
        EXPECT_TRUE( fileExists( path_13 ) );
    }

    // update 11.1, 11.2, 11.3
    {
        EXPECT_CALL( testDelegate, onFirmwareDownloadStatus( _, _, _ ) ).Times( 0 );
        EXPECT_CALL( testDelegate, onFirmwareUpdateFinished( 11, expectedUpdateResults ) ).Times( 1 );

        EXPECT_TRUE( testDelegate.onUpdateFinished( true ) );
        EXPECT_TRUE( testDelegate.onUpdateFinished( true ) );
        EXPECT_TRUE( testDelegate.onUpdateFinished( true ) );
    }

    // update 21.1, 21.2, 21.3
    {
        EXPECT_CALL( testDelegate, onFirmwareDownloadStatus( _, _, _ ) ).Times( 0 );
        EXPECT_CALL( testDelegate, onFirmwareUpdateFinished( 21, expectedUpdateResults ) ).Times( 1 );

        EXPECT_TRUE( testDelegate.onUpdateFinished( true ) );
        EXPECT_TRUE( testDelegate.onUpdateFinished( true ) );
        EXPECT_TRUE( testDelegate.onUpdateFinished( true ) );

        EXPECT_FALSE( fileExists( path_13 ) );
    }
}

TEST_F( DeviceFirmwareUpdateManagerTestSuite, failedDownload_failedUpdate ) {
    TestDelegate testDelegate;
    DeviceFirmwareUpdateManager manager( testDelegate, 3 );

    const DeviceFirmwareUpdateManager::TargetsUpdateResults expectedUpdateResults = {
        { 1, DeviceFirmwareUpdateManager::TargetUpdateResult::DownloadFailed },
        { 2, DeviceFirmwareUpdateManager::TargetUpdateResult::DownloadFailed },
        { 3, DeviceFirmwareUpdateManager::TargetUpdateResult::UpdateFailed },
        { 4, DeviceFirmwareUpdateManager::TargetUpdateResult::UpdateDone }
    };

    EXPECT_CALL( testDelegate, onFirmwareUpdateStarted( 10 ) ).Times( 1 );
    EXPECT_CALL( testDelegate, onFirmwareUpdateFinished( 10, expectedUpdateResults ) ).Times( 1 );

    manager.addUpdateScope( { 10 },
                            {
                                { 1, { "broken_link", "" } },
                                { 2, { "http://image2.bin", "" } },
                                { 3, { "http://image3.bin", "" } },
                                { 4, { "http://image4.bin", "" } },
                            } );

    {
        EXPECT_CALL(
            testDelegate,
            onFirmwareDownloadStatus( DeviceFirmwareUpdateManager::ExtendedLink{ "http://image2.bin", "" }, false, 0 ) )
            .Times( 1 );
        testDelegate.updateDownloader( "http://image2.bin", false, 0 );
        EXPECT_EQ( testDelegate.runAllTasks(), 1 );
    }

    {
        EXPECT_CALL( testDelegate, onFirmwareDownloadStatus( _, true, MaxProgress ) ).Times( 2 );

        testDelegate.updateDownloader( "http://image3.bin", true, MaxProgress );
        testDelegate.updateDownloader( "http://image4.bin", true, MaxProgress );

        EXPECT_EQ( testDelegate.runAllTasks(), 2 );
    }

    EXPECT_TRUE( testDelegate.onUpdateFinished( false ) );
    EXPECT_TRUE( testDelegate.onUpdateFinished( true ) );
}

TEST_F( DeviceFirmwareUpdateManagerTestSuite, storedImagesLimit ) {
    TestDelegate testDelegate;
    DeviceFirmwareUpdateManager manager( testDelegate, 1, 1 );

    EXPECT_CALL( testDelegate, onFirmwareUpdateStarted( 10 ) ).Times( 1 );

    manager.addUpdateScope( { 10, 20 }, { { 1, { "http://image1.bin", "" } }, { 2, { "http://image2.bin", "" } } } );

    {
        EXPECT_CALL( testDelegate,
                     onFirmwareDownloadStatus(
                         DeviceFirmwareUpdateManager::ExtendedLink{ "http://image1.bin", "" }, true, 100 ) )
            .Times( 1 );
        testDelegate.updateDownloader( "http://image1.bin", true, 100 );
        EXPECT_EQ( testDelegate.runAllTasks(), 1 );

        EXPECT_FALSE( testDelegate.hasDownloader( "http://image2.bin" ) );
    }

    {
        EXPECT_TRUE( testDelegate.onUpdateFinished( true ) );

        EXPECT_TRUE( testDelegate.hasDownloader( "http://image2.bin" ) );
    }
}

}  // namespace UT
}  // namespace HubPlatform
