#include "zwave/zwave_addon.h"
#include "zwave/common/auth_mode_utils.h"
#include "zwave/common/zwave_errors_utils.h"
#include "zwave/resources/zwave_resources.h"
#include "zwave/znet.h"
#include "zwave/znet_factory.h"
#include "zwave/zwave_reporter.h"
#include "zwave/zwave_node_setting.h"
#include "zwave/serialization/zwave_node_json_serializer.h"
#include "zwave/provisioning/provisioning_list_provider.h"
#include "zwave/provisioning/provisioning_list_utils.h"
#include "zwave/version/zwave_addon_version.h"
#include "zwave/script_events/module_reset.h"
#include "zwave/script_events/action_busy.h"
#include "zwave/script_events/configuration_value_updated.h"
#include "zwave/script_events/configuration_request_status_updated.h"
#include "zwave/script_events/node_added.h"
#include "zwave/script_events/node_updated.h"
#include "zwave/script_events/reachable_state_updated.h"
#include "zwave/script_events/value_updated.h"
#include "zwave/script_events/association_updated.h"
#include "zwave/script_events/token_value_updated.h"
#include "zwave/script_events/float_value_updated.h"
#include "zwave/script_events/rgb_value_updated.h"
#include "zwave/script_events/button_state_updated.h"
#include "zwave/script_events/scene_activated.h"
#include "zwave/script_events/node_removed.h"
#include "zwave/script_events/all_nodes_removed.h"
#include "zwave/script_events/unknown_node_removed.h"
#include "zwave/script_events/s2_select_authentication_modes.h"
#include "zwave/script_events/s2_request_device_specific_key.h"
#include "zwave/script_events/s2_show_device_side_key.h"
#include "zwave/script_events/include_event.h"
#include "zwave/script_events/exclude_event.h"
#include "zwave/script_events/reconstruct_event.h"
#include "zwave/script_events/reconstruct_progress_error_event.h"
#include "zwave/script_events/reconstruct_progress_event.h"
#include "zwave/script_events/binary_sensor_value_updated.h"
#include "zwave/script_events/thermostat_setpoint_value_updated.h"
#include "zwave/script_events/thermostat_mode_updated.h"
#include "zwave/script_events/multilevel_sensor_value_updated.h"
#include "zwave/script_events/thermostat_operating_state_value_updated.h"
#include "zwave/script_events/thermostat_setback_value_updated.h"
#include "zwave/script_events/door_lock_state_updated.h"
#include "zwave/script_events/door_lock_configuration_updated.h"
#include "zwave/script_events/learn_event.h"
#include "zwave/script_events/user_code_updated.h"
#include "zwave/script_events/user_codes_checksum_updated.h"
#include "zwave/script_events/user_codes_keypad_mode_updated.h"
#include "zwave/script_events/master_code_updated.h"
#include "zwave/script_events/notification_recieved.h"
#include "zwave/script_events/unknown_notification_recieved.h"
#include "zwave/script_events/alarm_sensor_value_updated.h"
#include "zwave/script_events/schedule_entry_lock_week_day_updated.h"
#include "zwave/script_events/schedule_entry_lock_year_day_updated.h"
#include "zwave/script_events/schedule_entry_lock_daily_repeating_updated.h"
#include "zwave/script_events/schedule_updated.h"
#include "zwave/script_events/schedule_state_updated.h"
#include "zwave/script_events/sound_switch_configuration_updated.h"
#include "zwave/script_events/sound_switch_tone_play_updated.h"
#include "zwave/script_events/include_device_progress.h"
#include "zwave/script_events/learn_show_key.h"
#include "zwave/script_events/learn_progress.h"
#include "zwave/script_events/node_firmware_download_status_event.h"
#include "zwave/script_events/node_firmware_update_status_event.h"
#include "zwave/script_events/rediscover_device_progress.h"
#include "zwave/script_events/barrier_signal_value_updated.h"
#include "zwave/script_events/node_check_status.h"
#include "zwave/script_events/sis_node_updated.h"
#include "zwave/script_events/anti_theft_state_updated.h"
#include "zwave/script_events/vera_migration_finished.h"
#include "zwave/script_events/wake_up_interval_updated.h"
#include "zwave/script_events/simple_av_control_set_received.h"

#include "zwave/resources/zwave_device_info.h"
#include "zwave/anti_theft_helper.h"
#include "zwave/zw_nvm_converter_utils/zw_nvm_converter_utils.h"
#include <znet/converters/schedule_converter.hpp>

#include "service_interfaces/bus_wrapper.h"
#include "service_interfaces/task_loop_timer.h"
#include "common_utils/backup/backup_helper.h"
#include "common_utils/backup/restore_helper.h"
#include "common_utils/lock_helper.h"
#include "common_utils/firmware_info_helper.h"
#include "json_utils/json_utils.h"
#include "json_utils/json_wrapper.h"
#include "json_utils/json_custom_getters.h"

#include "base_utils/md5.h"

#include "jsonrpc/jsonrpc.h"

#include "extensions_manager/remote_interface/em_remote_interface.h"
#include "hardware_manager/remote_interface/hm_remote_interface.h"
#include "backupd/remote_interface/backup_remote_interface.h"
#include "update_manager/resources/um_resources.h"

#include "system_utils/filesystem.h"
#include "common_utils/cpp14.h"
#include "database_client/dbqueryparser.h"
#include "zwave/znet_io_wrapper.h"
#include <znet/znet_node.hpp>
#include <znet/znet_node_control_manager.hpp>
#include <znet/znet_command_class_values_setter.hpp>
#include <znet/znet_all_command_classes.hpp>
#include <znet/enums/thermostat_operating_state.hpp>
#include <znet/converters/thermostat_operating_state_converter.hpp>
#include <znet/converters/thermostat_setback_converter.hpp>
#include <znet/converters/door_lock.hpp>
#include <znet/converters/user_code.hpp>
#include <znet/converters/barrier_operator_signal_converter.hpp>
#include <znet/converters/signed_configuration_parameter_converter.hpp>
#include <znet/converters/mode_converter.hpp>
#include <znet/converters/mode_status_converter.hpp>
#include <znet/converters/color_switch_converter.hpp>
#include <znet/znet_meter.hpp>
#include <znet/schedule.hpp>
#include <znet/optional.hpp>

#include <tiny/include/tiny/tiny_utils.h>
#include <tiny/tiny_command_line_parser.h>
#include <time.h>

#include <cassert>
#include <chrono>
#include <stdlib.h>
#include <sys/stat.h>
#include <bitset>
#include <memory>
#include <base_utils/utils.h>
#include <system_utils/environment.h>

#include <random>

namespace HubPlatform {
namespace Zwave {

const char* const ZwaveAddon::Name = "zwave";
const char* const ZwaveAddon::Version = "1.0";

namespace {
constexpr size_t UartBufferSize = 512;
constexpr uint8_t DefaultNodeFirmwareLoadingStatusStep = 10;
enum OperationStatus { OK = 0, Fail = -1 };

constexpr std::chrono::milliseconds ZnetLoopInterval( 35 );
constexpr std::chrono::milliseconds ZnetLoopIntervalBackup( 20 );
constexpr std::chrono::milliseconds ZnetLoopInitializationInterval( 1000 );

const auto ScheduleIdGetter = JsonCustomGetters::makeStructGetter(
    JsonCustomGetters::makeMemberMapping( &znet_schedule_id_t::user_id, Resources::RPCParams::UserId ),
    JsonCustomGetters::makeMemberMapping( &znet_schedule_id_t::slot_id, Resources::RPCParams::SlotId ) );

class ZNetTimer : public znet::ITimerProvider::Timer, public Services::TaskLoopTimer {
public:
    ZNetTimer( Services::TaskLoop& taskLoop, znet::ITimerProvider::TimerCallback&& callback )
    : Services::TaskLoopTimer( taskLoop, std::move( callback ) ) {
        setPeriodicMode( Services::PostponedTask::PeriodicMode::FixedIdleDuration );
    }

    using TaskLoopTimer::TaskLoopTimer;

    void runPeriodically( znet::ITimerProvider::TimerInterval first,
                          znet::ITimerProvider::TimerInterval interval ) override {
        start( first, interval );
    }

    void runOnce( znet::ITimerProvider::TimerInterval first ) override {
        start( first, false );
    }
};

}  // namespace

void errorObj( JsonObject& answer, const JsonObject& errorObject ) {
    LOGGER_ERROR( cc::error( "Error: " ) + cc::warn( json_get_str( errorObject, "message" ) ) );
    json_add_str( answer, "status", "aborted" );
    setErrorObject( answer, errorObject );
}

const std::string ZwaveAddon::EnvmFileName = "envm.bin";

ZwaveAddon::ZwaveAddon( const std::string& ttyPath, const std::string& znetStoreFilePath )
: ZwaveAddon( Name, ttyPath, znetStoreFilePath, nullptr, nullptr, nullptr, nullptr ) {}

ZwaveAddon::ZwaveAddon( const std::string& name,
                        const std::string& ttyPath,
                        const std::string& znetStoreFilePath,
                        std::unique_ptr< Services::BusWrapper > impl,
                        std::unique_ptr< IDbClient > dbClient,
                        std::unique_ptr< ZWaveStick > zwaveStick,
                        std::unique_ptr< ZNetFactory > znetFactory )
: Addons::Addon( name, std::move( impl ), std::move( dbClient ) )
, m_ttyPath( ttyPath )
, m_znetStoreFilePath( znetStoreFilePath )
, m_znetFactory( std::move( znetFactory ) )
, m_zwaveStick( std::move( zwaveStick ) ) {}

ZwaveAddon::~ZwaveAddon() = default;

CommonUtils::AddonVersion ZwaveAddon::getVersion() const {
    return Addons::Zwave::getVersion();
}

void ZwaveAddon::addZWaveTask( ZwaveAddon::ZWaveTask&& task ) {
    m_zwaveTaskLoop.addTask( std::move( task ) );
}

void ZwaveAddon::setUp() {
    Addon::setUp();

    join( GroupsNames::SYSTEM_GROUP );

    m_uart = std::make_unique< Uart >( m_ttyPath );
    m_store = std::make_unique< ZNetStore >( m_znetStoreFilePath );

    if( !m_zwaveStick ) {
        m_zwaveStick = std::make_unique< ZWaveStick >();
    }

    m_provisioningListProvider =
        std::make_unique< ProvisioningListProvider >( getDBClient(), Resources::DB::TableNames::Provisioning );
    m_classicInclusionSecurityProvider = std::make_unique< ProvisioningListProvider >(
        getDBClient(), Resources::DB::TableNames::ClassicInclusionSecurity );

    if( m_znetFactory ) {
        m_znet =
            m_znetFactory->createZNet( *m_classicInclusionSecurityProvider, *m_provisioningListProvider, *this, *this );
    }
    else {
        ZNetFactory znetFactory( ZNetFactory::ZNetTypeTag< ZNet >{} );
        m_znet =
            znetFactory.createZNet( *m_classicInclusionSecurityProvider, *m_provisioningListProvider, *this, *this );
    }

    m_zwaveAddonCommandsHandler = std::make_unique< ZwaveAddonCommandsHandler >(
        *m_provisioningListProvider, m_znet->getProvisioningListController() );
    m_deviceFirmwareUpdateManager = std::make_unique< DeviceFirmwareUpdateManager >( *this );

    m_nodeManager = std::make_shared< NodeManager >(
        getDBClient(), m_znet->getRequestsManager(), *m_znet, m_znet->getReachableCheckTimeoutSettings() );

    m_nodeStatisticsCollector = std::make_shared< NodeStatisticsCollector >();

    Service::Service::setReporter(
        std::make_unique< ZwaveReporter >( getName(), m_nodeManager, m_nodeStatisticsCollector ) );

    addTask( [this]() {
        auto flagFilePath =
            Environment::getPersistentFolder() + "/" + UpdateManager::Resources::ResetZwaveRequiredFileName;
        if( filesystem::isFileExist( flagFilePath ) ) {
            reset();
            filesystem::removeFile( flagFilePath.c_str() );
        }
    } );

    m_zwaveThread = std::thread( &TaskLoop::run, &m_zwaveTaskLoop );

    if( m_isTestMode ) {
        LOGGER_INFO( "Zwave addon setup done !\n" );
        return;
    }

    m_zwaveLoopTask = m_zwaveTaskLoop.createPostponedTask( [this]( Services::PostponedTask& ) {
        while( isRunning() && !m_znet->onLoop() ) {
            LOGGER_ERROR_RAW( "Failed to set up ZWave addon" );

            HardwareManager::RemoteInterface hwRemoteInterface( *this );
            hwRemoteInterface.setZwaveState( HardwareManager::HubState::Zwave::Off );

            for( const auto timePoint = std::chrono::steady_clock::now() + ZnetLoopInitializationInterval;
                 isRunning() && std::chrono::steady_clock::now() < timePoint; ) {
                std::this_thread::sleep_for( ZnetLoopInterval );
            }
        }

        if( isRunning() && !m_wasZwaveIndicatorInitialized ) {
            m_wasZwaveIndicatorInitialized = true;
            LOGGER_INFO_RAW( "ZWave stack initialized successfully" );

            HardwareManager::RemoteInterface hwRemoteInterface( *this );
            hwRemoteInterface.setZwaveState( HardwareManager::HubState::Zwave::On );
        }
    } );
    m_zwaveLoopTask->setPeriodicMode( Services::PostponedTask::PeriodicMode::FixedIdleDuration );
    m_zwaveLoopTask->start( Interval( 0 ), ZnetLoopInterval );

    std::unique_lock< std::mutex > lock( m_mutex );
    if( !m_initCondVar.wait_for(
            lock, std::chrono::seconds{ 10 }, [this]() -> bool { return m_wasZwaveInitialized; } ) ) {
        LOGGER_ERROR( "Zwave addon setup timeout !\n" );
    }
    else {
        LOGGER_INFO( "Zwave addon setup done !\n" );
    }
}

void ZwaveAddon::tearDown() {
    std::cout << "ZWave addon terminated..." << std::endl << std::flush;

    m_deviceFirmwareUpdateManager.reset();

    m_zwaveTaskLoop.addTask( [this]() { m_zwaveTaskLoop.stop(); } );
    m_zwaveThread.join();

    HardwareManager::RemoteInterface hwRemoteInterface( *this );
    hwRemoteInterface.setZwaveState( HardwareManager::HubState::Zwave::Off );

    Addon::tearDown();
}

JsonObject ZwaveAddon::handleRequestFromBus( const JsonObject& request, MessageSubClasses requestType ) {
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
        for( auto& tableName : Resources::DB::BackupTableNames ) {
            if( !helper.backupDatabase( tableName ) ) {
                return helper.getResponse();
            }
        }
        if( !helper.backupFile( m_znetStoreFilePath ) )
            return helper.getResponse();

        auto error = doChipBackupSync( helper.getBackupDataDirectory(), true );
        if( !error.empty() )
            return CommonUtils::BackupHelper::createFailedResponse( error );

        return CommonUtils::BackupHelper::createSuccessResponse();
    }
    case ADDON_MESSAGE:
        LOGGER_TRACE( cc::trace( "start" ) );
        return m_zwaveAddonCommandsHandler->handleAddonMessage( request );

    case ZWAVE_GET_INFO:
        return getZwaveInfo();

    default:
        return Service::handleRequestFromBus( request, requestType );
    };

    return JSON_EMPTY_OBJ;
}

bool ZwaveAddon::handleAsyncRequestFromBus( const JsonObject& request,
                                            MessageSubClasses requestType,
                                            const std::string& uuid,
                                            const std::string& sender ) {
    switch( requestType ) {
    case BACKUP_RESTORE_VERA: {
        m_veraBackupRequestInfo = std::make_unique< AsyncRequestInfo >( request, uuid, sender );

        if( m_veraMigrationHelper.get() != nullptr ) {
            rediscoverNextVeraDevice();
        }
        else {
            auto directoryIt = request.find( "directory" );
            if( directoryIt != request.end() ) {
                onVeraBackupMigrationRequested( *directoryIt );
            }
            else {
                onVeraBackupStatus( "failed", "Failed to read directory path" );
            }
        }
        break;
    }
    default:
        return Service::handleAsyncRequestFromBus( request, requestType, uuid, sender );
    }
    return true;
}

void ZwaveAddon::handleMessageFromBus( const Services::Message& message, const std::string& sender ) {
    if( message.msgClass != EVENT ) {
        Services::Service::handleMessageFromBus( message, sender );
        return;
    }

    switch( message.msgSubClass ) {
    case SERVICE_STARTED: {
        const std::string serviceId = json_get_str( message.data, "id" );
        if( serviceId == GroupsNames::HARDWARE_MANAGER ) {
            // Hardware manager service has just started, send actual led state.
            updateLeds();
        }
        break;
    }
    case HUB_SYS_TIME_UPDATED:
        m_znet->updateTimeSettings();
        break;
    }
}

void ZwaveAddon::startChipBackup( const std::string& folder, bool backupCreation ) {
    const auto chipFileName = folder + "/" + "zwave_chip_backup.bin";
    if( backupCreation ) {
        auto stream = std::make_shared< std::ofstream >( chipFileName, std::ofstream::binary );
        if( !stream->is_open() ) {
            m_backupHelper->setFinished( "Cannot open backup file" + chipFileName );
        }
        else {
            m_znet->createChipBackup( std::make_unique< FileDestination >( stream ) );
        }
    }
    else {
        auto stream = std::make_shared< std::ifstream >( chipFileName, std::ifstream::binary );
        if( !stream->is_open() ) {
            m_backupHelper->setFinished( "Cannot open file " + chipFileName + " for chip backup creation" );
        }
        else {
            m_znet->restoreChipBackup( std::make_unique< FileSource >( stream ) );
        }
    }
}

std::string ZwaveAddon::doChipBackupSync( const std::string& folder, bool backupCreation ) {
    assert( !m_backupHelper );

    const auto isAddonRunning = isRunning();

    auto deleter = [isAddonRunning]( ZwaveAddon* addon ) {
        if( !isAddonRunning ) {
            addon->m_zwaveLoopTask.reset();
            addon->m_zwaveTaskLoop.stop();
            addon->m_zwaveThread.join();

            addon->m_nodeManager.reset();
            addon->m_znet.reset();
            addon->m_classicInclusionSecurityProvider.reset();
            addon->m_provisioningListProvider.reset();
            addon->m_zwaveStick.reset();
            addon->m_store.reset();
            addon->m_uart.reset();
        }

        addon->m_backupHelper.reset();
    };

    std::unique_ptr< ZwaveAddon, decltype( deleter ) > guard( this, deleter );

    m_backupHelper = std::make_unique< BackupHelper >();

    if( !isAddonRunning ) {
        m_uart = std::make_unique< Uart >( m_ttyPath );
        m_store = std::make_unique< ZNetStore >( m_znetStoreFilePath );

        m_zwaveStick = std::make_unique< ZWaveStick >();

        m_provisioningListProvider =
            std::make_unique< ProvisioningListProvider >( getDBClient(), Resources::DB::TableNames::Provisioning );
        m_classicInclusionSecurityProvider = std::make_unique< ProvisioningListProvider >(
            getDBClient(), Resources::DB::TableNames::ClassicInclusionSecurity );

        ZNetFactory znetFactory( ZNetFactory::ZNetTypeTag< ZNet >{} );
        m_znet =
            znetFactory.createZNet( *m_classicInclusionSecurityProvider, *m_provisioningListProvider, *this, *this );

        m_nodeManager = std::make_unique< NodeManager >(
            getDBClient(), m_znet->getRequestsManager(), *m_znet, m_znet->getReachableCheckTimeoutSettings() );

        m_zwaveThread = std::thread( &TaskLoop::run, &m_zwaveTaskLoop );
        m_zwaveLoopTask = m_zwaveTaskLoop.createPostponedTask( [=]( Services::PostponedTask& ) {
            while( !m_znet->onLoop() ) {
                m_backupHelper->onStickInitFailed();
                std::this_thread::sleep_for( std::chrono::milliseconds( 1000 ) );
            }
        } );
    }

    m_zwaveLoopTask->start( Interval(), ZnetLoopIntervalBackup );
    addZWaveTask( [=]() { startChipBackup( folder, backupCreation ); } );

    m_backupHelper->waitForFinishing();
    m_zwaveLoopTask->start( Interval(), ZnetLoopInterval );

    LOGGER_INFO( "finish do chip backup" );

    return m_backupHelper->getErrorDescription();
}

