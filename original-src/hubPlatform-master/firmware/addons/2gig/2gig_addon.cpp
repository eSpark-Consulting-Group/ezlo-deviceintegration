#include "2gig/2gig_addon.h"
#include "2gig/2gig_client.h"
#include "2gig/2gig_instrumentation_client.h"
#include "2gig/2gig_persistence_client.h"
#include "2gig/2gig_devices_repository.h"
#include "2gig/2gig_devices_reachability_manager.h"
#include "2gig/version/2gig_addon_version.h"
#include "2gig/events/device_discovering_status_updated.h"
#include "2gig/resources/enum_utils.h"
#include "2gig/resources/resources.h"

#include "2gig/2gig.h"
#include "2gig/device_descriptor.h"
#include "2gig/devices_repository.h"

#include "common_utils/backup/backup_helper.h"
#include "common_utils/backup/restore_helper.h"
#include "common_utils/lock_helper.h"
#include "common_utils/cpp14.h"
#include "system_utils/filesystem.h"
#include "log_utils/ezlo_log.h"
#include "tiny/tiny_consolecolors.h"
#include "json_utils/json_utils.h"

namespace HubPlatform {
namespace TwoGig {

const char* const Addon::Name = "2gig";
const char* const Addon::Version = "1.0";

namespace {
constexpr auto DeviceDiscoveringTimeout = std::chrono::minutes( 1 );
}

Addon::Addon( const std::string& devicePath, const std::string& storageFilePath, const std::string& resetPinGpio )
: Addons::Addon( Name )
, m_devicePath( devicePath )
, m_oldStorageFilePath( storageFilePath )
, m_resetPinGpio( resetPinGpio )
, m_client( std::make_unique< ClientImpl >( *this ) )
, m_instrumentationClient( std::make_unique< InstrumentationClientImpl >() ) {}

Addon::~Addon() = default;

CommonUtils::AddonVersion Addon::getVersion() const {
    return Addons::TwoGig::getVersion();
}

void Addon::migrateOldDevicesStorage( DevicesRepositoryImpl& newStorage ) {
    if( !filesystem::isFileExist( m_oldStorageFilePath ) ) {
        return;
    }

    Log::info( cc::info( "Migration of old devices was started!" ) );
    auto oldRawStorage = PersistenceClientImpl::createForReading( m_oldStorageFilePath );

    if( newStorage.migrateOldDevices( *oldRawStorage ) ) {
        oldRawStorage.reset();
        filesystem::removeFile( m_oldStorageFilePath.c_str() );
        Log::info( cc::info( "Migration of old devices was done! All devices migrated." ) );
    }
    else {
        Log::warn( cc::warn( "Cannot migrate old devices. Migration process was" ) + cc::error( "canceled" ) +
                   cc::warn( "." ) );
    }
}

void Addon::initReachableStateRecheck() {
    m_reachabilityManager->checkAllDevicesReachableStates();
}

void Addon::setUp() {
    Base::setUp();

    join( GroupsNames::SYSTEM_GROUP );

    auto devices = std::make_unique< DevicesRepositoryImpl >( getDBClient() );
    migrateOldDevicesStorage( *devices );
    m_devices = std::move( devices );
    std::shared_ptr< DevicesReachabilityManagerImpl > reachabilityManager =
        std::make_shared< DevicesReachabilityManagerImpl >( *m_client, *m_devices );
    Services::Service::setReporter( std::make_unique< TwoGigReporter >( getName(), reachabilityManager ) );
    m_reachabilityManager = reachabilityManager;

    m_instance = ::TwoGig::createInstance(
        m_devicePath, m_resetPinGpio, *m_client, *m_devices, *m_reachabilityManager, m_instrumentationClient.get() );

    m_instance->start();

    createPostponedTask( [this]( Services::PostponedTask& ) { initReachableStateRecheck(); } )
        ->start( std::chrono::minutes( 10 ), true );
}

void Addon::tearDown() {
    if( m_instance )
        m_instance->stop();

    m_instance.reset();
    m_devices.reset();

    Base::tearDown();
}

void Addon::handleMethodCall( const JsonObject& json, MessageSubClasses requestType, const std::string& sender ) {
    using MethodHandler = void ( Addon::* )( const JsonObject& json );
    using MethodHandlersMap = std::unordered_map< MessageSubClasses, MethodHandler, MessageSubClassesHash >;

    static const MethodHandlersMap methodHandlers = {
        { TWOGIG_RESET, &Addon::reset },
        { TWOGIG_START_DEVICE_DISCOVERING, &Addon::startDeviceDiscovering },
        { TWOGIG_STOP_DEVICE_DISCOVERING, &Addon::stopDeviceDiscovering },
        { TWOGIG_ADD_DEVICE, &Addon::addDevice },
        { TWOGIG_REMOVE_DEVICE, &Addon::removeDevice },
        { TWOGIG_CONFIGURE_DEVICE, &Addon::configureDevice },
    };

    const auto it = methodHandlers.find( requestType );
    if( it != methodHandlers.end() ) {
        const auto handler = it->second;
        ( this->*handler )( json );
    }
}

JsonObject Addon::handleRequestFromBus( const JsonObject& request, MessageSubClasses requestType ) {
    switch( requestType ) {
    case GLOBAL_LOCK: {
        auto& lockHelper = CommonUtils::LockHelper::global();
        lockHelper.tryLock( request );
        return lockHelper.getResponse();
    }
    case GLOBAL_UNLOCK: {
        auto& lockHelper = CommonUtils::LockHelper::global();
        lockHelper.unlock( request );
        return lockHelper.getResponse();
    }
    case BACKUP_CREATE: {
        CommonUtils::BackupHelper helper( getDBClient(), request );
        helper.backupDatabase( Resources::DB::TableNames::Devices );
        return helper.getResponse();
    }
    default:
        return Service::handleRequestFromBus( request, requestType );
    };

    return JSON_EMPTY_OBJ;
}

int Addon::restoreBackup( const std::string& tempBackupFolder ) {
    CommonUtils::RestoreHelper helper( getDBClientSafe(), tempBackupFolder );
    helper.restoreDatabase( Resources::DB::TableNames::Devices );
    return helper.getReturnCode();
}

void Addon::reset( const JsonObject& params ) {
    addTask( [this] {
        m_instance->stop();
        m_instance->reset();
        m_instance->start();
    } );
}

void Addon::startDeviceDiscovering( const JsonObject& params ) {
    addTask( [this] {
        m_instance->startDeviceDiscovering();
        emit( DeviceDiscoveringStatusUpdated( ActionStatus::Started ) );

        m_discoveringTimeoutTask = std::make_unique< Services::TaskLoopTimer >( *this, [this] {
            m_instance->stopDeviceDiscovering();
            emit( DeviceDiscoveringStatusUpdated( ActionStatus::Timeout ) );
        } );

        m_discoveringTimeoutTask->start( DeviceDiscoveringTimeout, false );
    } );
}

void Addon::stopDeviceDiscovering( const JsonObject& params ) {
    if( m_discoveringTimeoutTask ) {
        m_discoveringTimeoutTask.reset();

        addTask( [this] {
            m_instance->stopDeviceDiscovering();
            emit( DeviceDiscoveringStatusUpdated( ActionStatus::Finished ) );
        } );
    }
}

void Addon::addDevice( const JsonObject& params ) {
    const JsonObject& txIds = params[ Resources::TxIds ];
    const JsonObject& model = params[ Resources::Events::Parameters::Model ];
    const std::string& deviceTypeStr = params[ Resources::Events::Parameters::DeviceType ];

    auto deviceType = Resources::EnumUtils< ::TwoGig::DeviceType >::fromString( deviceTypeStr );

    if( !model.is_null() ) {
        m_instance->addDevice( { txIds, deviceType, model.get< std::string >() } );
    }
    else {
        m_instance->addDevice( { txIds, deviceType } );
    }
}

void Addon::removeDevice( const JsonObject& params ) {
    const JsonObject& txIds = params[ Resources::TxIds ];

    m_instance->removeDevice( txIds );
}

void Addon::configureDevice( const JsonObject& params ) {
    const std::vector< uint32_t >& txIds = params[ Resources::TxIds ];
    const JsonObject& config = params[ Resources::Configuration ];

    const JsonObject& pinLength = config[ Resources::PinLength ];
    const JsonObject& pinInputTimeout = config[ Resources::PinInputTimeout ];

    for( uint32_t txId : txIds ) {
        if( !pinLength.is_null() )
            m_devices->setPinCodeLength( txId, pinLength.get< uint8_t >() );

        if( !pinInputTimeout.is_null() ) {
            auto seconds = pinInputTimeout.get< uint8_t >();
            m_devices->setPinCodeInputTimeout( txId, std::chrono::seconds( seconds ) );
        }
    }
}

}  // namespace TwoGig
}  // namespace HubPlatform