void ZwaveAddon::onVeraBackupMigrationRequested( const std::string& folder ) {
    // Init helper
    m_veraMigrationHelper =
        std::make_unique< VeraMigrationHelper >( folder + "/etc/cmh/keys", folder + "/etc/cmh/user_data.json.lzo" );

    // Check Vera locale same as Ezlo
    std::string veraLocale;
    {
        std::ifstream stream( folder + "/etc/cmh/zwave_locale" );
        stream >> veraLocale;
    }

    if( tiny::tools::to_upper( veraLocale ) != Resources::DeviceInfo::getZwaveChipLocale() ) {
        onVeraBackupStatus( "failed", "Wrong zwave region " + veraLocale );
        return;
    }

    // Convert NVM image and collect nodes
    std::map< znet_node_id_t, znet::SecurityMode::Class > nodes;

    auto jsonImageHandler = [&nodes]( const Json& jsonImage ) {
        const auto& nodeTable = jsonImage[ "zwController" ][ "nodeTable" ];
        for( const auto& nodeInfo : nodeTable ) {
            auto nodeId = json_get_uint( nodeInfo, "nodeId" );
            if( nodeId != 1 ) {
                nodes.emplace( nodeId, znet::SecurityMode::Class::None );
            }
        }
    };

    if( !Zwave::NvmConverterUtils::convertVeraImageToActualFormat(
            folder + "/etc/cmh/dongle.6.1.dump.0", folder + "/zwave_chip_backup.bin", jsonImageHandler ) ) {
        onVeraBackupStatus( "failed", "Cannot convert old chip image to new one" );
        return;
    }

    // Write NVM image to chip
    const auto chipBackupResult = doChipBackupSync( folder, false );

    if( !chipBackupResult.empty() ) {
        onVeraBackupStatus( "failed", "Cannot restore chip backup" );
        return;
    }

    // Clear storages
    m_nodeManager->reset();
    znetStoreReset();
    addTask( [this]() { emit( AllNodesRemoved{} ); } );

    // Init chip and ZNet
    bool isInitialized = initZwaveSync();
    if( !isInitialized ) {
        onVeraBackupStatus( "failed", "Cannot init znet after restore" );
        return;
    }

    // Check nodes security
    auto nodesInfo = m_veraMigrationHelper->readVeraNodesInformation();
    const auto& devices = nodesInfo[ "devices" ];
    for( const auto& deviceInfo : devices ) {
        auto nodeIdString = json_get_str( deviceInfo, "altid" );
        if( nodeIdString.empty() || !std::all_of( nodeIdString.begin(), nodeIdString.end(), []( unsigned char c ) {
                return std::isdigit( c );
            } ) ) {
            continue;
        }

        znet_node_id_t nodeId = std::stoi( nodeIdString );
        if( nodes.find( nodeId ) != nodes.end() ) {
            auto const& states = deviceInfo[ "states" ];
            for( const auto& state : states ) {
                if( state[ "service" ] == "urn:micasaverde-com:serviceId:ZWaveDevice1" &&
                    state[ "variable" ] == "SecurityFailed" && state[ "value" ] == "0" ) {
                    nodes[ nodeId ] = znet::SecurityMode::Class::S0;
                }
            }
        }
    }

    // Register nodes
    for( const auto& nodeInfo : nodes ) {
        auto nodeId = nodeInfo.first;

        m_nodeManager->registerNode( nodeId );
        auto registeredNode = m_nodeManager->findNode( nodeId );
        registeredNode->setNeedExtendedRediscover( true );
        registeredNode->setSecurity( nodeInfo.second );
        m_nodeManager->update( nodeId );

        m_veraMigrationHelper->addNodeToRediscover( nodeId );
    }

    // Start nodes rediscover
    addZWaveTask( [this]() { onVeraBackupStatus( "in_progress", "" ); } );
}

void ZwaveAddon::rediscoverNextVeraDevice() {
    if( m_veraMigrationHelper ) {
        if( m_veraMigrationHelper->isRediscoverProcessCompleted() ) {
            onVeraBackupStatus( "finished", "" );
        }
        else {
            auto nextNodeId = m_veraMigrationHelper->getNextNodeToRediscover();
            if( !nextNodeId ) {
                assert( !"nextNodeId should exists" );
            }
            m_znet->interviewNode( *nextNodeId );
        }
    }
}

bool ZwaveAddon::initZwaveSync() {
    m_wasZwaveInitialized = false;
    m_znet->initZnet();

    std::unique_lock< std::mutex > lock( m_mutex );
    if( !m_initCondVar.wait_for(
            lock, std::chrono::seconds{ 10 }, [this]() -> bool { return m_wasZwaveInitialized; } ) ) {
        LOGGER_ERROR( "Zwave addon setup timeout !\n" );
        return false;
    }
    else {
        LOGGER_INFO( "Zwave addon setup done !\n" );
        return true;
    }
}

void ZwaveAddon::onVeraBackupStatus( const std::string& status, const std::string& errorDescription ) {
    addTask( [=] {
        if( status == "finished" && m_veraMigrationHelper ) {
            auto nodesInfo = m_veraMigrationHelper->readVeraNodesInformation();
            emit( VeraMigrationFinished{ std::move( nodesInfo ) } );
        }

        if( !( status == "in_progress" && m_veraMigrationHelper ) ) {
            m_veraMigrationHelper.reset();
            LOGGER_INFO( cc::info( "Migration from Vera to Ezlo controller finished, status: " + status +
                                   ", desc: " + errorDescription ) );
        }

        auto responseTask = createPostponedTask( [this, status, errorDescription]( Services::PostponedTask& ) {
            if( !m_veraBackupRequestInfo ) {
                LOGGER_ERROR_RAW( cc::error( "No Vera backup request info" ) );
                return;
            }
            auto response = JSON( "status" << status );
            if( !errorDescription.empty() ) {
                response[ "error_description" ] = errorDescription;
            }
            sendAsyncResponse(
                m_veraBackupRequestInfo->m_sender, std::move( response ), m_veraBackupRequestInfo->m_uuid );

            m_veraBackupRequestInfo.reset();
        } );

        responseTask->start( status == "finished" ? std::chrono::seconds( 5 ) : std::chrono::seconds( 0 ), false );
    } );
}

int ZwaveAddon::restoreBackup( const std::string& tempBackupFolder ) {
    CommonUtils::RestoreHelper helper( getDBClientSafe(), tempBackupFolder );
    for( auto& tableName : Resources::DB::BackupTableNames ) {
        if( !helper.restoreDatabase( tableName ) ) {
            return helper.getReturnCode();
        }
    }
    if( !helper.restoreFile( m_znetStoreFilePath ) )
        return helper.getReturnCode();

    auto error = doChipBackupSync( helper.getBackupDataDirectory(), false );
    if( !error.empty() )
        helper.setReturnCode( 255 );

    return helper.getReturnCode();
}

// Zwave public interface -----------------------------------------------------
void ZwaveAddon::startInclude( const Json& params ) {
    m_deviceSettingPath = json_get_str( params, Resources::RPCParams::DeviceSettingFilePath );
    addZWaveTask( [this]() { m_znet->startInclude(); } );
}

void ZwaveAddon::startExclude() {
    addZWaveTask( [this]() { m_znet->startExclude( 0 ); } );
}

void ZwaveAddon::stopInclude() {
    addZWaveTask( [this]() { m_znet->stopInclude(); } );
}

void ZwaveAddon::stopExclude() {
    addZWaveTask( [this]() { m_znet->stopExclude(); } );
}

void ZwaveAddon::startLearn() {
    addZWaveTask( [this]() { m_znet->startLearn(); } );
}

void ZwaveAddon::stopLearn() {
    addZWaveTask( [this]() { m_znet->stopLearn(); } );
}

void ZwaveAddon::stopSecurityInclude( const Json& /*params*/ ) {
    addZWaveTask( [this]() { m_znet->stopSecurityInclude(); } );
}

void ZwaveAddon::startExclude( const uint8_t nodeId ) {
    addZWaveTask( [this, nodeId]() { m_znet->startExclude( nodeId ); } );
}

void ZwaveAddon::reset() {
    addZWaveTask( [this]() {
        m_znet->resetZwaveModule( true );  // TODO: Add reset parameter
    } );
}

void ZwaveAddon::setAuthenticationMode( const Json& params ) {
    auto jaModes = JSON_EMPTY_ARR;
    json_get_arr( params, Resources::RPCParams::Modes, jaModes );

    LOGGER_INFO( "modes: " + json_write( jaModes ) );

    auto keyList = AuthModeUtils::modesToKeyList( jaModes );

    addZWaveTask( [this, keyList]() { m_znet->s2SelectKeys( keyList ); } );
}

void ZwaveAddon::setDeviceSpecificKey( const Json& params ) {
    const uint16_t key = params[ Resources::RPCParams::Key ];

    LOGGER_INFO( "key: " + std::to_string( key ) );

    addZWaveTask( [this, key]() {
        const size_t buffSize = sizeof( znet_S2_pub_key_t ) + 2 * sizeof( ( (znet_S2_pub_key_t*) nullptr )->key[ 0 ] );
        uint8_t buff[ buffSize ] = {};

        auto pubKey = (znet_S2_pub_key_t*) &buff;
        pubKey->size = 2;

        pubKey->key[ 0 ] = ( key >> 8 ) & 0xFF;
        pubKey->key[ 1 ] = key & 0xFF;

        m_znet->setPublicKey( pubKey );
    } );
}

void ZwaveAddon::setUserCode( const Json& params ) {
    LOGGER_TRACE( json_write( params ) );

    const uint8_t nodeId = params[ Resources::RPCParams::NodeId ];
    const uint8_t channelId = params[ Resources::RPCParams::ChannelId ];
    const uint8_t userId = params[ Resources::RPCParams::UserId ];
    const uint8_t userIdStatus = params[ Resources::RPCParams::UserIdStatus ];
    const std::string userCode = params[ Resources::RPCParams::UserCode ];

    addZWaveTask(
        [=]() { m_znet->getNodeControlManager().setUserCode( nodeId, channelId, userId, userIdStatus, userCode ); } );
}

void ZwaveAddon::setSilentSensors( const Json& params ) {
    LOGGER_TRACE( json_write( params ) );

    const uint8_t nodeId = params[ Resources::RPCParams::NodeId ];
    const uint8_t channelId = params[ Resources::RPCParams::ChannelId ];
    const auto disablingMode =
        static_cast< znet::SensorsSoundDisablingMode >( uint8_t( params[ Resources::RPCParams::SoundDisablingMode ] ) );
    const uint16_t delay = params[ Resources::RPCParams::Delay ];
    const auto sensorsMask = tiny::tools::base64::decode( params[ Resources::RPCParams::NotificationsMask ] );

    addZWaveTask( [=]() {
        m_znet->getNodeControlManager().setSilentSensors( nodeId, channelId, disablingMode, delay, sensorsMask );
    } );
}

void ZwaveAddon::requestSensorAlarmValue( const Json& params ) {
    LOGGER_TRACE( json_write( params ) );

    const uint8_t nodeId = params[ Resources::RPCParams::NodeId ];
    const uint8_t channelId = params[ Resources::RPCParams::ChannelId ];
    const uint8_t sensorType = params[ Resources::RPCParams::SensorType ];

    addZWaveTask( [=]() { m_znet->getNodeControlManager().requestSensorAlarmValue( nodeId, channelId, sensorType ); } );
}

void ZwaveAddon::addNodeToAssociationGroup( const Json& params ) {
    LOGGER_TRACE( json_write( params ) );

    const uint8_t nodeId = params[ Resources::RPCParams::NodeId ];
    const uint8_t groupId = params[ Resources::RPCParams::GroupId ];
    const uint8_t destinationNodeId = params[ Resources::RPCParams::DestinationNode ];
    const uint8_t destinationEndpoint = params[ Resources::RPCParams::DestinationEndpoint ];
    const uint8_t channelId = params[ Resources::RPCParams::ChannelId ];

    addZWaveTask( [channelId, groupId, destinationNodeId, nodeId, this, destinationEndpoint]() {
        m_znet->getNodeControlManager().addNodeToAssociationGroup(
            nodeId, channelId, groupId, destinationNodeId, destinationEndpoint );
    } );
}

void ZwaveAddon::removeNodeFromAssociationGroup( const Json& params ) {
    LOGGER_TRACE( json_write( params ) );

    const uint8_t nodeId = params[ Resources::RPCParams::NodeId ];
    const uint8_t groupId = params[ Resources::RPCParams::GroupId ];
    const uint8_t destinationNodeId = params[ Resources::RPCParams::DestinationNode ];
    const uint8_t destinationEndpoint = params[ Resources::RPCParams::DestinationEndpoint ];
    const uint8_t channelId = params[ Resources::RPCParams::ChannelId ];

    addZWaveTask( [channelId, groupId, destinationNodeId, nodeId, this, destinationEndpoint]() {
        m_znet->getNodeControlManager().removeNodeFromAssociationGroup(
            nodeId, channelId, groupId, destinationNodeId, destinationEndpoint );
    } );
}

template < typename Type, typename MethodSet, typename MethodRemove, typename Getter >
void ZwaveAddon::setScheduleEntryLockValue( const Json& params,
                                            MethodSet methodSet,
                                            MethodRemove methodRemove,
                                            Getter&& getter,
                                            const char* name ) {
    const uint8_t nodeId = params[ Resources::RPCParams::NodeId ];
    const uint8_t channelId = params[ Resources::RPCParams::ChannelId ];
    Type schedule{};
    if( !ScheduleIdGetter( params, schedule.schedule_id ) ) {
        LOGGER_ERROR( std::string( "ZwaveAddon::" ) + name + ": invalid schedule id " + params.dump() );
        return;
    }

    auto valueIt = params.find( Resources::RPCParams::Value );
    if( valueIt != params.end() ) {
        if( !getter( *valueIt, schedule ) ) {
            LOGGER_ERROR( std::string( "ZwaveAddon::" ) + name + ": invalid schedule value " + valueIt->dump() );
            return;
        }

        addZWaveTask( [this, methodSet, nodeId, channelId, schedule]() {
            ( m_znet->getNodeControlManager().*methodSet )( nodeId, channelId, schedule );
        } );
    }
    else {
        addZWaveTask( [this, methodRemove, nodeId, channelId, schedule]() {
            ( m_znet->getNodeControlManager().*methodRemove )( nodeId, channelId, schedule.schedule_id );
        } );
    }
}

template < typename Method >
void ZwaveAddon::requestScheduleEntryLockValue( const Json& params, Method method ) {
    const uint8_t nodeId = params[ Resources::RPCParams::NodeId ];
    const uint8_t channelId = params[ Resources::RPCParams::ChannelId ];
    znet_schedule_id_t scheduleId{};

    if( !ScheduleIdGetter( params, scheduleId ) ) {
        LOGGER_ERROR( std::string( "ZwaveAddon" ) + ": invalid schedule id " + params.dump() );
        return;
    }

    addZWaveTask( [this, method, nodeId, channelId, scheduleId]() {
        ( m_znet->getNodeControlManager().*method )( nodeId, channelId, scheduleId );
    } );
}

void ZwaveAddon::setWeekDayEntryLockSchedule( const Json& params ) {
    LOGGER_TRACE( json_write( params ) );

    using namespace JsonCustomGetters;
    using Schedule = znet_week_day_schedule_t;

    static const auto getter =
        makeStructGetter( makeMemberMapping( &Schedule::day_of_week, Resources::RPCParams::WeekDay ),
                          makeMemberMapping( &Schedule::start_hour, Resources::RPCParams::StartHour ),
                          makeMemberMapping( &Schedule::start_min, Resources::RPCParams::StartMin ),
                          makeMemberMapping( &Schedule::stop_hour, Resources::RPCParams::StopHour ),
                          makeMemberMapping( &Schedule::stop_min, Resources::RPCParams::StopMin ) );

    setScheduleEntryLockValue< Schedule >( params,
                                           &znet::NodeControlManager::setWeekDayEntryLockSchedule,
                                           &znet::NodeControlManager::removeWeekDayEntryLockSchedule,
                                           getter,
                                           "setWeekDayEntryLockSchedule" );
}

void ZwaveAddon::setYearDayEntryLockSchedule( const Json& params ) {
    LOGGER_TRACE( json_write( params ) );

    using namespace JsonCustomGetters;
    using namespace Resources;
    using Schedule = znet_year_day_schedule_t;

    struct {
        using Type = uint8_t;

        GettingResult operator()( Json::const_iterator it, Type& scheduleYear, bool ) const {
            const auto* year = it.value().get< const Json::number_integer_t* >();
            if( !year || century != *year / 100 )
                return GettingResult::Error::Default;

            scheduleYear = *year % 100;

            return {};
        }

        uint8_t century;
    } yearGetter{ static_cast< uint8_t >( getLocalDateTime().date.year / 100 ) };

    const auto getter = makeStructGetter(
        makeMemberMapping( &Schedule::start_year, makeFieldGetter( RPCParams::StartYear, yearGetter ) ),
        makeMemberMapping( &Schedule::start_month, RPCParams::StartMonth ),
        makeMemberMapping( &Schedule::start_day, RPCParams::StartDay ),
        makeMemberMapping( &Schedule::start_hour, RPCParams::StartHour ),
        makeMemberMapping( &Schedule::start_minute, RPCParams::StartMin ),
        makeMemberMapping( &Schedule::stop_year, makeFieldGetter( RPCParams::StopYear, yearGetter ) ),
        makeMemberMapping( &Schedule::stop_month, RPCParams::StopMonth ),
        makeMemberMapping( &Schedule::stop_day, RPCParams::StopDay ),
        makeMemberMapping( &Schedule::stop_hour, RPCParams::StopHour ),
        makeMemberMapping( &Schedule::stop_minute, RPCParams::StopMin ) );

    setScheduleEntryLockValue< Schedule >( params,
                                           &znet::NodeControlManager::setYearDayEntryLockSchedule,
                                           &znet::NodeControlManager::removeYearDayEntryLockSchedule,
                                           getter,
                                           "setYearDayEntryLockSchedule" );
}

void ZwaveAddon::setDailyRepeatingEntryLockSchedule( const Json& params ) {
    LOGGER_TRACE( json_write( params ) );

    using namespace JsonCustomGetters;
    using Schedule = znet_daily_repeating_schedule_t;

    static const auto getter =
        makeStructGetter( makeMemberMapping( &Schedule::week_day_bitmask, Resources::RPCParams::WeekDays ),
                          makeMemberMapping( &Schedule::start_hour, Resources::RPCParams::StartHour ),
                          makeMemberMapping( &Schedule::start_min, Resources::RPCParams::StartMin ),
                          makeMemberMapping( &Schedule::duration_hour, Resources::RPCParams::DurationHour ),
                          makeMemberMapping( &Schedule::duration_min, Resources::RPCParams::DurationMin ) );

    setScheduleEntryLockValue< Schedule >( params,
                                           &znet::NodeControlManager::setDailyRepeatingEntryLockSchedule,
                                           &znet::NodeControlManager::removeDailyRepeatingEntryLockSchedule,
                                           getter,
                                           "setDailyRepeatingEntryLockSchedule" );
}

void ZwaveAddon::setEntryLockScheduleEnable( const Json& params ) {
    LOGGER_TRACE( json_write( params ) );

    const uint8_t nodeId = params[ Resources::RPCParams::NodeId ];
    const uint8_t channelId = params[ Resources::RPCParams::ChannelId ];
    const uint8_t userId = params[ Resources::RPCParams::UserId ];
    const bool enable = params[ Resources::RPCParams::Enable ];

    addZWaveTask(
        [=]() { m_znet->getNodeControlManager().setSchedulesEntryLockEnable( nodeId, channelId, userId, enable ); } );
}

template < typename T1 >
struct OptionalUintGetter {
    using Type = znet::utils::optional< T1 >;

    JsonCustomGetters::GettingResult operator()( Json::const_iterator it, Type& result, bool ) const {
        const auto* ptr = it.value().get< const Json::number_integer_t* >();
        if( !ptr ) {
            return JsonCustomGetters::GettingResult::Error::Default;
        }
        result = *ptr;
        return JsonCustomGetters::GettingResult::Error::None;
    }
};

template < typename T2, typename T3 >
struct EnumGetter {
    using Type = T2;
    using ConverterPtr = T3 ( * )( const char* );

    EnumGetter( const ConverterPtr ptr )
    : m_fPtr( ptr ) {}

    JsonCustomGetters::GettingResult operator()( Json::const_iterator it, Type& result, bool ) const {
        if( !m_fPtr ) {
            return JsonCustomGetters::GettingResult::Error::Default;
        }
        if( it.value().is_string() ) {
            auto typeStr = it.value().get< std::string >();
            if( typeStr.size() == 0 ) {
                return JsonCustomGetters::GettingResult::Error::Default;
            }

            result = ( *m_fPtr )( typeStr.c_str() );

            return JsonCustomGetters::GettingResult::Error::None;
        }
        return JsonCustomGetters::GettingResult::Error::Default;
    }

private:
    ConverterPtr m_fPtr;
};

void ZwaveAddon::setSchedule( const Json& params ) {
    LOGGER_TRACE( "ZwaveAddon::setSchedule: " + params.dump() );

    const uint8_t nodeId = params[ Resources::RPCParams::NodeId ];
    const uint8_t channelId = params[ Resources::RPCParams::ChannelId ];

    znet::Schedule schedule{};

    using namespace Resources::RPCParams;

    uint8_t scheduleId = params[ Schedule::ScheduleId ];
    const auto scheduleIdBlockIt = params.find( Schedule::ScheduleIdBlock );
    znet::ScheduleIdBlockOpt idBlockOpt =
        scheduleIdBlockIt == params.end() ? znet::ScheduleIdBlockOpt() : scheduleIdBlockIt->get< uint8_t >();

    auto valueIt = params.find( Resources::RPCParams::Value );
    if( valueIt != params.end() ) {
        using namespace JsonCustomGetters;

        struct {
            using Type = std::array< znet::ScheduleWeeksDay, 7 >;
            GettingResult operator()( Json::const_iterator it, Type& result, bool ) const {
                auto end = it->end();
                it = it->begin();

                int counter = 0;
                for( ; it != end; ++it ) {
                    const auto& weekday = it.value().get< std::string >();
                    if( weekday.size() > 0 ) {
                        result[ counter++ ] = znet::schedule_weeks_day_converter::stringToWeeksDay( weekday.c_str() );
                    }

                    if( counter >= 7 ) {
                        GettingResult::Error::Default;
                    }
                }

                return GettingResult::Error::None;
            }
        } weekdaysGetter;

        struct {
            using Type = znet::utils::optional< bool >;
            GettingResult operator()( Json::const_iterator it, Type& result, bool ) const {
                const auto* ptr = it.value().get< const Json::boolean_t* >();
                if( !ptr ) {
                    return GettingResult::Error::Default;
                }

                result = *ptr;
                return GettingResult::Error::None;
            }
        } optionalBoolGetter;

        EnumGetter< znet::utils::optional< znet::ScheduleRecurrenceMode >, znet::ScheduleRecurrenceMode >
            recurrenceModeGetter( &znet::schedule_recurrence_mode_converter::stringToMode );
        EnumGetter< znet::ScheduleDurationType, znet::ScheduleDurationType > durationTypeGetter(
            &znet::schedule_duration_type_converter::stringToDurationType );

        const auto valueGetter = makeStructGetter(
            makeMemberMapping(
                &znet::Schedule::yearday,
                makeFieldGetter(
                    Schedule::Yearday,
                    makeStructGetter(
                        makeMemberMapping( &znet::Yearday::startYearOpt,
                                           makeFieldGetter( Schedule::StartYear,
                                                            OptionalUintGetter< uint16_t >(),
                                                            OptionalMemberPolicy{ false } ) ),
                        makeMemberMapping( &znet::Yearday::startMonthOpt,
                                           makeFieldGetter( Schedule::StartMonth,
                                                            OptionalUintGetter< uint8_t >(),
                                                            OptionalMemberPolicy{ false } ) ),
                        makeMemberMapping( &znet::Yearday::startDayOfMonth, Schedule::StartDayOfMonth ) ) ) ),
            makeMemberMapping(
                &znet::Schedule::hourminute,
                makeFieldGetter(
                    Schedule::Hourandminute,
                    makeStructGetter( makeMemberMapping( &znet::Hourminute::startHourOpt,
                                                         makeFieldGetter( Schedule::StartHour,
                                                                          OptionalUintGetter< uint8_t >(),
                                                                          OptionalMemberPolicy{ false } ) ),
                                      makeMemberMapping( &znet::Hourminute::startMinuteOpt,
                                                         makeFieldGetter( Schedule::StartMinute,
                                                                          OptionalUintGetter< uint8_t >(),
                                                                          OptionalMemberPolicy{ false } ) ) ) ) ),
            makeMemberMapping( &znet::Schedule::weekdays, makeFieldGetter( Schedule::Weekdays, weekdaysGetter ) ),
            makeMemberMapping(
                &znet::Schedule::options,
                makeFieldGetter(
                    Schedule::Options,
                    makeStructGetter(
                        makeMemberMapping( &znet::Options::recurrenceOffsetOpt,
                                           makeFieldGetter( Schedule::RecurrenceOffset,
                                                            OptionalUintGetter< uint8_t >(),
                                                            OptionalMemberPolicy{ false } ) ),
                        makeMemberMapping( &znet::Options::recurrenceModeOpt,
                                           makeFieldGetter( Schedule::RecurrenceMode,
                                                            recurrenceModeGetter,
                                                            OptionalMemberPolicy{ false } ) ),
                        makeMemberMapping(
                            &znet::Options::relativeOpt,
                            makeFieldGetter( Schedule::Relative, optionalBoolGetter, OptionalMemberPolicy{ false } ) ),
                        makeMemberMapping( &znet::Options::durationType,
                                           makeFieldGetter( Schedule::DurationType, durationTypeGetter ) ),
                        makeMemberMapping(
                            &znet::Options::duration,
                            makeFieldGetter(
                                Schedule::Duration, OptionalUintGetter< uint16_t >(), OptionalMemberPolicy{ false } ) ),
                        makeMemberMapping( &znet::Options::reportToFollowOpt,
                                           makeFieldGetter( Schedule::ReportToFollow,
                                                            OptionalUintGetter< uint8_t >(),
                                                            OptionalMemberPolicy{ false } ) ) ) ) ),
            makeMemberMapping(
                &znet::Schedule::commands,
                makeFieldGetter( Schedule::Commands, makeDefaultGetter< std::vector< std::vector< uint8_t > > >() ) ) );

        znet::Schedule schedule{};

        if( !valueGetter( *valueIt, schedule ) ) {
            LOGGER_ERROR( " schedule's value is invalid " +
                          ( valueIt != params.end() ? ( *valueIt ).dump() : std::string( " (value is empty)" ) ) );
            return;
        }

        addZWaveTask( [this, nodeId, channelId, scheduleId, idBlockOpt, schedule]() {
            m_znet->getNodeControlManager().setSchedule( nodeId, channelId, scheduleId, idBlockOpt, schedule );
        } );
    }
    else {
        addZWaveTask( [this, nodeId, channelId, scheduleId, idBlockOpt]() {
            m_znet->getNodeControlManager().removeSchedule( nodeId, channelId, scheduleId, idBlockOpt );
        } );
    }
}

void ZwaveAddon::requestSchedule( const Json& params ) {
    LOGGER_TRACE( "ZwaveAddon::requestSchedule: " + params.dump() );
    const uint8_t nodeId = params[ Resources::RPCParams::NodeId ];
    const uint8_t channelId = params[ Resources::RPCParams::ChannelId ];
    const uint8_t scheduleId = params[ Resources::RPCParams::Schedule::ScheduleId ];

    const auto blockIdIt = params.find( Resources::RPCParams::Schedule::ScheduleIdBlock );
    const auto activeIdCtlIt = params.find( Resources::RPCParams::Schedule::AidRoControl );

    const znet::ScheduleIdBlockOpt blockIdOpt =
        blockIdIt == params.end() ? znet::ScheduleIdBlockOpt() : blockIdIt->get< uint8_t >();

    const znet::ActiveIdCtlOpt activeIdCtlOpt =
        activeIdCtlIt == params.end() ? znet::ActiveIdCtlOpt() : activeIdCtlIt->get< bool >();

    addZWaveTask( [this, nodeId, channelId, scheduleId, blockIdOpt, activeIdCtlOpt]() {
        m_znet->getNodeControlManager().requestSchedule( nodeId, channelId, scheduleId, blockIdOpt, activeIdCtlOpt );
    } );
}

void ZwaveAddon::setScheduleState( const Json& params ) {
    LOGGER_TRACE( "ZwaveAddon::setScheduleState: " + params.dump() );
    const uint8_t nodeId = params[ Resources::RPCParams::NodeId ];
    const uint8_t channelId = params[ Resources::RPCParams::ChannelId ];
    const uint8_t scheduleId = params[ Resources::RPCParams::Schedule::ScheduleId ];
    const auto blockIdIt = params.find( Resources::RPCParams::Schedule::ScheduleIdBlock );
    const uint8_t scheduleState = params[ Resources::RPCParams::Schedule::ScheduleState ];

    const znet::ScheduleIdBlockOpt blockIdOpt =
        blockIdIt == params.end() ? znet::ScheduleIdBlockOpt() : blockIdIt->get< uint8_t >();

    addZWaveTask( [this, nodeId, channelId, scheduleId, blockIdOpt, scheduleState]() {
        m_znet->getNodeControlManager().setScheduleState( nodeId, channelId, scheduleId, blockIdOpt, scheduleState );
    } );
}

void ZwaveAddon::requestScheduleState( const Json& params ) {
    LOGGER_TRACE( "ZwaveAddon::requestScheduleState: " + params.dump() );
    const uint8_t nodeId = params[ Resources::RPCParams::NodeId ];
    const uint8_t channelId = params[ Resources::RPCParams::ChannelId ];
    const auto blockIdIt = params.find( Resources::RPCParams::Schedule::ScheduleIdBlock );

    const znet::ScheduleIdBlockOpt blockIdOpt =
        blockIdIt == params.end() ? znet::ScheduleIdBlockOpt() : blockIdIt->get< uint8_t >();

    addZWaveTask( [this, nodeId, channelId, blockIdOpt]() {
        m_znet->getNodeControlManager().requestScheduleState( nodeId, channelId, blockIdOpt );
    } );
}

void ZwaveAddon::requestWeekDayEntryLockSchedule( const Json& params ) {
    LOGGER_TRACE( json_write( params ) );

    requestScheduleEntryLockValue( params, &znet::NodeControlManager::requestWeekDayEntryLockSchedule );
}

void ZwaveAddon::requestYearDayEntryLockSchedule( const Json& params ) {
    LOGGER_TRACE( json_write( params ) );

    requestScheduleEntryLockValue( params, &znet::NodeControlManager::requestYearDayEntryLockSchedule );
}

void ZwaveAddon::requestDailyRepeatingEntryLockSchedule( const Json& params ) {
    LOGGER_TRACE( json_write( params ) );

    requestScheduleEntryLockValue( params, &znet::NodeControlManager::requestDailyRepeatingEntryLockSchedule );
}

void ZwaveAddon::reconstructNetwork( const Json& ) {
    addZWaveTask( [this]() { m_znet->reconstructNetwork(); } );
}

void ZwaveAddon::requestUserCodesChecksum( const Json& params ) {
    const uint8_t nodeId = params[ Resources::RPCParams::NodeId ];
    const uint8_t channelId = params[ Resources::RPCParams::ChannelId ];

    addZWaveTask( [=]() { m_znet->getNodeControlManager().requestUserCodesChecksum( nodeId, channelId ); } );
}

void ZwaveAddon::requestMasterCode( const Json& params ) {
    const uint8_t nodeId = params[ Resources::RPCParams::NodeId ];
    const uint8_t channelId = params[ Resources::RPCParams::ChannelId ];

    addZWaveTask( [=]() { m_znet->getNodeControlManager().requestMasterCode( nodeId, channelId ); } );
}

void ZwaveAddon::setMasterCode( const Json& params ) {
    const uint8_t nodeId = params[ Resources::RPCParams::NodeId ];
    const uint8_t channelId = params[ Resources::RPCParams::ChannelId ];
    const std::string code = params[ Resources::RPCParams::MasterCode ];

    addZWaveTask( [=]() { m_znet->getNodeControlManager().setMasterCode( nodeId, channelId, code ); } );
}

void ZwaveAddon::requestUserCodesKeypadMode( const Json& params ) {
    const uint8_t nodeId = params[ Resources::RPCParams::NodeId ];
    const uint8_t channelId = params[ Resources::RPCParams::ChannelId ];

    addZWaveTask( [=]() { m_znet->getNodeControlManager().requestUserCodesKeypadMode( nodeId, channelId ); } );
}

void ZwaveAddon::setUserCodesKeypadMode( const Json& params ) {
    const uint8_t nodeId = params[ Resources::RPCParams::NodeId ];
    const uint8_t channelId = params[ Resources::RPCParams::ChannelId ];
    const uint8_t keypadMode = params[ Resources::RPCParams::Mode ];

    addZWaveTask( [=]() {
        m_znet->getNodeControlManager().setUserCodesKeypadMode(
            nodeId, channelId, static_cast< znet::UserCode::KeypadMode >( keypadMode ) );
    } );
}

void ZwaveAddon::requestUserCode( const Json& params ) {
    LOGGER_TRACE( json_write( params ) );

    const uint8_t nodeId = params[ Resources::RPCParams::NodeId ];
    const uint8_t channelId = params[ Resources::RPCParams::ChannelId ];
    const uint8_t userId = params[ Resources::RPCParams::UserId ];

    addZWaveTask( [=]() { m_znet->getNodeControlManager().requestUserCode( nodeId, channelId, userId ); } );
}

void ZwaveAddon::requestUserCodes( const Json& params ) {
    LOGGER_TRACE( json_write( params ) );

    const uint8_t nodeId = params[ Resources::RPCParams::NodeId ];
    const uint8_t channelId = params[ Resources::RPCParams::ChannelId ];

    addZWaveTask( [=]() { m_znet->getNodeControlManager().requestUserCodes( nodeId, channelId ); } );
}

void ZwaveAddon::requestAssociationGroup( const Json& params ) {
    LOGGER_TRACE( json_write( params ) );

    const uint8_t nodeId = params[ Resources::RPCParams::NodeId ];
    const uint8_t channelId = params[ Resources::RPCParams::ChannelId ];
    const uint8_t groupId = params[ Resources::RPCParams::GroupId ];

    addZWaveTask( [=]() { m_znet->getNodeControlManager().requestAssociationGroup( nodeId, channelId, groupId ); } );
}

void ZwaveAddon::setValue( const Json& params ) {
    LOGGER_TRACE( json_write( params ) );

    const uint8_t nodeId = params[ Resources::RPCParams::NodeId ];
    const uint8_t channelId = params[ Resources::RPCParams::ChannelId ];
    const uint8_t classId = params[ Resources::RPCParams::ClassId ];
    const uint8_t value = json_get_uint( params, Resources::RPCParams::Value );

    const auto durationIt = params.find( Resources::RPCParams::Duration );
    const znet::SecondsOpt duration =
        durationIt == params.end() ? znet::SecondsOpt() : znet::Seconds( durationIt->get< uint32_t >() );

    if( duration.has_value() ) {
        addZWaveTask( [this, nodeId, channelId, classId, value, duration]() {
            m_znet->getNodeControlManager().setValue(
                nodeId, channelId, classId, znet::ValueWithDuration{ .value = value, .seconds = duration } );
        } );
    }
    else {
        addZWaveTask( [this, nodeId, channelId, classId, value]() {
            m_znet->getNodeControlManager().setValue( nodeId, channelId, classId, value );
        } );
    }
}

void ZwaveAddon::setValueMulticast( const Json& params ) {
    LOGGER_TRACE( "ZwaveAddon::setValueMulticast: " + params.dump() );

    std::vector< uint8_t > nodeIds = params[ Resources::RPCParams::NodeId ];
    const uint8_t channelId = params[ Resources::RPCParams::ChannelId ];
    const uint8_t ccId = params[ Resources::RPCParams::ClassId ];
    const uint8_t value = json_get_uint( params, Resources::RPCParams::Value );
    const auto modeIt = params.find( Resources::RPCParams::Mode );
    const znet::MulticastMode mode = modeIt == params.end() ?
                                         znet::MulticastMode::Default :
                                         static_cast< znet::MulticastMode >( modeIt->get< uint8_t >() );

    addZWaveTask( [=]() {
        m_znet->getNodeControlManager().setValueMulticast( std::move( nodeIds ), channelId, ccId, value, mode );
    } );
}

void ZwaveAddon::setTokenValue( const Json& params ) {
    LOGGER_TRACE( "setTokenValue: " + json_write( params ) );

    const uint8_t nodeId = params[ Resources::RPCParams::NodeId ];
    const uint8_t channelId = params[ Resources::RPCParams::ChannelId ];
    const uint8_t classId = params[ Resources::RPCParams::ClassId ];
    const std::string value = json_get_str( params, Resources::RPCParams::Value );

    addZWaveTask( [=]() { m_znet->getNodeControlManager().setTokenValue( nodeId, channelId, classId, value ); } );
}

void ZwaveAddon::setTokenValueMulticast( const Json& params ) {
    LOGGER_TRACE( "ZwaveAddon::setTokenValueMulticast: " + params.dump() );

    std::vector< uint8_t > nodeIds = params[ Resources::RPCParams::NodeId ];
    const uint8_t channelId = params[ Resources::RPCParams::ChannelId ];
    const uint8_t ccId = params[ Resources::RPCParams::ClassId ];
    const std::string value = json_get_str( params, Resources::RPCParams::Value );
    const auto modeIt = params.find( Resources::RPCParams::Mode );
    const znet::MulticastMode mode = modeIt == params.end() ?
                                         znet::MulticastMode::Default :
                                         static_cast< znet::MulticastMode >( modeIt->get< uint8_t >() );

    addZWaveTask( [=]() {
        m_znet->getNodeControlManager().setValueMulticast( std::move( nodeIds ), channelId, ccId, value, mode );
    } );
}

void ZwaveAddon::setRgbValue( const Json& params ) {
    LOGGER_TRACE( json_write( params ) );

    const uint8_t nodeId = params[ Resources::RPCParams::NodeId ];
    const uint8_t channelId = params[ Resources::RPCParams::ChannelId ];
    const uint8_t ccId = params[ Resources::RPCParams::ClassId ];

    const auto& joColors = params[ Resources::RPCParams::Value ];
    std::vector< znet_cl_component_t > components;
    components.reserve( joColors.size() );
    for( auto it = joColors.begin(); it != joColors.end(); ++it ) {
        if( auto componentId = znet::color_switch_converter::strToColorComponentId( it.key().c_str() ) ) {
            components.push_back( { static_cast< uint8_t >( *componentId ), it.value() } );
        }
    }

    addZWaveTask( [=]() { m_znet->getNodeControlManager().setRgbValue( nodeId, channelId, ccId, components ); } );
}

void ZwaveAddon::setRgbValueMulticast( const Json& params ) {
    LOGGER_TRACE( "ZwaveAddon::setRgbValueMulticast: " + params.dump() );

    std::vector< uint8_t > nodeIds = params[ Resources::RPCParams::NodeId ];
    const uint8_t channelId = params[ Resources::RPCParams::ChannelId ];
    const uint8_t ccId = params[ Resources::RPCParams::ClassId ];

    const auto& joColors = params[ Resources::RPCParams::Value ];
    std::vector< znet_cl_component_t > components;
    components.reserve( joColors.size() );
    for( auto it = joColors.begin(); it != joColors.end(); ++it ) {
        if( auto componentId = znet::color_switch_converter::strToColorComponentId( it.key().c_str() ) ) {
            components.push_back( { static_cast< uint8_t >( *componentId ), it.value() } );
        }
    }

    const auto modeIt = params.find( Resources::RPCParams::Mode );
    const znet::MulticastMode mode = modeIt == params.end() ?
                                         znet::MulticastMode::Default :
                                         static_cast< znet::MulticastMode >( modeIt->get< uint8_t >() );

    addZWaveTask( [=]() {
        m_znet->getNodeControlManager().setValueMulticast( std::move( nodeIds ), channelId, ccId, components, mode );
    } );
}

void ZwaveAddon::setSoundSwitchConfiguration( const Json& params ) {
    LOGGER_TRACE( json_write( params ) );

    const uint8_t nodeId = params[ Resources::RPCParams::NodeId ];
    const uint8_t channelId = params[ Resources::RPCParams::ChannelId ];
    const JsonObject& configuration = params[ Resources::RPCParams::Value ];
    const uint8_t volume = configuration[ Resources::ScriptEvent::Param::Sound::Volume ];
    const uint8_t defaultToneId = configuration[ Resources::ScriptEvent::Param::Sound::DefaultToneId ];

    addZWaveTask( [=]() {
        m_znet->getNodeControlManager().setSoundSwitchConfiguration( nodeId, channelId, volume, defaultToneId );
    } );
}

void ZwaveAddon::requestSoundSwitchConfiguration( const Json& params ) {
    LOGGER_TRACE( json_write( params ) );

    const uint8_t nodeId = params[ Resources::RPCParams::NodeId ];
    const uint8_t channelId = params[ Resources::RPCParams::ChannelId ];

    addZWaveTask( [=]() { m_znet->getNodeControlManager().requestSoundSwitchConfiguration( nodeId, channelId ); } );
}

void ZwaveAddon::setSoundSwitchTonePlay( const Json& params ) {
    LOGGER_TRACE( json_write( params ) );

    const uint8_t nodeId = params[ Resources::RPCParams::NodeId ];
    const uint8_t channelId = params[ Resources::RPCParams::ChannelId ];
    const JsonObject& configuration = params[ Resources::RPCParams::Value ];
    const uint8_t toneId = configuration[ Resources::ScriptEvent::Param::Sound::ToneId ];
    const uint8_t volume = configuration[ Resources::ScriptEvent::Param::Sound::Volume ];

    addZWaveTask(
        [=]() { m_znet->getNodeControlManager().setSoundSwitchTonePlay( nodeId, channelId, volume, toneId ); } );
}

void ZwaveAddon::requestSoundSwitchTonePlay( const Json& params ) {
    LOGGER_TRACE( json_write( params ) );

    const uint8_t nodeId = params[ Resources::RPCParams::NodeId ];
    const uint8_t channelId = params[ Resources::RPCParams::ChannelId ];

    addZWaveTask( [=]() { m_znet->getNodeControlManager().requestSoundSwitchTonePlay( nodeId, channelId ); } );
}

void ZwaveAddon::setSetpointValue( const Json& params ) {
    LOGGER_TRACE( json_write( params ) );

    const uint8_t nodeId = params[ Resources::RPCParams::NodeId ];
    const uint8_t channelId = params[ Resources::RPCParams::ChannelId ];
    const uint8_t setpointType = params[ Resources::RPCParams::SetpointType ];
    const uint8_t scale = params[ Resources::RPCParams::Scale ];
    const float value = params[ Resources::RPCParams::Value ];

    addZWaveTask(
        [=]() { m_znet->getNodeControlManager().setSetpointValue( nodeId, channelId, setpointType, scale, value ); } );
}

void ZwaveAddon::setConfigurationValue( const Json& params ) {
    LOGGER_TRACE( json_write( params ) );

    const uint8_t nodeId = params[ Resources::RPCParams::NodeId ];
    const uint16_t paramNumber = params[ Resources::RPCParams::ParamNumber ];
    const uint8_t paramSize = params[ Resources::RPCParams::ParamSize ];
    const int64_t value = params[ Resources::RPCParams::Value ];

    addZWaveTask(
        [=]() { m_znet->getNodeControlManager().setConfigurationValue( nodeId, paramNumber, paramSize, value ); } );
}

void ZwaveAddon::addConfiguration( const Json& params ) {
    LOGGER_TRACE( json_write( params ) );

    const uint8_t nodeId = params[ Resources::RPCParams::NodeId ];
    const uint8_t paramNumber = params[ Resources::RPCParams::ParamNumber ];
    const uint8_t paramSize = params[ Resources::RPCParams::ParamSize ];

    addZWaveTask( [=]() { m_znet->getNodeControlManager().addConfiguration( nodeId, paramNumber, paramSize ); } );
}

void ZwaveAddon::removeConfiguration( const Json& params ) {
    LOGGER_TRACE( json_write( params ) );

    const uint8_t nodeId = params[ Resources::RPCParams::NodeId ];
    const uint8_t paramNumber = params[ Resources::RPCParams::ParamNumber ];

    addZWaveTask( [=]() { m_znet->getNodeControlManager().removeConfiguration( nodeId, paramNumber ); } );
}

void ZwaveAddon::resetConfigurationValue( const Json& params ) {
    LOGGER_TRACE( json_write( params ) );

    const uint8_t nodeId = params[ Resources::RPCParams::NodeId ];
    const uint16_t paramNumber = params[ Resources::RPCParams::ParamNumber ];

    addZWaveTask( [=]() { m_znet->getNodeControlManager().resetConfigurationValue( nodeId, paramNumber ); } );
}

void ZwaveAddon::resetAllConfigurationValues( const Json& params ) {
    LOGGER_TRACE( json_write( params ) );

    const uint8_t nodeId = params[ Resources::RPCParams::NodeId ];

    addZWaveTask( [=]() { m_znet->getNodeControlManager().resetAllConfigurationValues( nodeId ); } );
}

void ZwaveAddon::requestConfigurationValue( const Json& params ) {
    LOGGER_TRACE( json_write( params ) );

    const uint8_t nodeId = params[ Resources::RPCParams::NodeId ];
    const uint16_t paramNumber = params[ Resources::RPCParams::ParamNumber ];

    addZWaveTask( [=]() { m_znet->getNodeControlManager().requestConfigurationValue( nodeId, paramNumber ); } );
}

void ZwaveAddon::switchMultilevelLevelChange( const Json& params ) {
    LOGGER_TRACE( json_write( params ) );

    const uint8_t nodeId = params[ Resources::RPCParams::NodeId ];
    const uint8_t channelId = params[ Resources::RPCParams::ChannelId ];
    const uint8_t command = params[ Resources::RPCParams::Command ];
    const auto durationIt = params.find( Resources::RPCParams::Duration );
    const znet::SecondsOpt duration =
        durationIt == params.end() ? znet::SecondsOpt() : znet::Seconds( durationIt->get< uint32_t >() );

    addZWaveTask( [=]() {
        m_znet->getNodeControlManager().switchMultilevelLevelChange(
            nodeId, channelId, static_cast< znet::ValueChangeCommand >( command ), duration );
    } );
}

void ZwaveAddon::colorSwitchLevelChange( const Json& params ) {
    LOGGER_TRACE( json_write( params ) );

    const uint8_t nodeId = params[ Resources::RPCParams::NodeId ];
    const uint8_t channelId = params[ Resources::RPCParams::ChannelId ];
    const uint8_t command = params[ Resources::RPCParams::Command ];
    const uint8_t componentId = params[ Resources::RPCParams::ComponentId ];
    const auto durationIt = params.find( Resources::RPCParams::Duration );
    const znet::SecondsOpt duration =
        durationIt == params.end() ? znet::SecondsOpt() : znet::Seconds( durationIt->get< uint32_t >() );

    addZWaveTask( [=]() {
        m_znet->getNodeControlManager().colorSwitchLevelChange( nodeId,
                                                                channelId,
                                                                static_cast< znet::ValueChangeCommand >( command ),
                                                                static_cast< znet::ColorComponentID >( componentId ),
                                                                duration );
    } );
}

void ZwaveAddon::requestValue( const Json& params ) {
    LOGGER_TRACE( json_write( params ) );

    const uint8_t nodeId = params[ Resources::RPCParams::NodeId ];
    const uint8_t channelId = params[ Resources::RPCParams::ChannelId ];
    const uint8_t classId = params[ Resources::RPCParams::ClassId ];

    addZWaveTask( [=]() { m_znet->getNodeControlManager().requestValue( nodeId, channelId, classId ); } );
}

void ZwaveAddon::requestSetpointValue( const Json& params ) {
    LOGGER_TRACE( json_write( params ) );

    const uint8_t nodeId = params[ Resources::RPCParams::NodeId ];
    const uint8_t channelId = params[ Resources::RPCParams::ChannelId ];
    const uint8_t setpointType = params[ Resources::RPCParams::SetpointType ];

    addZWaveTask( [=]() { m_znet->getNodeControlManager().requestSetpointValue( nodeId, channelId, setpointType ); } );
}

void ZwaveAddon::requestAllNodesValues( const Json& ) {
    addZWaveTask( [this] { m_znet->requestAllNodesValues(); } );
}

void ZwaveAddon::requestNodeValues( const Json& params ) {
    LOGGER_TRACE( json_write( params ) );

    const uint8_t nodeId = params[ Resources::RPCParams::NodeId ];
    const auto requestDelay = std::chrono::milliseconds{ params[ Resources::RPCParams::RequestDelay ] };
    const bool force = params[ Resources::RPCParams::Force ];
    const size_t unsetPinCodesCountToStop = params[ Resources::RPCParams::UnsetPinCodesCountToStop ];

    addZWaveTask( [=]() {
        m_znet->getNodeControlManager().requestNodeValues( nodeId, requestDelay, force, unsetPinCodesCountToStop );
    } );
}

void ZwaveAddon::handleLegacyMethodCall( const JsonObject& json, const std::string& sender ) {
    const auto method = json_get_str( json[ "method" ] );
    if( method == Resources::RPC::StartInclude ) {
        startInclude( json[ "params" ] );
    }
    else if( method == Resources::RPC::StartExclude ) {
        const auto& params = json[ "params" ];
        params.empty() ? startExclude() : startExclude( json_get_uint( params, Resources::RPCParams::NodeId ) );
    }
    else if( method == Resources::RPC::SetAuthenticationMode ) {
        setAuthenticationMode( json[ "params" ] );
    }
    else if( method == Resources::RPC::SetDeviceSpecificKey ) {
        setDeviceSpecificKey( json[ "params" ] );
    }
    else if( method == Resources::RPC::SetValue ) {
        setValue( json[ "params" ] );
    }
    else if( method == Resources::RPC::SetRgbValue ) {
        setRgbValue( json[ "params" ] );
    }
    else if( method == Resources::RPC::SetSetpointValue ) {
        setSetpointValue( json[ "params" ] );
    }
    else if( method == Resources::RPC::RequestValue ) {
        requestValue( json[ "params" ] );
    }
    else if( method == Resources::RPC::RequestSetpointValue ) {
        requestSetpointValue( json[ "params" ] );
    }
    else if( method == Resources::RPC::Reset ) {
        reset();
    }
    else if( method == Resources::RPC::StopInclude ) {
        stopInclude();
    }
    else if( method == Resources::RPC::StopExclude ) {
        stopExclude();
    }
    else if( method == Resources::RPC::StartLearn ) {
        startLearn();
    }
    else if( method == Resources::RPC::StopLearn ) {
        stopLearn();
    }
    else {
        Addon::handleLegacyMethodCall( json, sender );
    }
}

void ZwaveAddon::handleMethodCall( const JsonObject& json,
                                   MessageSubClasses requestType,
                                   const std::string& /*sender*/ ) {
    using MethodHandler = void ( ZwaveAddon::* )( const JsonObject& json );
    using MethodHandlersMap = std::unordered_map< MessageSubClasses, MethodHandler, MessageSubClassesHash >;

    static const MethodHandlersMap methodHandlers = {
        { ZWAVE_SWITCH_MULTILEVEL_LEVEL_CHANGE, &ZwaveAddon::switchMultilevelLevelChange },
        { ZWAVE_COLOR_SWITCH_LEVEL_CHANGE, &ZwaveAddon::colorSwitchLevelChange },
        { ZWAVE_SET_USER_CODE, &ZwaveAddon::setUserCode },
        { ZWAVE_REQUEST_USER_CODE, &ZwaveAddon::requestUserCode },
        { ZWAVE_REQUEST_USER_CODES, &ZwaveAddon::requestUserCodes },
        { ZWAVE_REQUEST_USER_CODES_CHECKSUM, &ZwaveAddon::requestUserCodesChecksum },
        { ZWAVE_REQUEST_MASTER_CODE, &ZwaveAddon::requestMasterCode },
        { ZWAVE_SET_MASTER_CODE, &ZwaveAddon::setMasterCode },
        { ZWAVE_REQUEST_USER_CODES_KEYPAD_MODE, &ZwaveAddon::requestUserCodesKeypadMode },
        { ZWAVE_SET_USER_CODES_KEYPAD_MODE, &ZwaveAddon::setUserCodesKeypadMode },
        { ZWAVE_REQUEST_ASSOCIATION_GROUP, &ZwaveAddon::requestAssociationGroup },
        { ZWAVE_REQUEST_NODE_VALUES, &ZwaveAddon::requestNodeValues },
        { ZWAVE_REQUEST_ALL_NODES_VALUES, &ZwaveAddon::requestAllNodesValues },
        { ZWAVE_SET_CONFIGURATION_VALUE, &ZwaveAddon::setConfigurationValue },
        { ZWAVE_ADD_CONFIGURATION, &ZwaveAddon::addConfiguration },
        { ZWAVE_REMOVE_CONFIGURATION, &ZwaveAddon::removeConfiguration },
        { ZWAVE_RESET_CONFIGURATION_VALUE, &ZwaveAddon::resetConfigurationValue },
        { ZWAVE_RESET_ALL_CONFIGURATION_VALUES, &ZwaveAddon::resetAllConfigurationValues },
        { ZWAVE_REQUEST_CONFIGURATION_VALUE, &ZwaveAddon::requestConfigurationValue },
        { ZWAVE_SET_TOKEN_VALUE, &ZwaveAddon::setTokenValue },
        { ZWAVE_SET_TOKEN_VALUE_MULTICAST, &ZwaveAddon::setTokenValueMulticast },
        { ZWAVE_SET_RGB_VALUE_MULTICAST, &ZwaveAddon::setRgbValueMulticast },
        { ZWAVE_SET_SOUND_SWITCH_CONFIGURATION, &ZwaveAddon::setSoundSwitchConfiguration },
        { ZWAVE_REQUEST_SOUND_SWITCH_CONFIGURATION, &ZwaveAddon::requestSoundSwitchConfiguration },
        { ZWAVE_SET_SOUND_SWITCH_TONE_PLAY, &ZwaveAddon::setSoundSwitchTonePlay },
        { ZWAVE_REQUEST_SOUND_SWITCH_TONE_PLAY, &ZwaveAddon::requestSoundSwitchTonePlay },
        { ZWAVE_SET_VALUE_MULTICAST, &ZwaveAddon::setValueMulticast },
        { ZWAVE_SET_SILENT_SENSORS, &ZwaveAddon::setSilentSensors },
        { ZWAVE_REQUEST_SENSOR_ALARM_VALUE, &ZwaveAddon::requestSensorAlarmValue },
        { ZWAVE_ADD_NODE_TO_ASSOCIATION_GROUP, &ZwaveAddon::addNodeToAssociationGroup },
        { ZWAVE_REMOVE_NODE_FROM_ASSOCIATION_GROUP, &ZwaveAddon::removeNodeFromAssociationGroup },
        { ZWAVE_STOP_SECURITY_INCLUDE, &ZwaveAddon::stopSecurityInclude },
        { ZWAVE_REQUEST_WEEK_DAY_ENTRY_LOCK_SCHEDULE, &ZwaveAddon::requestWeekDayEntryLockSchedule },
        { ZWAVE_REQUEST_YEAR_DAY_ENTRY_LOCK_SCHEDULE, &ZwaveAddon::requestYearDayEntryLockSchedule },
        { ZWAVE_REQUEST_DAILY_REPEATING_ENTRY_LOCK_SCHEDULE, &ZwaveAddon::requestDailyRepeatingEntryLockSchedule },
        { ZWAVE_SET_WEEK_DAY_ENTRY_LOCK_SCHEDULE, &ZwaveAddon::setWeekDayEntryLockSchedule },
        { ZWAVE_SET_YEAR_DAY_ENTRY_LOCK_SCHEDULE, &ZwaveAddon::setYearDayEntryLockSchedule },
        { ZWAVE_SET_DAILY_REPEATING_ENTRY_LOCK_SCHEDULE, &ZwaveAddon::setDailyRepeatingEntryLockSchedule },
        { ZWAVE_SET_ENTRY_LOCK_SCHEDULE_ENABLE, &ZwaveAddon::setEntryLockScheduleEnable },
        { ZWAVE_SET_SCHEDULE, &ZwaveAddon::setSchedule },
        { ZWAVE_REQUEST_SCHEDULE, &ZwaveAddon::requestSchedule },
        { ZWAVE_SET_SCHEDULE_STATE, &ZwaveAddon::setScheduleState },
        { ZWAVE_REQUEST_SCHEDULE_STATE, &ZwaveAddon::requestScheduleState },
        { ZWAVE_RECONSTRUCT_NETWORK, &ZwaveAddon::reconstructNetwork },
        { ZWAVE_SET_DOOR_LOCK_MODE, &ZwaveAddon::setDoorLockMode },
        { ZWAVE_REQUEST_DOOR_LOCK_STATE, &ZwaveAddon::requestDoorLockState },
        { ZWAVE_SET_DOOR_LOCK_CONFIGURATION, &ZwaveAddon::setDoorLockConfiguration },
        { ZWAVE_REQUEST_DOOR_LOCK_CONFIGURATION, &ZwaveAddon::requestDoorLockConfiguration },
        { ZWAVE_RESET_METER, &ZwaveAddon::resetMeter },
        { ZWAVE_REQUEST_METER, &ZwaveAddon::requestMeter },
        { ZWAVE_UPDATE_NODES_FIRMWARE, &ZwaveAddon::updateNodesFirmware },
        { ZWAVE_DOWNLOAD_NODE_FIRMWARE, &ZwaveAddon::downloadNodeFirmware },
        { ZWAVE_START_REDISCOVER, &ZwaveAddon::startRediscover },
        { ZWAVE_SEND_IDENTIFY, &ZwaveAddon::sendIdentify },
        { ZWAVE_REQUEST_NOTIFICATION_VALUE, &ZwaveAddon::requestNotificationValue },
        { ZWAVE_REQUEST_ALARM_V1_VALUE, &ZwaveAddon::requestAlarmV1Value },
        { ZWAVE_SET_BARRIER_SIGNAL_VALUE, &ZwaveAddon::setBarrierSignalValue },
        { ZWAVE_REQUEST_BARRIER_SIGNAL_VALUE, &ZwaveAddon::requestBarrierSignalValue },
        { ZWAVE_SET_THERMOSTAT_SETBACK_VALUE, &ZwaveAddon::setThermostatSetbackValue },
        { ZWAVE_SET_DEVICES_POLL_INTERVALS, &ZwaveAddon::setDevicesPollIntervals },
        { ZWAVE_SET_UNRESPONSIVE_DEVICE_IGNORE_INTERVAL, &ZwaveAddon::setUnresponsiveDeviceIgnoreInterval },
        { ZWAVE_DEFAULT_DEVICE_CONFIGURATIONS_FILE, &ZwaveAddon::setDefaultDeviceConfigurationsFile },
        { ZWAVE_CHECK_REACHABLE_STATUS, &ZwaveAddon::checkReachableStatus },
        { ZWAVE_CHECK_NODE_VALUES, &ZwaveAddon::checkNodeValues },
        { ZWAVE_LOCK_ANTI_THEFT, &ZwaveAddon::lockAntiTheft },
        { ZWAVE_LOCK_ANTI_THEFT_DEFAULT, &ZwaveAddon::lockAntiTheftDefault },
        { ZWAVE_UNLOCK_ANTI_THEFT, &ZwaveAddon::unlockAntiTheft },
        { ZWAVE_UNLOCK_ANTI_THEFT_DEFAULT, &ZwaveAddon::unlockAntiTheftDefault },
        { ZWAVE_ADD_DEVICE_SPECIFIC_CONFIGURATION, &ZwaveAddon::addDeviceSpecificConfiguration },
        { ZWAVE_ADD_DEVICE_SPECIFIC_ASSOCIATION, &ZwaveAddon::addDeviceSpecificAssociation },
        { ZWAVE_CLEAR_DEVICE_SPECIFIC, &ZwaveAddon::clearDeviceSpecific },
        { ZWAVE_SET_THERMOSTAT_MODE, &ZwaveAddon::setThermostatMode },
        { ZWAVE_SYNCHRONIZE_DEVICE_TIME, &ZwaveAddon::synchronizeDeviceTime },
        { ZWAVE_SET_WAKE_UP_INTERVAL, &ZwaveAddon::setWakeUpInterval }
    };

    const auto it = methodHandlers.find( requestType );
    if( it != methodHandlers.end() ) {
        const auto handler = it->second;
        ( this->*handler )( json );
    }
}

void ZwaveAddon::defineDaemonParameters( tiny::command_line::parser& parser ) {
    parser.on( "test_mode", "ZwaveAddon\nSet test mode", [this]() { m_isTestMode = true; } );
    parser.on( "vera_backup", "ZwaveAddon\nSet vera backup mode", [this]() { m_isVeraBackupMode = true; } );
}

void ZwaveAddon::updateLeds() {
    LastZwaveState zwaveState;
    m_lastZwaveState.get( zwaveState );

    HardwareManager::RemoteInterface hwRemoteInterface( *this );

    if( zwaveState.cmd == znet::ZNet::Mode::IncludeStarting || zwaveState.cmd == znet::ZNet::Mode::ExcludeStarting ) {
        if( zwaveState.cmdStatus == znet::ModeStatus::Invoked || zwaveState.cmdStatus == znet::ModeStatus::Started ) {
            hwRemoteInterface.setZwaveState( zwaveState.cmd == znet::ZNet::Mode::IncludeStarting ?
                                                 HardwareManager::HubState::Zwave::Include :
                                                 HardwareManager::HubState::Zwave::Exclude );
            return;
        }
        else if( zwaveState.cmdStatus == znet::ModeStatus::Failed ||
                 zwaveState.cmdStatus == znet::ModeStatus::Timeout ||
                 zwaveState.cmdStatus == znet::ModeStatus::Finished ) {
            hwRemoteInterface.setZwaveState( HardwareManager::HubState::Zwave::On );
            return;
        }
    }

    if( zwaveState.cmd == znet::ZNet::Mode::IncludeStopping || zwaveState.cmd == znet::ZNet::Mode::ExcludeStopping ) {
        if( zwaveState.cmdStatus == znet::ModeStatus::Finished ) {
            hwRemoteInterface.setZwaveState( HardwareManager::HubState::Zwave::On );
            return;
        }
    }

    hwRemoteInterface.setZwaveState( m_wasZwaveIndicatorInitialized ? HardwareManager::HubState::Zwave::On :
                                                                      HardwareManager::HubState::Zwave::Off );
}

// DeviceFirmwareUpdateManager::Delegate

bool ZwaveAddon::startFirmwareUpdate( const DeviceFirmwareUpdateManager::Device& device,
                                      const DeviceFirmwareUpdateManager::Target& target,
                                      const DeviceFirmwareUpdateManager::Path& destination,
                                      std::function< void( bool ) > resultCallback ) {
    std::shared_ptr< std::ifstream > stream = std::make_shared< std::ifstream >( destination, std::ifstream::binary );
    if( !stream->is_open() ) {
        return false;
    }

    addZWaveTask( [=]() {
        m_znet->updateNodeTargetFirmware( static_cast< znet_node_id_t >( device ),
                                          static_cast< uint8_t >( target ),
                                          std::make_unique< FileSource >( std::move( stream ) ) );
    } );

    return true;
}

void ZwaveAddon::onFirmwareDownloadStatus( const DeviceFirmwareUpdateManager::ExtendedLink& link,
                                           bool isOk,
                                           uint8_t progress ) {
    auto truncateLink = []( const std::string& name ) {
        const auto pos = name.rfind( '/' );
        return name.substr( pos == std::string::npos ? 0 : ( pos + 1 ) );
    };
    NodeFirmwareDownloadStatusEvent event( truncateLink( link.image ),
                                           truncateLink( link.checksum ),
                                           znet::utils::progressToStringStatus( isOk, progress ),
                                           progress );
    addTask( [this, event] { emit( event ); } );
}

void ZwaveAddon::addFirmwareUpdateTask( std::function< void() > task ) {
    addTask( std::move( task ) );
}

void ZwaveAddon::onFirmwareUpdateFinished( const DeviceFirmwareUpdateManager::Device& device,
                                           const DeviceFirmwareUpdateManager::TargetsUpdateResults& targetsResults ) {
    const auto allFailed =
        std::none_of( targetsResults.begin(),
                      targetsResults.end(),
                      []( const DeviceFirmwareUpdateManager::TargetsUpdateResults::value_type& targetToResult ) {
                          return targetToResult.second == DeviceFirmwareUpdateManager::TargetUpdateResult::UpdateDone;
                      } );
    NodeFirmwareUpdateStatusEvent event( static_cast< uint8_t >( device ), {}, allFailed ? "failed" : "finished" );
    addTask( [this, event] { emit( event ); } );
}

void ZwaveAddon::onFirmwareUpdateStarted( const DeviceFirmwareUpdateManager::Device& device ) {
    NodeFirmwareUpdateStatusEvent event( static_cast< uint8_t >( device ), {}, "started" );
    addTask( [this, event] { emit( event ); } );
}

// IZNet interface ------------------------------------------------

bool ZwaveAddon::uartInit() {
    return m_uart->init();
}

void ZwaveAddon::uartClose() {
    m_uart->deinit();
}

int ZwaveAddon::znetUartWrite( const void* data, size_t size, size_t* retSize ) {
    if( !data ) {
        if( !retSize ) {
            LOGGER_ERROR( cc::error( "UART write: invalid args!" ) );
            return Fail;
        }

        *retSize = UartBufferSize;
        return OK;
    }

    if( !size ) {
        LOGGER_ERROR( cc::error( "UART write: invalid args!" ) );
        return Fail;
    }

    const auto writeLen = m_uart->write( data, size );
    if( !m_uart->isIoOk( writeLen ) ) {
        LOGGER_ERROR( cc::error( "UART write: failed!" ) );
        return Fail;
    }

    if( retSize ) {
        *retSize = writeLen;
    }

    return OK;
}

int ZwaveAddon::znetUartRead( void* data, size_t size, size_t* retSize ) {
    if( !retSize ) {
        LOGGER_ERROR( cc::error( "UART read: invalid args!" ) );
        return Fail;
    }

    int bufferSize = 0;
    if( m_uart->getInputBufferBitsCount( bufferSize ) < 0 || !bufferSize ) {
        *retSize = 0;
        return Fail;
    }
    *retSize = bufferSize;

    if( data ) {
        auto readLen = m_uart->read( data, size );
        if( !m_uart->isIoOk( readLen ) ) {
            LOGGER_ERROR( cc::error( "UART read: failed!" ) );
            return Fail;
        }

        *retSize = readLen;
    }
    return OK;
}

int ZwaveAddon::znetStoreSave( size_t offset, const void* data, size_t size ) {
    return m_store->save( offset, data, size ) ? OK : Fail;
}

int ZwaveAddon::znetStoreLoad( size_t offset, void* data, size_t size ) {
    return m_store->load( offset, data, size ) ? OK : Fail;
}

int ZwaveAddon::znetStoreReset() {
    return m_store->reset() ? OK : Fail;
}

znet::ZNode* ZwaveAddon::onFindNode( znet_node_id_t nodeId ) {
    return m_nodeManager->findNode( nodeId );
}

ZwaveAddon::NodeIdsSet ZwaveAddon::getNodeIds() const {
    ZwaveAddon::NodeIdsSet nodesSet;
    m_nodeManager->forEachNode(
        [&]( const znet::ZNode& node ) { znet::utils::setBit( nodesSet.mask, node.getId() ); } );

    return nodesSet;
}

void ZwaveAddon::znetLog( int lvl, const char* format, va_list list, void* /*arg*/ ) {
    Log::PriorityLevel logLevel = Log::PriorityLevel::TRACE;

    std::string colorPrefix;
    switch( lvl ) {
    case 1:
        colorPrefix = cc::error();
        logLevel = Log::PriorityLevel::ERROR;
        break;
    case 2:
        colorPrefix = cc::warn();
        logLevel = Log::PriorityLevel::WARN;
        break;
    case 3:
        logLevel = Log::PriorityLevel::INFO;
        break;
    case 4:
        colorPrefix = cc::debug();
        logLevel = Log::PriorityLevel::DEBUG;
        break;

    case 0:
    case 5:
    default:
        colorPrefix = cc::trace();
        logLevel = Log::PriorityLevel::TRACE;
        break;
    }

    static std::mutex logMutex;
    std::unique_lock< std::mutex > lockOutput( logMutex );

    static std::array< char, 2048 > buffer;
    static char* currentIter = buffer.begin();
    static auto spaceLeft = buffer.size();

    const auto charsWritten = ::vsnprintf( currentIter, spaceLeft, format, list );

    if(::strchr( buffer.data(), '\r' ) || ::strchr( buffer.data(), '\n' ) ) {
        char* tokenState = nullptr;
        char* token = ::strtok_r( buffer.data(), "\r\n", &tokenState );
        while( token ) {
            Log::print( logLevel, colorPrefix + token + cc::reset() );
            token = ::strtok_r( NULL, "\r\n", &tokenState );
        }

        currentIter = buffer.begin();
        spaceLeft = buffer.size();

        memset( buffer.data(), buffer.size(), '\0' );
    }
    else {
        spaceLeft -= charsWritten;
        currentIter += charsWritten;
    }
}

void* ZwaveAddon::znetAlloc( void* ptr, size_t size, void* arg ) {
    if( size != 0 )
        return realloc( ptr, size );

    free( ptr );
    return nullptr;
}

uint32_t ZwaveAddon::znetClock( void* arg ) {
    auto now = std::chrono::steady_clock().now();
    auto res = std::chrono::duration_cast< std::chrono::milliseconds >( now.time_since_epoch() );
    if( res.count() == 0 )
        return 1;
    return res.count();
}

void ZwaveAddon::znetS2SelectKeys( znet_S2_keys_t keyList, void* /*arg*/ ) {
    LOGGER_INFO( "key list: " + std::bitset< 8 >{ keyList }.to_string() );

    S2SelectAuthenticationModes event( AuthModeUtils::keyListToModes( keyList ) );
    addTask( [this, event] { emit( event ); } );
}

void ZwaveAddon::znetS2RequestPubKey( znet_S2_keys_t /*keyList*/, const znet_S2_pub_key_t* key, void* /*arg*/ ) {
    LOGGER_INFO( "znetS2RequestPubKey" );

    constexpr auto REQUEST_DEVICE_KEY_SIZE = 14;
    constexpr auto SHOW_DEVICE_SIDE_KEY_SIZE = 16;

    if( key ) {
        uint8_t key_count = 0;
        if( key->size == REQUEST_DEVICE_KEY_SIZE ) {
            key_count = 7;
        }
        else if( key->size == SHOW_DEVICE_SIDE_KEY_SIZE ) {  // CSA
            key_count = 2;                                   // Show only 2 first parts
        }
        else {
            return;
        }

        std::vector< uint16_t > pubKey;
        pubKey.reserve( key_count );
        for( std::uint8_t i = 0; i < key_count; ++i ) {
            pubKey.push_back( ( (uint16_t) key->key[ i * 2 ] << 8 ) | key->key[ i * 2 + 1 ] );
        }

        std::shared_ptr< S2BaseDeviceKeyEvent > event;
        if( key_count == 7 ) {
            event = std::make_shared< S2RequestDeviceSpecificKey >( std::move( pubKey ) );
        }
        else {
            event = std::make_shared< S2ShowDeviceSideKey >( std::move( pubKey ) );
        }

        addTask( [this, event]() { emit( *event ); } );
    }
}

void ZwaveAddon::znetS2ShowPubKey( znet_S2_keys_t keyList, const znet_S2_pub_key_t* key, void* arg ) {
    auto dsk = getDSKAsArrayOfInts( key->key, key->size );
    addTask( [this, dsk] { emit( LearnShowKey( dsk ) ); } );
}

std::unique_ptr< znet::INodeSetting > ZwaveAddon::provideNodeSetting( uint16_t manufacturerId,
                                                                      uint16_t productType,
                                                                      uint16_t productId ) {
    try {
        if( m_deviceSettingPath.empty() )
            return nullptr;

        ExtensionsManager::RemoteInterface extensionsInterface( *this );
        auto stream = extensionsInterface.getPluginFile( m_deviceSettingPath );
        Json settings = Json::object();
        *stream >> settings;

        return NodeSetting::create( settings, getDBClient(), manufacturerId, productType, productId );
    }
    catch( const std::exception& e ) {
        LOGGER_ERROR( e.what() );
        return nullptr;
    }
}

void ZwaveAddon::zwaveModuleResetPinAssert() {
    LOGGER_INFO( "Assert reset on zwave chip" );

    m_zwaveStick->zwResetAssert();
}

void ZwaveAddon::zwaveModuleResetPinRelease() {
    LOGGER_INFO( "Deassert reset on zwave chip" );

    m_zwaveStick->zwResetRelease();
}

bool ZwaveAddon::getKnownS0Key( uint8_t* buf ) {
    if( m_veraMigrationHelper ) {
        return m_veraMigrationHelper->fillS0Key( buf );
    }

    return false;
}

void ZwaveAddon::executeTask( std::function< void() > action, std::chrono::system_clock::duration delay ) {
    createPostponedTask( [action]( Services::PostponedTask& ) { action(); } )->start( delay, false );
}

void ZwaveAddon::nodeCreate( const uint8_t nodeId ) {
    LOGGER_INFO( "nodeCreate, nodeId: " + std::to_string( nodeId ) );

    if( auto node = m_nodeManager->findNode( nodeId ) ) {
        LOGGER_WARN( "Reinclusion of existing nodeId: " + std::to_string( nodeId ) +
                     ", previous device will be deleted" );
        NodeRemoved event( node->getId(), node->getManufacturerId(), node->getProductType(), node->getProductId() );
        m_nodeManager->remove( nodeId );
        addTask( [this, event]() { emit( event ); } );
        LOGGER_TRACE( cc::debug( "Node was removed, nodeId: " ) + cc::note( std::to_string( nodeId ) ) );
    }

    m_nodeManager->registerNode( nodeId );
}

void ZwaveAddon::interviewDone( const uint8_t nodeId ) {
    ZwaveNodeJsonSerializer serializer( *m_nodeManager->findNode( nodeId ) );
    LOGGER_INFO( cc::info( "Interview has " ) + cc::success( "done" ) + cc::info( ", node: " ) +
                 cc::j( serializer.serialize() ) );

    // We need to synchronize node data in DB
    if( !m_nodeManager->update( nodeId ) ) {
        LOGGER_ERROR( cc::error( "Cannot update node information in DB after interview, nodeId: " ) +
                      cc::note( std::to_string( nodeId ) ) );
        return;
    }

    using namespace Resources::ScriptEvent;
    addTask( [this, nodeId] { emit( NodeAdded( nodeId ) ); } );
}

void ZwaveAddon::nodeRemove( const uint8_t nodeId ) {
    if( auto node = m_nodeManager->findNode( nodeId ) ) {
        NodeRemoved event( node->getId(), node->getManufacturerId(), node->getProductType(), node->getProductId() );
        m_nodeManager->remove( nodeId );
        addTask( [this, event]() { emit( event ); } );
        LOGGER_TRACE( cc::debug( "Node was removed, nodeId: " ) + cc::note( std::to_string( nodeId ) ) );
    }
    else {
        addTask( [this]() { emit( UnknownNodeRemoved() ); } );
        LOGGER_TRACE( cc::debug( "Unknown node was removed" ) );
    }
}

void ZwaveAddon::onUpdateState( znet::IZNet::Mode cmd, znet::ModeStatus cmdStatus, znet::ErrorOpt error ) {
    if( Log::getLogLevel() >= Log::PriorityLevel::TRACE ) {
        std::string message = cc::debug( "Switched to: " ) + cc::note( znet::mode_converter::toString( cmd ) ) +
                              cc::debug( " mode, " ) + cc::note( znet::mode_status_converter::toString( cmdStatus ) ) +
                              cc::debug( " status. " );
        if( error ) {
            message += getHighlightedMessage( *error );
        }
        Log::trace( message );
    }

    m_lastZwaveState.set( { cmd, cmdStatus } );

    if( !isSupportedEventStatus( cmdStatus ) )
        return;

    if( cmdStatus == znet::ModeStatus::Busy ) {
        addTask( [this, cmd] { emit( ActionBusy( cmd ) ); } );
        return;
    }

    switch( cmd ) {
    case znet::IZNet::Mode::Reseting:
        addTask( [this, cmdStatus, error] { emit( ModuleReset( cmdStatus, std::move( error ) ) ); } );
        if( cmdStatus == znet::ModeStatus::Finished ) {
            m_nodeManager->reset();
        }
        break;

    case znet::IZNet::Mode::IncludeStarting:
        addTask( [this, cmdStatus, error] {
            emit( IncludeEvent( cmdStatus, std::move( error ) ) );
            updateLeds();
        } );
        break;

    case znet::IZNet::Mode::IncludeStopping:
        addTask( [this, cmdStatus, error] {
            switch( cmdStatus ) {
            case znet::ModeStatus::Finished:
                emit( IncludeEvent( znet::ModeStatus::Failed, std::move( error ) ) );
                updateLeds();
            }
        } );
        break;

    case znet::IZNet::Mode::ExcludeStarting:
        addTask( [this, cmdStatus, error] {
            emit( ExcludeEvent( cmdStatus, std::move( error ) ) );
            updateLeds();
        } );
        break;

    case znet::IZNet::Mode::ExcludeStopping:
        addTask( [this, cmdStatus, error] {
            switch( cmdStatus ) {
            case znet::ModeStatus::Finished:
                emit( ExcludeEvent( znet::ModeStatus::Failed, std::move( error ) ) );
                updateLeds();
            }
        } );
        break;

    case znet::IZNet::Mode::LearnStarting:
        addTask( [this, cmdStatus, error] { emit( LearnEvent( cmdStatus, std::move( error ) ) ); } );
        break;

    case znet::IZNet::Mode::RequestNeighbor:
        addTask( [this, cmdStatus, error] { emit( ReconstructEvent( cmdStatus, std::move( error ) ) ); } );
        break;

    case znet::IZNet::Mode::Initializing:
        if( cmdStatus == znet::ModeStatus::Finished ) {
            std::unique_lock< std::mutex > lock( m_mutex );
            m_wasZwaveInitialized = true;
            lock.unlock();
            m_initCondVar.notify_all();
        }
        break;

    case znet::IZNet::Mode::BackupCreation:
    case znet::IZNet::Mode::BackupRestoring:
        if( cmdStatus == znet::ModeStatus::Finished ) {
            m_backupHelper->setFinished();
        }
        else if( cmdStatus == znet::ModeStatus::Failed ) {
            if( error ) {
                m_backupHelper->setFinished( getMessage( *error ) );
            }
            else {
                m_backupHelper->setFinished( "Unknown backup error" );
            }
        }
        break;

    default:
        break;
    }

    // TODO: implement asap
}

void ZwaveAddon::onNodeCheckStatusUpdated( uint8_t nodeId, znet::ModeStatus status ) {
    addTask( [this, nodeId, status] { emit( NodeCheckStatus( nodeId, status ) ); } );
}

void ZwaveAddon::onDeviceIncludeProgressUpdate( uint8_t progress ) {
    addTask( [this, progress] { emit( IncludeDeviceProgress( progress ) ); } );
}

void ZwaveAddon::onDeviceRediscoverProgressUpdate( znet_node_id_t nodeId,
                                                   bool isOk,
                                                   znet::ErrorOpt errorMessage,
                                                   uint8_t progress ) {
    if( Log::getLogLevel() >= Log::PriorityLevel::TRACE ) {
        std::string message = cc::debug( "Rediscovery progress: " ) + cc::num10( progress ) +
                              cc::debug( ", nodeId: " ) + cc::num10( nodeId );

        if( errorMessage ) {
            message += getHighlightedMessage( *errorMessage );
        }

        Log::trace( message );
    }

    if( !isOk ) {
        m_nodeManager->resetNodeCache( nodeId );
        auto* node = m_nodeManager->findNode( nodeId );
        node->setRediscoverRequired( true );

        if( !m_nodeManager->update( nodeId ) ) {
            LOGGER_ERROR( cc::error( "Cannot update node information in DB after rediscover, nodeId: " ) +
                          cc::note( std::to_string( nodeId ) ) );
        }
    }
    else if( progress == 100 ) {
        auto* node = m_nodeManager->findNode( nodeId );
        node->setNeedExtendedRediscover( false );

        ZwaveNodeJsonSerializer serializer( *node );
        LOGGER_TRACE( cc::info( "Interview has " ) + cc::success( "done" ) + cc::info( ", node: " ) +
                      cc::j( serializer.serialize() ) );

        if( !m_nodeManager->update( nodeId ) ) {
            LOGGER_ERROR( cc::error( "Cannot update node information in DB after rediscover, nodeId: " ) +
                          cc::note( std::to_string( nodeId ) ) );
        }
    }

    addTask( [=] {
        emit( RediscoverDeviceProgress(
            nodeId, znet::utils::progressToStringStatus( isOk, progress ), progress, std::move( errorMessage ) ) );
    } );

    if( m_veraMigrationHelper && ( !isOk || progress == 100 ) ) {
        if( !isOk ) {
            addTask( [=] { emit( NodeAdded( nodeId ) ); } );
        }
        m_veraMigrationHelper->onNodeRediscovered( nodeId );
        onVeraBackupStatus( "in_progress", "" );
    }
}

void ZwaveAddon::onLearnProgressUpdate( uint8_t devicesAmount, uint8_t currentDeviceIndex ) {
    addTask(
        [this, devicesAmount, currentDeviceIndex] { emit( LearnProgress( devicesAmount, currentDeviceIndex ) ); } );
}

void ZwaveAddon::onReconstructNetworkUpdate( const uint8_t devicesAmount, const uint8_t currentDeviceIndex ) {
    addTask( [this, devicesAmount, currentDeviceIndex] {
        emit( ReconstructProgressEvent( devicesAmount, currentDeviceIndex ) );
    } );
}

void ZwaveAddon::onReconstructNetworkError( const znet_node_id_t deviceId ) {
    addTask( [this, deviceId] { emit( ReconstructProgressErrorEvent( deviceId ) ); } );
}

void ZwaveAddon::onCheckReachable( uint8_t nodeId ) {
    LOGGER_TRACE( cc::debug( "Checking existing of node: " ) + cc::note( std::to_string( nodeId ) ) );
}

void ZwaveAddon::onUpdateReachable( uint8_t nodeId, bool reachable ) {
    LOGGER_TRACE( cc::debug( "Node " ) + cc::note( std::to_string( nodeId ) ) + cc::debug( " is " ) +
                  ( reachable ? cc::success( "available" ) : cc::fail( "unavailable" ) ) );

    m_nodeManager->update( nodeId );

    ReachableStateUpdated event( nodeId, reachable );
    addTask( [this, event] { emit( event ); } );
}

void ZwaveAddon::onUpdateValue( const uint8_t nodeId,
                                const uint8_t command,
                                const uint8_t channelId,
                                const uint8_t value,
                                const znet::Uint8Opt targetValue,
                                const znet::SecondsOpt targetDuration ) {
    LOGGER_TRACE( "onUpdateValue, nodeId: " + std::to_string( nodeId ) + ", command: " + std::to_string( command ) +
                  ", channelId: " + std::to_string( channelId ) + ", value: " + std::to_string( value ) +
                  ( targetValue ? ", targetValue: " + std::to_string( *targetValue ) : "none" ) +
                  ( targetDuration ? ", targetDuration: " + std::to_string( targetDuration->count() ) : "none" ) );

    ValueUpdated event( nodeId, command, channelId, value, targetValue, targetDuration );
    addTask( [this, event] { emit( event ); } );
}

void ZwaveAddon::onUpdateTokenValue( const uint8_t nodeId,
                                     const uint8_t command,
                                     const uint8_t channelId,
                                     const std::string& value ) {
    LOGGER_TRACE( "onUpdateTokenValue, nodeId: " + std::to_string( nodeId ) + ", command: " +
                  std::to_string( command ) + ", channelId: " + std::to_string( channelId ) + ", value: " + value );

    TokenValueUpdated event( nodeId, command, channelId, value );
    addTask( [this, event] { emit( event ); } );
}

void ZwaveAddon::onUpdateValueAlarmSensor( const uint8_t nodeId,
                                           const uint8_t channelId,
                                           const znet::AlarmSensorType sensorType,
                                           const uint8_t value,
                                           const uint16_t seconds ) {
    const auto sensorName = znet::getAlarmSensorName( sensorType );
    assert( sensorName );

    LOGGER_TRACE( "onUpdateValueAlarmSensor, nodeId: " + std::to_string( nodeId ) + ", sensorName: " + sensorName +
                  ", channelId: " + std::to_string( channelId ) + ", value: " + std::to_string( value ) +
                  ", delay: " + std::to_string( seconds ) );

    AlarmSensorValueUpdated event( nodeId, channelId, sensorType, value, seconds );
    addTask( [this, event] { emit( event ); } );
}

void ZwaveAddon::onUpdateValueBinarySensor( const uint8_t nodeId,
                                            const uint8_t command,
                                            const uint8_t channelId,
                                            const uint8_t sensorType,
                                            const uint8_t value ) {
    LOGGER_TRACE( "onUpdateValueBinarySensor, nodeId: " + std::to_string( nodeId ) +
                  ", channelId: " + std::to_string( channelId ) + ", sensorType: " + std::to_string( sensorType ) +
                  ", value: " + std::to_string( value ) );

    BinarySensorValueUpdated event( nodeId, channelId, value, sensorType );
    addTask( [this, event] { emit( event ); } );
}

void ZwaveAddon::onUpdateValueMultilevelSensor( const uint8_t nodeId,
                                                const uint8_t ccId,
                                                const uint8_t channelId,
                                                const znet::MultilevelSensor::Type type,
                                                const uint8_t scale,
                                                const float value ) {
    LOGGER_TRACE( "onUpdateValueMultilevelSensor, nodeId: " + std::to_string( nodeId ) +
                  ", channelId: " + std::to_string( channelId ) + ", sensorType: " + std::to_string( type ) +
                  ", sensorScale: " + std::to_string( scale ) + ", value: " + std::to_string( value ) );

    MultilevelSensorValueUpdated event( nodeId, channelId, value, type, scale );
    addTask( [this, event] { emit( event ); } );
}

void ZwaveAddon::onUpdateValueDoorLockOperation( const uint8_t nodeId,
                                                 const uint8_t channelId,
                                                 const znet::DoorLock::Mode mode,
                                                 const uint8_t insideHandlesMode,
                                                 const uint8_t outsideHandlesMode,
                                                 const znet::DoorCondition condition,
                                                 const znet::SecondsOpt lockTimeout,
                                                 const znet::DoorLock::ModeOpt targetMode,
                                                 const znet::SecondsOpt targetModeTimeout ) {
    LOGGER_TRACE( "onUpdateValueDoorLockOperation" );

    DoorLockStateUpdated event( nodeId,
                                channelId,
                                mode,
                                insideHandlesMode,
                                outsideHandlesMode,
                                condition,
                                lockTimeout,
                                targetMode,
                                targetModeTimeout );
    addTask( [this, event] { emit( event ); } );
}

void ZwaveAddon::onUpdateValueDoorLockConfiguration(
    const uint8_t nodeId,
    const uint8_t channelId,
    const znet_door_lock_configuration_t& configuration,
    const znet_door_lock_extended_configuration_t* extendedConfiguration ) {
    LOGGER_TRACE( "onUpdateValueDoorLockOperation" );

    DoorLockConfigurationUpdated event(
        nodeId,
        channelId,
        configuration,
        extendedConfiguration ? DoorLockConfigurationUpdated::ExtendedConfigurationOpt( *extendedConfiguration, true ) :
                                DoorLockConfigurationUpdated::ExtendedConfigurationOpt( {}, false ) );
    addTask( [this, event] { emit( event ); } );
}

void ZwaveAddon::onUpdateValueThermostatSetpoint( const uint8_t nodeId,
                                                  const uint8_t channelId,
                                                  const uint8_t type,
                                                  const uint8_t scale,
                                                  const float value,
                                                  const bool isSetpointRequested ) {
    LOGGER_TRACE( "nodeId: " + std::to_string( nodeId ) + ", channelId: " + std::to_string( channelId ) +
                  ", type: " + std::to_string( type ) + ", scale: " + std::to_string( scale ) +
                  ", value: " + std::to_string( value ) );

    ThermostatSetpointValueUpdated event{ nodeId, channelId, type, scale, value, isSetpointRequested };
    addTask( [this, event] { emit( event ); } );
}

void ZwaveAddon::onUpdateThermostatModeValue( const uint8_t nodeId,
                                              const uint8_t channelId,
                                              const uint8_t mode,
                                              znet::ManufacturerSpecificDataOpt manufacturerSpecificData ) {
    auto manufacturerSpecificDataStr =
        manufacturerSpecificData ? znet::utils::bytesToHex( *manufacturerSpecificData ) : std::string();

    LOGGER_TRACE( "nodeId: " + std::to_string( nodeId ) + ", channelId: " + std::to_string( channelId ) + ", mode: " +
                  std::to_string( mode ) + ", manufacturer specific data: " + manufacturerSpecificDataStr );

    ThermostatModeUpdated event( nodeId, channelId, mode, std::move( manufacturerSpecificDataStr ) );
    addTask( [this, event] { emit( event ); } );
}

void ZwaveAddon::onUpdateValueThermostatOperatingState( const uint8_t nodeId,
                                                        const uint8_t channelId,
                                                        const uint8_t command,
                                                        znet::ThermostatOperatingState state ) {
    const char* stateStr = znet::thermostat_operating_state_converter::typeToString( state );
    LOGGER_TRACE( "onUpdateValueThermostatOperatingState, nodeId:" + std::to_string( nodeId ) +
                  ", channelId: " + std::to_string( channelId ) + ", commandId: " + std::to_string( command ) +
                  ", state: " + ( stateStr ? stateStr : "nullptr" ) );

    if( stateStr ) {
        ThermostatOperatingStateValueUpdated event{ nodeId, channelId, command, stateStr };
        addTask( [this, event] { emit( event ); } );
    }
    else {
        LOGGER_ERROR( "cannot send ThermostatOperatingStateValueUpdated, invalid params" );
    }
}

void ZwaveAddon::onUpdateThermostatSetbackValue( const uint8_t nodeId,
                                                 const uint8_t channelId,
                                                 const znet::thermostat_setback::OverrideType overrideType,
                                                 const znet::thermostat_setback::Mode mode,
                                                 const znet::thermostat_setback::SetbackValueOpt valueOpt ) {
    const char* overrideTypeStr = znet::thermostat_setback::overrideTypeToString( overrideType );
    const char* modeTypeStr = znet::thermostat_setback::modeToString( mode );
    LOGGER_TRACE( "nodeId:" + std::to_string( nodeId ) + ", channelId: " + std::to_string( channelId ) +
                  ", override type: " + ( overrideTypeStr ? overrideTypeStr : "nullptr" ) +
                  ", mode: " + ( modeTypeStr ? modeTypeStr : "nullptr" ) +
                  ", value: " + ( valueOpt ? std::to_string( *valueOpt ) : "none" ) );

    if( overrideTypeStr && modeTypeStr ) {
        ThermostatSetbackValueUpdated event{ nodeId, channelId, overrideTypeStr, modeTypeStr, valueOpt };
        addTask( [this, event] { emit( event ); } );
    }
    else {
        LOGGER_ERROR( "cannot send ThermostatSetbackValueUpdated, invalid params " +
                      std::to_string( static_cast< uint8_t >( overrideType ) ) + ", " +
                      std::to_string( static_cast< uint8_t >( mode ) ) );
    }
}

void ZwaveAddon::onUpdateFloatValue( const uint8_t nodeId,
                                     const uint8_t command,
                                     const uint8_t channelId,
                                     const znet::meter::MeterType type,
                                     const znet::meter::Scale scale,
                                     const int32_t value,
                                     const uint8_t precision ) {
    const auto typeName = znet::meter::typeToString( type );
    const auto scaleName = znet::meter::scaleToString( type, scale );

    if( !typeName ) {
        LOGGER_WARN( cc::warn( "Invalid meter type " ) + cc::num10( static_cast< uint8_t >( type ) ) );
        return;
    }

    if( !scaleName ) {
        LOGGER_WARN( cc::warn( "Invalid meter scale " ) + cc::num10( static_cast< uint8_t >( scale ) ) );
        return;
    }

    LOGGER_TRACE( "onUpdateFloatValue, nodeId: " + std::to_string( nodeId ) +
                  ", command: " + std::to_string( command ) + ", channelId: " + std::to_string( channelId ) +
                  ", type: " + typeName + ", scale: " + std::to_string( scale ) + ", value: " + scaleName +
                  ", precision: " + std::to_string( precision ) );

    FloatValueUpdated event( nodeId, command, channelId, typeName, scaleName, value, precision );
    addTask( [this, event] { emit( event ); } );
}

void ZwaveAddon::onUpdateSceneValue( const uint8_t nodeId,
                                     const uint8_t command,
                                     const uint8_t channelId,
                                     const uint8_t sequenceNumber,
                                     const uint8_t sceneNumber,
                                     const std::string& keyAttribute ) {
    LOGGER_INFO( "onUpdateSceneValue, nodeId: " + std::to_string( nodeId ) + ", command: " + std::to_string( command ) +
                 ", channelId: " + std::to_string( channelId ) + ", sceneNumber: " + std::to_string( sceneNumber ) +
                 ", value: " + keyAttribute );
    ButtonStateUpdated event( nodeId, channelId, sceneNumber, keyAttribute );
    addTask( [this, event] { emit( event ); } );
}

void ZwaveAddon::onSceneActivated( const uint8_t nodeId,
                                   const uint8_t command,
                                   const uint8_t channelId,
                                   const uint8_t sceneNumber ) {
    LOGGER_INFO( "onSceneActivated, nodeId: " + std::to_string( nodeId ) + ", command: " + std::to_string( command ) +
                 ", channelId: " + std::to_string( channelId ) + ", sceneNumber: " + std::to_string( sceneNumber ) );
    SceneActivated event( nodeId, channelId, sceneNumber );
    addTask( [this, event] { emit( event ); } );
}

void ZwaveAddon::onSimpleAvControlSet( const uint8_t nodeId,
                                       const uint8_t channelId,
                                       const uint8_t sequenceNumber,
                                       const std::string& keyAtribute,
                                       std::vector< uint16_t >&& commands ) {
    SimpleAvControlSetReceived event( nodeId, channelId, sequenceNumber, keyAtribute, std::move( commands ) );
    addTask( [this, event] { emit( event ); } );
}

void ZwaveAddon::onUpdateRgbValue( const uint8_t nodeId,
                                   const uint8_t command,
                                   const uint8_t channelId,
                                   znet::ColorComponentInfos&& components ) {
    LOGGER_INFO( "onUpdateRgbValue, nodeId: " + std::to_string( nodeId ) + ", command: " + std::to_string( command ) +
                 ", channelId: " + std::to_string( channelId ) );

    RgbValueUpdated event( nodeId, command, channelId, std::move( components ) );
    addTask( [this, event] { emit( event ); } );
}

void ZwaveAddon::onConfigurationReport( const uint8_t nodeId,
                                        const uint8_t channelId,
                                        const uint16_t configParamNum,
                                        const uint8_t configParamSize,
                                        const uint32_t configParamValue ) {
    addTask( [=] {
        const znet::ZCc0x70Configuration::Parameter* parameter = nullptr;

        if( auto cc =
                findCommandClassSafe< znet::ZCc0x70Configuration >( nodeId, 0, "ZwaveAddon::onConfigurationReport" ) ) {
            parameter = cc->findV3Parameter( configParamNum );
        }

        ConfigurationValueUpdated event( nodeId,
                                         channelId,
                                         configParamNum,
                                         configParamSize,
                                         configParamValue,
                                         parameter ? &parameter->m_format : nullptr );
        emit( event );
    } );
}

void ZwaveAddon::onConfigurationRequestStatus( const uint8_t nodeId,
                                               const uint16_t configParamNum,
                                               const znet::RequestStatus::Enum status ) {
    ConfigurationRequestStatusUpdated event( nodeId, configParamNum, status );
    addTask( [this, event] { emit( event ); } );
}

void ZwaveAddon::onNotification( const uint8_t nodeId,
                                 const uint8_t channelId,
                                 const uint8_t v1AlarmType,
                                 const uint8_t v1AlarmLevel,
                                 znet::NotificationType notification,
                                 znet::NotificationEvent eventType,
                                 const uint8_t* data,
                                 const uint8_t dataSize ) {
    NotificationReceived event(
        nodeId, channelId, v1AlarmType, v1AlarmLevel, notification, eventType, { data, data + dataSize } );
    addTask( [this, event] { emit( event ); } );
}

void ZwaveAddon::onUnknownNotification( const uint8_t nodeId,
                                        const uint8_t channelId,
                                        const uint8_t v1AlarmType,
                                        const uint8_t v1AlarmLevel,
                                        znet::NotificationType notification,
                                        znet::NotificationEvent eventType,
                                        const uint8_t* data,
                                        const uint8_t dataSize ) {
    UnknownNotificationReceived event(
        nodeId, channelId, v1AlarmType, v1AlarmLevel, notification, eventType, { data, data + dataSize } );
    addTask( [this, event] { emit( event ); } );
}

void ZwaveAddon::onUserCodeUpdated( const znet_node_id_t nodeId,
                                    const znet_node_channel_id_t channelId,
                                    znet::UserCode::UserCodeRanges&& userCodeRanges,
                                    znet_supported_users_t usersCount,
                                    znet::UserCode::UserPollStatus userPollingStatus ) {
    auto userCodesToString = [&]() {
        std::string result;

        for( const auto& range : userCodeRanges ) {
            result.append( "[" )
                .append( std::to_string( range.id ) )
                .append( "; " )
                .append( std::to_string( range.lastId ) )
                .append( "; " )
                .append( std::to_string( static_cast< uint8_t >( range.status ) ) )
                .append( "; " )
                .append( range.code )
                .append( "]" );
        }

        return result;
    };

    LOGGER_TRACE( std::string( "onUserCodeUpdated" ) + ", nodeId: " + std::to_string( nodeId ) +
                  ", channelId: " + std::to_string( channelId ) + ", codes: " + userCodesToString() );

    UserCodeUpdated event( nodeId, channelId, std::move( userCodeRanges ), usersCount, userPollingStatus );
    addTask( [this, event] { emit( event ); } );
}

void ZwaveAddon::onMasterCodeUpdated( const znet_node_id_t nodeId,
                                      const znet_node_channel_id_t channelId,
                                      const std::string& masterCode ) {
    LOGGER_TRACE( std::string( "onMasterCodeUpdated" ) + ", nodeId: " + std::to_string( nodeId ) +
                  ", channelId: " + std::to_string( channelId ) + ", code: " + masterCode );

    MasterCodeUpdated event( nodeId, channelId, masterCode );
    addTask( [this, event] { emit( event ); } );
}

void ZwaveAddon::onUserCodesKeypadModeUpdated( const uint8_t nodeId,
                                               const uint8_t channelId,
                                               znet::UserCode::KeypadMode keypadMode ) {
    const char* stringKeypadMode = znet::user_code_converter::keypadModeToString( keypadMode );

    LOGGER_TRACE( std::string( "onUserCodesKeypadModeUpdated" ) + ", nodeId: " + std::to_string( nodeId ) +
                  ", channelId: " + std::to_string( channelId ) +
                  ", mode: " + ( stringKeypadMode ? stringKeypadMode : "nullptr" ) );

    if( stringKeypadMode ) {
        UserCodesKeypadModeUpdated event( nodeId, channelId, stringKeypadMode );
        addTask( [this, event] { emit( event ); } );
    }
}

void ZwaveAddon::onUpdateUserCodesChecksum( const uint8_t nodeId, const uint8_t channelId, const uint16_t checksum ) {
    LOGGER_TRACE_RAW( std::string( "onChecksumReceived" ) + ", nodeId: " + std::to_string( nodeId ) +
                      ", channelId: " + std::to_string( channelId ) + ", checksum: " + std::to_string( checksum ) );

    UserCodesChecksumUpdated event( nodeId, channelId, checksum );
    addTask( [this, event] { emit( event ); } );
}

void ZwaveAddon::onUpdateScheduleEntryLockWeekDay( znet_node_id_t nodeId,
                                                   znet_node_channel_id_t channelId,
                                                   const znet_week_day_schedule_t& value ) {
    LOGGER_TRACE( std::string( "onUpdateScheduleEntryLockWeekDay" ) + ", nodeId: " + std::to_string( nodeId ) +
                  ", channelId: " + std::to_string( channelId ) +
                  ", userId: " + std::to_string( value.schedule_id.user_id ) + ", slotId: " +
                  std::to_string( value.schedule_id.slot_id ) + ", weekDay: " + std::to_string( value.day_of_week ) );

    ScheduleEntryLockWeekDayUpdated event( nodeId, channelId, value );
    addTask( [this, event] { emit( event ); } );
}

void ZwaveAddon::onUpdateScheduleEntryLockYearDay( znet_node_id_t nodeId,
                                                   znet_node_channel_id_t channelId,
                                                   const znet_year_day_schedule_t& value ) {
    LOGGER_TRACE( std::string( "onUpdateScheduleEntryLockYearDay" ) + ", nodeId: " + std::to_string( nodeId ) +
                  ", channelId: " + std::to_string( channelId ) +
                  ", userId: " + std::to_string( value.schedule_id.user_id ) +
                  ", slotId: " + std::to_string( value.schedule_id.slot_id ) );

    ScheduleEntryLockYearDayUpdated event(
        nodeId, channelId, value, static_cast< uint8_t >( getLocalDateTime().date.year / 100 ) );
    addTask( [this, event] { emit( event ); } );
}

void ZwaveAddon::onUpdateScheduleEntryLockDailyRepeating( znet_node_id_t nodeId,
                                                          znet_node_channel_id_t channelId,
                                                          const znet_daily_repeating_schedule_t& value ) {
    LOGGER_TRACE( std::string( "onUpdateScheduleEntryLockDailyRepeating" ) + ", nodeId: " + std::to_string( nodeId ) +
                  ", channelId: " + std::to_string( channelId ) +
                  ", userId: " + std::to_string( value.schedule_id.user_id ) +
                  ", slotId: " + std::to_string( value.schedule_id.slot_id ) +
                  ", weekDays: " + std::to_string( value.week_day_bitmask ) );

    ScheduleEntryLockDailyRepeatingUpdated event( nodeId, channelId, value );
    addTask( [this, event] { emit( event ); } );
}

void ZwaveAddon::onNodeSchedule( znet_node_id_t nodeId,
                                 znet_node_channel_id_t channelId,
                                 uint8_t scheduleId,
                                 const znet::ScheduleIdBlockOpt& idBlockOpt,
                                 const znet::Schedule& value ) {
    std::string optBlockIdStr;
    idBlockOpt ? optBlockIdStr = std::string( ", blockId " ) + std::to_string( *idBlockOpt ) :
                 std::string( "Block Id isn't set" );

    LOGGER_TRACE( std::string( "onNodeSchedule" ) + ", nodeId: " + std::to_string( nodeId ) + ", channelId: " +
                  std::to_string( channelId ) + ", scheduleId: " + std::to_string( scheduleId ) + optBlockIdStr );

    ScheduleUpdated event( nodeId, channelId, scheduleId, idBlockOpt, value );
    addTask( [this, event] { emit( event ); } );
}

void ZwaveAddon::onNodeScheduleState( znet_node_id_t nodeId,
                                      znet_node_channel_id_t channelId,
                                      const znet_cmd_schedule_state_report_t& value,
                                      const znet::ScheduleIdBlockOpt& idBlockOpt ) {
    LOGGER_TRACE( std::string( "onNodeScheduleState" ) + ", nodeId: " + std::to_string( nodeId ) +
                  ", channelId: " + std::to_string( channelId ) +
                  ", numberSupportedScheduleIds: " + std::to_string( value.number_supported_schedule_ids ) +
                  ", reportsToFollow " + std::to_string( value.reports_to_follow ) );

    ScheduleStateUpdated event( nodeId, channelId, value, idBlockOpt );
    addTask( [this, event] { emit( event ); } );
}

void ZwaveAddon::onUpdateSoundSwitchConfiguration( znet_node_id_t nodeId,
                                                   znet_node_channel_id_t channelId,
                                                   const znet_cmd_sound_switch_configuration_t& value ) {
    LOGGER_TRACE( std::string( "onUpdateSoundSwitchConfiguration" ) + ", nodeId: " + std::to_string( nodeId ) +
                  ", channelId: " + std::to_string( channelId ) + ", defaultToneId: " +
                  std::to_string( value.default_tone_id ) + ", volume: " + std::to_string( value.volume ) );

    SoundSwitchConfigurationUpdated event( nodeId, channelId, value.default_tone_id, value.volume );
    addTask( [this, event] { emit( event ); } );
}

void ZwaveAddon::onUpdateSoundSwitchTonePlay( znet_node_id_t nodeId,
                                              znet_node_channel_id_t channelId,
                                              const znet_cmd_sound_switch_tone_play_t& value ) {
    LOGGER_TRACE( std::string( "onUpdateSoundSwitchTonePlay" ) + ", nodeId: " + std::to_string( nodeId ) +
                  ", channelId: " + std::to_string( channelId ) + ", toneId: " + std::to_string( value.tone_id ) +
                  ", volume: " + std::to_string( value.volume ) );

    SoundSwitchTonePlayUpdated event( nodeId, channelId, value.tone_id, value.volume );
    addTask( [this, event] { emit( event ); } );
}

void ZwaveAddon::onNodeTargetFirmwareUpdateProgress( const znet_node_id_t nodeId,
                                                     const uint8_t target,
                                                     znet::FirmwareUpdate::UpdatingResult status,
                                                     uint8_t progress ) {
    LOGGER_INFO( "Device firmware update status for node " + std::to_string( nodeId ) + ", status " +
                 std::to_string( static_cast< uint8_t >( status ) ) + ", progress " + std::to_string( progress ) );

    if( status != znet::FirmwareUpdate::UpdatingResult::InProgress ) {
        m_deviceFirmwareUpdateManager->onFirmwareUpdateFinished(
            nodeId, target, status != znet::FirmwareUpdate::UpdatingResult::Error );
    }

    addTask( [=] {
        emit( NodeFirmwareUpdateStatusEvent(
            nodeId,
            static_cast< uint8_t >( target ),
            znet::utils::progressToStringStatus( status != znet::FirmwareUpdate::UpdatingResult::Error, progress ),
            progress ) );
    } );
}

void ZwaveAddon::onUpdateBarrierSignalValue( znet_node_id_t nodeId,
                                             znet_node_channel_id_t channelId,
                                             uint8_t signalId,
                                             uint8_t value ) {
    addTask( [=] { emit( BarrierSignalValueUpdated( nodeId, channelId, signalId, value ) ); } );
}

void ZwaveAddon::onHail( const uint8_t nodeId ) {
    addTask( [this, nodeId] { emit( NodeEvent( Resources::ScriptEvent::Name::OnHail, nodeId ) ); } );
}

void ZwaveAddon::onIndicatorIdentify() {
    addTask( [this] { sendBroadcast( ZWAVE_IDENTIFY_EVENT, {} ); } );
}

void ZwaveAddon::onNif( const znet_node_id_t nodeId ) {
    addTask( [this, nodeId] { emit( NodeEvent( Resources::ScriptEvent::Name::OnNif, nodeId ) ); } );
}

void ZwaveAddon::onSisUpdated( bool enabled ) {
    LOGGER_TRACE( cc::debug( "onSisUpdated, enabled: " ) + cc::note( enabled ? "true" : "false" ) );
    addTask( [this] { emit( SisNodeUpdated() ); } );
}

void ZwaveAddon::onAssociationSupportedGroupingResult( const uint8_t nodeId, const uint8_t count ) {
    std::cout << std::endl << __FUNCTION__ << std::endl;
}

void ZwaveAddon::onAssociationResult( const uint8_t nodeId,
                                      const uint8_t channelId,
                                      const uint8_t groupId,
                                      const std::vector< uint8_t >& nodes ) {
    LOGGER_TRACE( std::string( "onAssociationUpdated" ) + ", nodeId: " + std::to_string( nodeId ) +
                  ", groupId: " + std::to_string( groupId ) );

    AssociationUpdated< std::vector< uint8_t > > event(
        nodeId, channelId, ZNET_COMMAND_CLASS_ASSOCIATION, groupId, nodes );

    addTask( [this, event] { emit( event ); } );
}

void ZwaveAddon::onMultichannelAssociationResult( const uint8_t nodeId,
                                                  const uint8_t channelId,
                                                  const uint8_t groupId,
                                                  const std::vector< std::pair< uint8_t, uint8_t > >& nodes ) {
    LOGGER_TRACE( std::string( "onMultiChannelAssociationUpdated, nodeId: " ) + std::to_string( nodeId ) +
                  ", groupId: " + std::to_string( groupId ) );

    AssociationUpdated< std::vector< std::pair< uint8_t, uint8_t > > > event(
        nodeId, channelId, ZNET_COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION, groupId, nodes );

    addTask( [this, event] { emit( event ); } );
}

void ZwaveAddon::onProvisioningListEntryAdded( const znet::ProvisioningListEntry& entry ) {
    addTask( [this, entry] {
        Json json;
        ProvisioningListUtils::entryToJsonUiFiltered( entry, json, false );
        auto result = JSON( "provisioning" << JSON_ARRAY( json ) );
        Services::Message message = { "ui_service", EVENT, ZWAVE_NODES_PROVISIONING_ADDED, result };
        send( message );
    } );
}

void ZwaveAddon::onProvisioningListEntryUpdated( const znet::ProvisioningListEntry& oldEntry,
                                                 const znet::ProvisioningListEntry& newEntry ) {
    addTask( [this, oldEntry, newEntry] {
        Json oldEntryJson;
        Json newEntryJson;

        ProvisioningListUtils::entryFieldsToJsonUiFiltered( oldEntry.m_fields, oldEntryJson );
        ProvisioningListUtils::genEntryFieldsToJson( oldEntry.m_fields, oldEntryJson );
        ProvisioningListUtils::entryFieldsToJsonUiFiltered( newEntry.m_fields, newEntryJson );
        ProvisioningListUtils::genEntryFieldsToJson( newEntry.m_fields, newEntryJson );

        auto json = jsonDiff( oldEntryJson, newEntryJson );
        if( json.empty() ) {
            return;
        }

        ProvisioningListUtils::idToJson( newEntry.m_provisioningId, json );

        auto result = JSON( "provisioning" << JSON_ARRAY( json ) );
        Services::Message message = { "ui_service", EVENT, ZWAVE_NODES_PROVISIONING_UPDATED, result };
        send( message );
    } );
}

void ZwaveAddon::onProvisioningListEntryRemoved( uint32_t provisioningId ) {
    addTask( [this, provisioningId] {
        auto result = JSON( "provisioning" << JSON_ARRAY( ProvisioningListUtils::idToString( provisioningId ) ) );
        Services::Message message = { "ui_service", EVENT, ZWAVE_NODES_PROVISIONING_REMOVED, result };
        send( message );
    } );
}

bool ZwaveAddon::isSupportedEventStatus( znet::ModeStatus cmdStatus ) {
    switch( cmdStatus ) {
    case znet::ModeStatus::Invoked:
    case znet::ModeStatus::Started:
    case znet::ModeStatus::Failed:
    case znet::ModeStatus::Timeout:
    case znet::ModeStatus::Busy:
    case znet::ModeStatus::Finished:
        return true;

    default:
        return false;
    }
}

znet::ITimerProvider::TimerUPtr ZwaveAddon::createTimer( TimerCallback callback ) {
    return std::make_unique< ZNetTimer >( m_zwaveTaskLoop, std::move( callback ) );
}

void ZwaveAddon::fillManufacturerSpecific( znet_manufacturer_specific_report_t& data ) {
    data.product_id = Resources::DeviceInfo::getProductId();
    data.product_type = Resources::DeviceInfo::getProductType();
    data.manufacturer_id = Resources::DeviceInfo::getManufacturerId();
}

void ZwaveAddon::fillVersion( znet_version_report_t& data ) {
    data.hard_ver = Resources::DeviceInfo::getHardwareVersion();
    const auto chipVersion = Resources::DeviceInfo::getZwaveChipFirmwareVersion();
    data.proto.ver = chipVersion.first;
    data.proto.sub_ver = chipVersion.second;

    const auto mainVersion = Resources::DeviceInfo::getHubFirmwareVersion();

    data.firms_count = 2;
    data.firms[ 0 ].ver = chipVersion.first;
    data.firms[ 0 ].sub_ver = chipVersion.second;
    data.firms[ 1 ].ver = mainVersion.first;
    data.firms[ 1 ].sub_ver = mainVersion.second;
}

void ZwaveAddon::fillZWavePlusInfo( znet_zwaveplus_info_report_t& data ) {
    data.user_icon = Resources::DeviceInfo::getUserIconType();
    data.installer_icon = Resources::DeviceInfo::getInstallerIconType();
}

void ZwaveAddon::fillFirmwareUpdateInfo( znet_firmware_md_report_v1_t& dataV1,
                                         znet_firmware_md_report_v3_t& dataV3,
                                         znet_firmware_md_report_v5_t& dataV5 ) {
    dataV1.firmware_id = Resources::DeviceInfo::getZwaveChipFirmwareId();
    dataV1.manufacturer_id = Resources::DeviceInfo::getManufacturerId();

    dataV3.additional_firmware_count = 1;
    dataV3.additional_firmware[ 0 ] = Resources::DeviceInfo::getHubFirmwareId();

    dataV5.hardware_revision = Resources::DeviceInfo::getHardwareVersion();
}

void ZwaveAddon::fillFirmwareLocale( std::string& locale ) {
    locale = Resources::DeviceInfo::getZwaveChipLocale();
}

void ZwaveAddon::setTimeZoneSettings( const char* zoneName, const char* zone ) {
    addTask( [this, zoneName] {
        sendRequest(
            GroupsNames::CORED, JSON( "location" << zoneName << "restart_firmware" << false ), HUB_SET_TIME_ZONE );
    } );
}

void ZwaveAddon::onWakeupIntervalUpdated( const znet_node_id_t nodeId, znet::Seconds interval, bool isChanged ) {
    LOGGER_TRACE( "interval " + cc::num( interval.count() ) );

    WakeUpIntervalUpdated event( nodeId, interval );
    addTask( [this, event] { emit( event ); } );

    if( isChanged ) {
        m_nodeManager->update( nodeId );
    }
}

void ZwaveAddon::onAntiTheftStateUpdated( znet_node_id_t nodeId,
                                          znet_node_channel_id_t channelId,
                                          znet_command_class_t commandClassId,
                                          const znet::AntiTheftInfoOpt& antiTheftInfo ) {
    LOGGER_TRACE( "locked = " + cc::flag( antiTheftInfo.has_value() ) );

    AntiTheftStateUpdated event( nodeId, channelId, commandClassId, antiTheftInfo );
    addTask( [this, event] { emit( event ); } );
}

void ZwaveAddon::onNodeCommandDelay( const uint16_t nodeId,
                                     std::chrono::system_clock::time_point commandSent,
                                     std::chrono::system_clock::duration delay ) {
    LOGGER_DEBUG_PRINT( cc::debug( "Command to node sent, node_id: %hhu, delay: %lld ms" ).c_str(),
                        nodeId,
                        std::chrono::duration_cast< std::chrono::milliseconds >( delay ).count() );

    addTask( [=] { m_nodeStatisticsCollector->onNodeCommandDelay( nodeId, commandSent, delay ); } );
}

void ZwaveAddon::onNodeCommandFailed( const uint16_t nodeId, std::chrono::system_clock::time_point commandSent ) {
    LOGGER_DEBUG_PRINT( cc::debug( "Failed to send command to node, node_id: %hhu" ).c_str(), nodeId );

    addTask( [=] { m_nodeStatisticsCollector->onNodeCommandFailed( nodeId, commandSent ); } );
}

void ZwaveAddon::setDoorLockMode( const Json& params ) {
    LOGGER_TRACE( json_write( params ) );

    const uint8_t nodeId = params[ Resources::RPCParams::NodeId ];
    const uint8_t channelId = params[ Resources::RPCParams::ChannelId ];
    const uint8_t value = params[ Resources::RPCParams::Value ];

    addZWaveTask( [=]() {
        m_znet->getNodeControlManager().setDoorLockMode(
            nodeId, channelId, static_cast< znet::DoorLock::Mode >( value ) );
    } );
}

void ZwaveAddon::requestDoorLockState( const Json& params ) {
    LOGGER_TRACE( json_write( params ) );

    const uint8_t nodeId = params[ Resources::RPCParams::NodeId ];
    const uint8_t channelId = params[ Resources::RPCParams::ChannelId ];

    addZWaveTask( [=]() { m_znet->getNodeControlManager().requestDoorLockState( nodeId, channelId ); } );
}

void ZwaveAddon::setDoorLockConfiguration( const Json& params ) {
    LOGGER_TRACE( json_write( params ) );

    using namespace Resources;
    using namespace JsonCustomGetters;

    const uint8_t nodeId = params[ RPCParams::NodeId ];
    const uint8_t channelId = params[ RPCParams::ChannelId ];
    constexpr uint16_t InvalidLockTimeout = 0xFFFF;
    znet_door_lock_configuration_t configuration;
    configuration.lock_timeout = InvalidLockTimeout;

    std::pair< znet_door_lock_extended_configuration_t, bool > extendedConfigurationOpt( {}, false );

    const auto configurationGetter = makeStructGetter(
        makeMemberMapping(
            &znet_door_lock_configuration_t::lock_timeout, RPCParams::LockTimeout, OptionalMemberPolicy( false ) ),
        makeMemberMapping( &znet_door_lock_configuration_t::enabled_inside_handle_ids,
                           RPCParams::EnabledInsideHandleIds ),
        makeMemberMapping( &znet_door_lock_configuration_t::enabled_outside_handle_ids,
                           RPCParams::EnabledOutsideHandleIds ) );

    if( !configurationGetter( params, configuration ) ) {
        LOGGER_ERROR( "Wrong params: " + params.dump() );
        return;
    }

    configuration.are_timed_operations_enabled = configuration.lock_timeout != InvalidLockTimeout;

    const auto extendedConfigurationGetter =
        makeStructGetter( makeMemberMapping( &znet_door_lock_extended_configuration_t::auto_relock_timeout,
                                             RPCParams::AutoRelockTimeout ),
                          makeMemberMapping( &znet_door_lock_extended_configuration_t::hold_and_release_timeout,
                                             RPCParams::HoldAndReleaseTimeout ),
                          makeMemberMapping( &znet_door_lock_extended_configuration_t::twist_assist_enabled,
                                             RPCParams::TwistAssistEnabled ),
                          makeMemberMapping( &znet_door_lock_extended_configuration_t::block_to_block_enabled,
                                             RPCParams::BlockToBlockEnabled ) );

    extendedConfigurationOpt.second = extendedConfigurationGetter( params, extendedConfigurationOpt.first );

    addZWaveTask( [=]() {
        m_znet->getNodeControlManager().setDoorLockConfiguration(
            nodeId,
            channelId,
            configuration,
            extendedConfigurationOpt.second ? &extendedConfigurationOpt.first : nullptr );
    } );
}

void ZwaveAddon::requestDoorLockConfiguration( const Json& params ) {
    LOGGER_TRACE( json_write( params ) );

    const uint8_t nodeId = params[ Resources::RPCParams::NodeId ];
    const uint8_t channelId = params[ Resources::RPCParams::ChannelId ];

    addZWaveTask( [=]() { m_znet->getNodeControlManager().requestDoorLockConfiguration( nodeId, channelId ); } );
}

void ZwaveAddon::resetMeter( const Json& params ) {
    const uint8_t nodeId = params[ Resources::RPCParams::NodeId ];
    const uint8_t channelId = params[ Resources::RPCParams::ChannelId ];
    const uint8_t paramSize = params[ Resources::RPCParams::ParamSize ];
    const int value = params[ Resources::RPCParams::Value ];

    addZWaveTask( [=]() { m_znet->getNodeControlManager().resetMeter( nodeId, channelId, paramSize, value ); } );
}

void ZwaveAddon::requestMeter( const Json& params ) {
    const uint8_t nodeId = params[ Resources::RPCParams::NodeId ];
    const uint8_t channelId = params[ Resources::RPCParams::ChannelId ];
    const uint8_t scale = params[ Resources::RPCParams::Scale ];

    addZWaveTask( [=]() { m_znet->getNodeControlManager().requestMeter( nodeId, channelId, scale ); } );
}

void ZwaveAddon::updateNodesFirmware( const Json& params ) {
    DeviceFirmwareUpdateManager::Devices nodeIds = params[ Resources::RPCParams::NodeIds ];
    DeviceFirmwareUpdateManager::TargetsToLinks targetsToLinks;
    for( auto& firmware : params[ Resources::RPCParams::Firmwares ] ) {
        DeviceFirmwareUpdateManager::ExtendedLink link{ json_get_str( firmware, Resources::RPCParams::Image ),
                                                        json_get_str( firmware, Resources::RPCParams::Checksum ) };
        targetsToLinks.emplace( firmware[ Resources::RPCParams::Target ], std::move( link ) );
    }

    m_deviceFirmwareUpdateManager->addUpdateScope( std::move( nodeIds ), std::move( targetsToLinks ) );
}

Json ZwaveAddon::getZwaveInfo() const {
    using namespace Resources::Fields;

    auto commonInfo = m_znet->getCommonInfo();

    Json info;

    info[ ZwaveInfo::Module ] = commonInfo->m_module;
    info[ ZwaveInfo::ModuleFirmwareVersion ] = commonInfo->m_module_firmware_version;
    info[ ZwaveInfo::Locale ] = commonInfo->m_locale;
    info[ ZwaveInfo::ZwaveApiVersion ] = commonInfo->m_zwave_api_version;
    info[ ZwaveInfo::ZwaveApiCapabilities ] = commonInfo->m_zwave_api_capabilities;
    info[ ZwaveInfo::ChipType ] = commonInfo->m_chip_type;
    info[ ZwaveInfo::ChipVersion ] = commonInfo->m_chip_version;
    info[ ZwaveInfo::HomeId ] = commonInfo->m_home_id;
    info[ ZwaveInfo::NodeId ] = commonInfo->m_node_id;
    info[ ZwaveInfo::Port ] = m_ttyPath;

    Json poll;
    poll[ ZwaveInfo::PollPowered ] = commonInfo->m_pole.m_powered;
    poll[ ZwaveInfo::PollBattery ] = commonInfo->m_pole.m_battery;
    info[ ZwaveInfo::Poll ] = std::move( poll );

    Json roles;
    roles[ ZwaveInfo::RoleSIS ] = static_cast< bool >( commonInfo->m_roles.m_sis );
    roles[ ZwaveInfo::RolePrimary ] = static_cast< bool >( commonInfo->m_roles.m_primary );
    roles[ ZwaveInfo::RoleSecondary ] = static_cast< bool >( commonInfo->m_roles.m_secondary );
    roles[ ZwaveInfo::RoleStatic ] = static_cast< bool >( commonInfo->m_roles.m_static );
    roles[ ZwaveInfo::RoleInclusion ] = static_cast< bool >( commonInfo->m_roles.m_inclusion );
    info[ ZwaveInfo::Roles ] = std::move( roles );

    return info;
}

void ZwaveAddon::setWakeUpInterval( const Json& params ) {
    const uint8_t nodeId = params[ Resources::RPCParams::NodeId ];
    const uint32_t value = params[ Resources::RPCParams::Value ];

    auto node = m_nodeManager->findNode( nodeId );
    if( node ) {
        addZWaveTask( [this, node, value]() {
            node->setDeviceSpecificWakeUpInterval( value );
            m_nodeManager->update( node->getId() );
        } );
    }
}

void ZwaveAddon::setTestMode() {
    m_isTestMode = true;
}

void ZwaveAddon::downloadNodeFirmware( const Json& params ) {
    const uint8_t nodeId = params[ Resources::RPCParams::NodeId ];
    const uint8_t target = params[ Resources::RPCParams::Target ];
    const std::string path = params[ Resources::RPCParams::Path ];

    auto stream = std::make_shared< std::ofstream >( path, std::ofstream::binary );
    if( !stream->is_open() ) {
        LOGGER_ERROR( "Cannot open file " + path + " for firmware download" );
    }

    if( auto cc = findCommandClassSafe< znet::ZCc0x7AFirmwareUpdate >(
            nodeId, ZNET_CHANNEL_ID_ROOT, "ZwaveAddon::downloadFirmware" ) ) {
        auto task = [=]() {
            cc->download(
                target,
                std::make_unique< FileDestination >( stream ),
                [=]( znet::ZCc0x7AFirmwareUpdate::DownloadingResult result ) {
                    if( result == znet::ZCc0x7AFirmwareUpdate::DownloadingResult::Error ) {
                        LOGGER_INFO_RAW( "Error occurred on device download for node " + std::to_string( nodeId ) );

                        stream->close();
                        remove( path.c_str() );
                    }
                    else {
                        LOGGER_INFO_RAW( "Device firmware has been downloaded for node " + std::to_string( nodeId ) );
                    }
                } );
        };

        addZWaveTask( std::move( task ) );
    }
}

void ZwaveAddon::startRediscover( const Json& params ) {
    const uint8_t nodeId = params[ Resources::RPCParams::NodeId ];
    m_deviceSettingPath = json_get_str( params, Resources::RPCParams::DeviceSettingFilePath );

    LOGGER_INFO( "startRediscover,  node " + std::to_string( nodeId ) );

    addZWaveTask( [this, nodeId]() { m_znet->getNodeControlManager().startRediscover( nodeId ); } );
}

void ZwaveAddon::sendIdentify( const Json& params ) {
    LOGGER_TRACE( json_write( params ) );

    const uint8_t nodeId = params[ Resources::RPCParams::NodeId ];
    const uint8_t channelId = params[ Resources::RPCParams::ChannelId ];

    addZWaveTask( [=]() { m_znet->getNodeControlManager().sendIdentify( nodeId, channelId ); } );
}

void ZwaveAddon::requestNotificationValue( const Json& params ) {
    LOGGER_TRACE( json_write( params ) );

    const uint8_t nodeId = params[ Resources::RPCParams::NodeId ];
    const uint8_t channelId = params[ Resources::RPCParams::ChannelId ];
    const uint8_t type = params[ Resources::RPCParams::NotificationType ];
    const uint8_t event = params[ Resources::RPCParams::NotificationEvent ];

    addZWaveTask( [=]() {
        m_znet->getNodeControlManager().requestNotificationValue(
            nodeId, channelId, static_cast< znet::NotificationType >( type ), event );
    } );
}

void ZwaveAddon::requestAlarmV1Value( const Json& params ) {
    LOGGER_TRACE( json_write( params ) );

    const uint8_t nodeId = params[ Resources::RPCParams::NodeId ];
    const uint8_t channelId = params[ Resources::RPCParams::ChannelId ];
    const uint8_t type = params[ Resources::RPCParams::NotificationType ];

    addZWaveTask( [=]() { m_znet->getNodeControlManager().requestAlarmV1Value( nodeId, channelId, type ); } );
}

void ZwaveAddon::setBarrierSignalValue( const Json& params ) {
    LOGGER_TRACE( json_write( params ) );

    const uint8_t nodeId = params[ Resources::RPCParams::NodeId ];
    const uint8_t channelId = params[ Resources::RPCParams::ChannelId ];
    const std::string signalName = params[ Resources::RPCParams::SignalName ];
    const uint8_t value = params[ Resources::RPCParams::Value ];

    addZWaveTask( [=]() {
        m_znet->getNodeControlManager().setBarrierSignalValue(
            nodeId,
            channelId,
            static_cast< uint8_t >( znet::barrier_operator_signal_converter::stringToSignal( signalName.c_str() ) ),
            value );
    } );
}

void ZwaveAddon::requestBarrierSignalValue( const Json& params ) {
    LOGGER_TRACE( json_write( params ) );

    const uint8_t nodeId = params[ Resources::RPCParams::NodeId ];
    const uint8_t channelId = params[ Resources::RPCParams::ChannelId ];
    const std::string signalName = params[ Resources::RPCParams::SignalName ];

    addZWaveTask( [=]() {
        m_znet->getNodeControlManager().requestBarrierSignalValue(
            nodeId,
            channelId,
            static_cast< uint8_t >( znet::barrier_operator_signal_converter::stringToSignal( signalName.c_str() ) ) );
    } );
}

void ZwaveAddon::setThermostatSetbackValue( const Json& params ) {
    LOGGER_TRACE( json_write( params ) );

    const uint8_t nodeId = params[ Resources::RPCParams::NodeId ];
    const uint8_t channelId = params[ Resources::RPCParams::ChannelId ];
    const uint8_t overrideType = params[ Resources::RPCParams::OverrideType ];
    const uint8_t mode = params[ Resources::RPCParams::Mode ];
    const Json value = params[ Resources::RPCParams::Value ];

    addZWaveTask( [=]() {
        m_znet->getNodeControlManager().setThermostatSetbackValue(
            nodeId,
            channelId,
            static_cast< znet::thermostat_setback::OverrideType >( overrideType ),
            static_cast< znet::thermostat_setback::Mode >( mode ),
            value.is_number() ? value.get< float >() : znet::thermostat_setback::SetbackValueOpt() );
    } );
}

void ZwaveAddon::setDevicesPollIntervals( const Json& params ) {
    LOGGER_TRACE( json_write( params ) );

    bool useFixedWakeUpInterval = params[ Resources::RPCParams::UseFixedWakeUpInterval ];
    uint32_t wakeUpInterval = params[ Resources::RPCParams::WakeUpInterval ];
    uint32_t alwaysOnDevicesInterval = params[ Resources::RPCParams::AlwaysOnDevicesInterval ];
    uint8_t attemptsCount = params[ Resources::RPCParams::AttemptsCount ];

    addZWaveTask( [this, useFixedWakeUpInterval, wakeUpInterval, alwaysOnDevicesInterval, attemptsCount]() {
        m_znet->updateReachableCheckTimeoutSettings(
            useFixedWakeUpInterval, wakeUpInterval, alwaysOnDevicesInterval, attemptsCount );
    } );
}

void ZwaveAddon::setUnresponsiveDeviceIgnoreInterval( const Json& params ) {
    LOGGER_TRACE( json_write( params ) );

    uint32_t interval = params[ Resources::RPCParams::Interval ];

    addZWaveTask( [this, interval]() { m_znet->updateUnresponsiveDeviceIgnoreInterval( interval ); } );
}

void ZwaveAddon::setDefaultDeviceConfigurationsFile( const Json& params ) {
    auto path = json_get_str( params, Resources::RPCParams::DeviceSettingFilePath );

    Log::info( cc::info( "Default device settings file path was set to " ) + cc::p( path ) );
    m_deviceSettingPath = path;
}

void ZwaveAddon::checkReachableStatus( const Json& params ) {
    LOGGER_TRACE( json_write( params ) );

    const uint8_t nodeId = params[ Resources::RPCParams::NodeId ];
    addZWaveTask( [=]() { m_znet->getNodeControlManager().checkReachableStatus( nodeId ); } );
}

void ZwaveAddon::lockAntiTheft( const Json& params ) {
    LOGGER_TRACE( json_write( params ) );

    const uint8_t nodeId = params[ Resources::RPCParams::NodeId ];
    const uint8_t channelId = params[ Resources::RPCParams::ChannelId ];
    const auto magicCode = json_get_str( params, Resources::RPCParams::MagicCode );
    const auto hint = json_get_str( params, Resources::RPCParams::Hint );

    addZWaveTask( [=]() {
        m_znet->getNodeControlManager().lockAntiTheft( nodeId,
                                                       channelId,
                                                       magicCode,
                                                       hint,
                                                       Resources::DeviceInfo::getManufacturerId(),
                                                       Resources::DeviceInfo::getZWaveAllianceLockingId() );
    } );
}

void ZwaveAddon::lockAntiTheftDefault( const Json& params ) {
    LOGGER_TRACE( json_write( params ) );

    const uint8_t nodeId = params[ Resources::RPCParams::NodeId ];
    const uint8_t channelId = params[ Resources::RPCParams::ChannelId ];
    const auto hint = AntiTheftHelper::generateHint();
    const auto magicCode = AntiTheftHelper::getMagicCode( hint );

    addZWaveTask( [=]() {
        m_znet->getNodeControlManager().lockAntiTheft( nodeId,
                                                       channelId,
                                                       *magicCode,
                                                       hint,
                                                       Resources::DeviceInfo::getManufacturerId(),
                                                       Resources::DeviceInfo::getZWaveAllianceLockingId() );
    } );
}

void ZwaveAddon::unlockAntiTheft( const Json& params ) {
    LOGGER_TRACE( json_write( params ) );

    const uint8_t nodeId = params[ Resources::RPCParams::NodeId ];
    const uint8_t channelId = params[ Resources::RPCParams::ChannelId ];
    const uint8_t classId = params[ Resources::RPCParams::ClassId ];
    const auto magicCode = json_get_str( params, Resources::RPCParams::MagicCode );

    addZWaveTask( [=]() { m_znet->getNodeControlManager().unlockAntiTheft( nodeId, channelId, classId, magicCode ); } );
}

void ZwaveAddon::unlockAntiTheftDefault( const Json& params ) {
    LOGGER_TRACE( json_write( params ) );

    const uint8_t nodeId = params[ Resources::RPCParams::NodeId ];
    const uint8_t channelId = params[ Resources::RPCParams::ChannelId ];
    const uint8_t classId = params[ Resources::RPCParams::ClassId ];
    const auto hint = znet::utils::hexToBytes( json_get_str( params, Resources::RPCParams::Hint ) );

    if( !hint ) {
        return;
    }

    const auto magicCode = AntiTheftHelper::getMagicCode( *hint );
    if( !magicCode ) {
        LOGGER_ERROR( "Cannot generate magic code from hint " + json_get_str( params, Resources::RPCParams::Hint ) );
        return;
    }

    addZWaveTask(
        [=]() { m_znet->getNodeControlManager().unlockAntiTheft( nodeId, channelId, classId, *magicCode ); } );
}

void ZwaveAddon::addDeviceSpecificConfiguration( const Json& params ) {
    const std::string deviceTypeId = params[ Resources::RPCParams::DeviceTypeId ];
    const Json configuration = params[ Resources::RPCParams::Configuration ];

    auto jsonNodeSettings = getDBClient().findOne(
        Resources::DB::TableNames::DeviceSettings, JSON( Resources::DB::Fields::Id << deviceTypeId ), nullptr );

    if( jsonNodeSettings.empty() ) {
        jsonNodeSettings[ Resources::DB::Fields::Id ] = deviceTypeId;
    }

    if( jsonNodeSettings.count( "configurations" ) == 0 ) {
        jsonNodeSettings[ "configurations" ] = JsonArray();
    }

    jsonNodeSettings[ "configurations" ].push_back( configuration );

    getDBClient().update( Resources::DB::TableNames::DeviceSettings, JSON_EMPTY_OBJ, jsonNodeSettings, true );
}

void ZwaveAddon::addDeviceSpecificAssociation( const Json& params ) {
    const std::string deviceTypeId = params[ Resources::RPCParams::DeviceTypeId ];
    const Json association = params[ Resources::RPCParams::Association ];

    auto jsonNodeSettings = getDBClient().findOne(
        Resources::DB::TableNames::DeviceSettings, JSON( Resources::DB::Fields::Id << deviceTypeId ), nullptr );

    if( jsonNodeSettings.empty() ) {
        jsonNodeSettings[ Resources::DB::Fields::Id ] = deviceTypeId;
    }

    if( jsonNodeSettings.count( "associations" ) == 0 ) {
        jsonNodeSettings[ "associations" ] = JsonArray();
    }

    jsonNodeSettings[ "associations" ].push_back( association );

    getDBClient().update( Resources::DB::TableNames::DeviceSettings, JSON_EMPTY_OBJ, jsonNodeSettings, true );
}

void ZwaveAddon::clearDeviceSpecific( const Json& params ) {
    getDBClient().clear( Resources::DB::TableNames::DeviceSettings );
}

void ZwaveAddon::setThermostatMode( const Json& params ) {
    const uint8_t nodeId = params[ Resources::RPCParams::NodeId ];
    const uint8_t channelId = params[ Resources::RPCParams::ChannelId ];
    const uint8_t mode = params[ Resources::RPCParams::Mode ];
    const auto manufacturerSpecificMode = 0x1F;

    if( mode == manufacturerSpecificMode ) {
        const auto manufacturerSpecificData =
            znet::utils::hexToBytes( json_get_str( params, Resources::RPCParams::ManufacturerSpecificData ) );

        if( !manufacturerSpecificData ) {
            LOGGER_ERROR( "Wrong manufacturer specific data " +
                          json_get_str( params, Resources::RPCParams::ManufacturerSpecificData ) );
            return;
        }

        addZWaveTask( [=]() {
            m_znet->getNodeControlManager().setMfgSpecificThermostatMode(
                nodeId, channelId, *manufacturerSpecificData );
        } );
    }
    else {
        addZWaveTask( [=]() { m_znet->getNodeControlManager().setThermostatMode( nodeId, channelId, mode ); } );
    }
}

void ZwaveAddon::synchronizeDeviceTime( const Json& params ) {
    const uint8_t nodeId = params[ Resources::RPCParams::NodeId ];

    addZWaveTask( [=]() { m_znet->getNodeControlManager().synchronizeDeviceTime( nodeId ); } );
}

void ZwaveAddon::checkNodeValues( const Json& params ) {
    LOGGER_TRACE( json_write( params ) );

    const uint8_t nodeId = params[ Resources::RPCParams::NodeId ];
    const auto requestDelay = std::chrono::milliseconds{ params[ Resources::RPCParams::RequestDelay ] };
    const auto& ccToCheck = params[ Resources::RPCParams::CcToCheck ];

    std::vector< std::pair< uint8_t, uint8_t > > dataToCheck;
    for( auto pair : ccToCheck ) {
        dataToCheck.push_back(
            std::make_pair( pair[ "channel_id" ].get< uint8_t >(), pair[ "class_id" ].get< uint8_t >() ) );
    }

    addZWaveTask( [=]() { m_znet->getNodeControlManager().checkNodeValues( nodeId, requestDelay, dataToCheck ); } );
}

template < typename ZClassType >
std::shared_ptr< ZClassType > ZwaveAddon::findCommandClassSafe( znet_node_id_t nodeId,
                                                                znet_node_channel_id_t channelId,
                                                                const char* methodName ) {
    return std::static_pointer_cast< ZClassType >(
        findCommandClassSafe( nodeId, channelId, ZClassType::ID::value, methodName ) );
}

std::shared_ptr< znet::ZClass > ZwaveAddon::findCommandClassSafe( znet_node_id_t nodeId,
                                                                  znet_node_channel_id_t channelId,
                                                                  znet_command_class_t classId,
                                                                  const char* methodName ) {
    auto node = m_nodeManager->findNode( nodeId );
    if( !node ) {
        LOGGER_ERROR( std::string( methodName ) + ": cannot find node " + std::to_string( nodeId ) );
        return {};
    }

    if( node->isRediscoverStarted() ) {
        LOGGER_ERROR( std::string( methodName ) + ": cannot do anything with rediscovering node " +
                      std::to_string( nodeId ) );
        return {};
    }

    auto cc = node->getCommandClass( channelId, classId );

    if( !cc ) {
        LOGGER_ERROR( std::string( methodName ) + ": cc " + std::to_string( classId ) + " not found for node " +
                      std::to_string( nodeId ) );
    }

    return cc;
}

}  // namespace Zwave
}  // namespace HubPlatform
