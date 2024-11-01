#include "networkd/gsm/nm_gsm_manager.h"
#include "networkd/resources/nm_resources.h"
#include "networkd/gsm/nm_gsm.h"
#include "networkd/gsm/nm_gsm_model.h"

#include "system_utils/system.h"
#include "system_utils/filesystem.h"
#include "common_utils/cpp14.h"
#include "log_utils/ezlo_log.h"
#include "tiny/include/tiny/tiny_consolecolors.h"
#include "database_client/dbqueryparser.h"
#include "json_utils/json_utils.h"
#include "base_utils/utils.h"
#include "jsonrpc/error_codes.h"

#include <memory>
#include <functional>
#include <utility>
#include <algorithm>
#include <cassert>
#include <vector>
#include <numeric>
#include <cstring>
#include <thread>
#include <chrono>
#include <tuple>

namespace HubPlatform {
namespace Network {

namespace {
using namespace GSM;

std::vector< const char* > onboardModems{
#ifdef __HUB_TYPE_H2_SECURE__
    Model::QUECTEL_EC25
#endif
};

void powerOnOnboardModems() {
    LOGGER_DEBUG_RAW( cc::debug( "Powering ON embedded modems" ) );
    std::vector< std::unique_ptr< GSMModem > > modems;
    for( auto& model : onboardModems )
        modems.emplace_back( createGSMModemNoLinkedToPhysical( model ) );

    for( auto& modem : modems ) {
        modem->powerOn();
    }
}

void powerOffOnboardModems() {
    std::vector< std::unique_ptr< GSMModem > > modems;
    for( auto& model : onboardModems )
        modems.emplace_back( createGSMModemNoLinkedToPhysical( model ) );

    for( auto& modem : modems )
        modem->powerOff();
}

std::string getTTYDevice( const std::string& devicePath, int portNumber ) {
    std::string subDeviceEntryName;
    std::string ttyDevice;

    filesystem::iterateFileSystemEntry(
        devicePath, [&subDeviceEntryName, portNumber]( const std::string& basePath, const std::string& entryName ) {
            if( entryName.find( ":1." + std::to_string( portNumber ) ) != std::string::npos ) {
                subDeviceEntryName = entryName;
                return false;
            }
            return true;
        } );

    if( subDeviceEntryName.empty() ) {
        return "";
    }

    filesystem::iterateFileSystemEntry( devicePath + '/' + subDeviceEntryName,
                                        [&ttyDevice]( const std::string& basePath, const std::string& entryName ) {
                                            if( entryName.find( "ttyUSB" ) != std::string::npos ) {
                                                ttyDevice = entryName;
                                                return false;
                                            }
                                            return true;
                                        } );

    return "/dev/" + ttyDevice;
}

std::pair< std::string, std::string > getTTYDevices( const std::string& devicePath ) {
    // use a second port for control (at commands) and a third one for data (ppp link), indexing starts from 0
    return { getTTYDevice( devicePath, 2 ), getTTYDevice( devicePath, 3 ) };
}

bool isModemEmbedded( const std::string& devicePath ) {
    return devicePath == std::string( "/sys/devices/platform/soc/" ) + Resources::GSM::EmbeddedCellularModemPath;
}

std::unique_ptr< GSMModem > createGSMModemFromPath( const NetworkConfigFileManager::MobileConfiguration& configuration,
                                                    const std::string& devicePath,
                                                    const std::string& ownerName ) {
    std::string controlTTY, dataTTY;
    const auto ttys = getTTYDevices( devicePath );
    std::tie( controlTTY, dataTTY ) = ttys;

    if( controlTTY.empty() || dataTTY.empty() ) {
        throw GSMModemException( "Failed to get a tty device names" );
    }

    LOGGER_INFO_RAW( cc::info( "Control tty device is " ) + cc::note( controlTTY ) +
                     cc::info( ", data tty device is " ) + cc::note( dataTTY ) + cc::info( ", interface: " ) +
                     cc::note( ownerName ) );

    auto ttyExist = [&controlTTY]() { return filesystem::isExist( controlTTY ); };
    if( !waitForCondition( ttyExist, std::chrono::seconds( 8 ), std::chrono::seconds( 1 ) ) ) {
        throw GSMModemFileDoesNotExist( controlTTY );
    }

    // Modem tty file requires some time to be operational
    std::this_thread::sleep_for( std::chrono::seconds( 8 ) );

    auto modem = createGSMModem( configuration,
                                 getModemModelByDevicePath( devicePath ),
                                 isModemEmbedded( devicePath ),
                                 { controlTTY, dataTTY, Resources::GSM::GSMModemBaudRate, true },
                                 ownerName );
    modem->startSession();

    return modem;
}
}  // namespace

GSMManager::GSMManager( IDbClient& dbClient,
                        const ProvisionDataUtils::ProvisionData& provisionData,
                        NetworkConfigManager& configManager,
                        const NetworkConfigFileManager::MobileConfiguration& configuration,
                        InterfaceMonitorFactory&& interfaceMonitorFactory,
                        InterfaceInfoListener& interfaceInfoListener,
                        Services::TaskLoop& taskLoop )
: m_dbClient( dbClient )
, m_provisionData( provisionData )
, m_configManager( configManager )
, m_configuration( configuration )
, m_interfaceMonitorFactory( std::move( interfaceMonitorFactory ) )
, m_interfaceInfoListener( interfaceInfoListener )
, m_taskLoop( taskLoop )
, m_embeddedModemWatchdog( taskLoop, [&]() { embeddedModemWatchdog(); } )
, m_eventQueue( taskLoop ) {}

GSMManager::~GSMManager() {
    m_embeddedModemWatchdog.stop();
}

void GSMManager::start() {
    powerOnOnboardModems();
    m_embeddedModemWatchdog.start( std::chrono::seconds( m_configuration.m_modemWatchdogTimer ), true );
}

/* sync gsm interfaces with existent gsm modems
 * note: hm notifies only about ACTUAL changes on a usb bus, so if we reboot without unplugging a usb device
 * no events will be sent, for such cases we synchronize gsm interfaces with records in the db */
void GSMManager::syncInterfaces() {
    const JsonObject fieldsToReturn = JSON( db::ID_FIELD_NAME << "id" );
    const auto data = m_dbClient.findAll( GSMManagerInterfacesDBName, JSON_EMPTY_OBJ, &fieldsToReturn );

    for( const auto& device : data ) {
        auto devicePath = json_get_str( device, "id" );

        /* check does modem still exist */
        if( !isMobileModem( devicePath ) ) {
            LOGGER_DEBUG_RAW( cc::debug( "Remove a mobile interface " ) + cc::note( devicePath ) +
                              cc::debug( " db entry as it doesn't map to a real modem" ) );
            m_dbClient.remove( GSMManagerInterfacesDBName, JSON( db::ID_FIELD_NAME << devicePath ) );
        }
        else {
            addGSMInterface( devicePath );
        }
    }
}

void GSMManager::restoreDynamicInterfaces() {
    syncInterfaces();
}

bool GSMManager::isMobileModem( const std::string& devicePath ) {
    const auto usbDesc = System::getUSBDescription( devicePath );
    const auto usbDescStr = usbDesc.idVendor + ':' + usbDesc.idProduct;
    for( auto& modem : gsmModemList() )
        if( modem[ "usbDescription" ] == usbDescStr )
            return true;
    return false;
}

bool GSMManager::addGSMInterface( const std::string& devicePath ) {
    GSMInterfacePtr interface;

    LOGGER_INFO( cc::info( "Adding GSM modem with device path: " ) + cc::note( devicePath ) );

    std::unique_lock< std::mutex > lock( m_interfaceMutex );
    if( m_dynamicGSMInterfaces.find( devicePath ) != m_dynamicGSMInterfaces.end() ) {
        LOGGER_ERROR_RAW( cc::error( "Attempt to add already added mobile interface, device: " ) +
                          cc::note( devicePath ) + cc::error( ", check if modem remove event is handled properly" ) );
        return true;
    }

    try {
        auto interfaceNumber = m_interfaceNumberManager.allocateInterfaceNumber( isModemEmbedded( devicePath ) );
        auto gsmModem = createGSMModemFromPath(
            m_configuration,
            devicePath,
            Resources::MobileBaseInterfaceName + std::to_string( interfaceNumber->getInterfaceNumber() ) );

        interface = std::make_shared< GSMInterface >( m_dbClient,
                                                      m_provisionData,
                                                      m_configManager,
                                                      m_interfaceMonitorFactory,
                                                      m_interfaceInfoListener,
                                                      m_taskLoop,
                                                      std::move( interfaceNumber ),
                                                      std::move( gsmModem ),
                                                      m_configuration );

        m_dynamicGSMInterfaces.emplace( devicePath, interface );
        lock.unlock();

        m_dbClient.insert( GSMManagerInterfacesDBName,
                           JSON( db::ID_FIELD_NAME << devicePath << "deviceName" << interface->getDeviceName() ) );

        LOGGER_INFO_RAW( cc::info( "Mobile interface " ) + cc::note( interface->getInterfaceName() ) +
                         cc::info( " has been created, device: " ) + cc::note( devicePath ) +
                         cc::info( ", interface is " ) +
                         cc::note( ( isModemEmbedded( devicePath ) ? "embedded" : "not embedded" ) ) );

        interface->start( interface );

        onInterfaceCreated( interface->getInterfaceName(),
                            devicePath,
                            getModemModelByDevicePath( devicePath ),
                            isModemEmbedded( devicePath ) );

        return true;
    }
    catch( const std::exception& e ) {
        LOGGER_ERROR_RAW( cc::error( "Failed to add a mobile interface, device: " ) + cc::note( devicePath ) +
                          cc::error( ", " ) + cc::note( e.what() ) );
        if( interface ) {
            try {
                onInterfaceCreationFailed( devicePath, isModemEmbedded( devicePath ) );
                interface->rebootModem();
            }
            catch( ... ) {
                LOGGER_ERROR_RAW( cc::error( "Failed to reboot modem, device " ) + cc::note( devicePath ) );
            }
            removeGSMInterface( devicePath );
        }
    }

    return false;
}

void GSMManager::removeGSMInterface( const std::string& devicePath ) {
    LOGGER_INFO( cc::info( "Removing GSM modem with device path: " ) + cc::note( devicePath ) );

    if( m_dynamicGSMInterfaces.find( devicePath ) == m_dynamicGSMInterfaces.end() ) {
        LOGGER_WARN_RAW( cc::warn( "Attempt to remove a non-existent mobile interface, device: " ) +
                         cc::note( devicePath ) );
        return;
    }

    auto interfaceName = m_dynamicGSMInterfaces[ devicePath ]->getInterfaceName();

    m_dynamicGSMInterfaces.erase( devicePath );

    m_dbClient.remove( GSMManagerInterfacesDBName, JSON( db::ID_FIELD_NAME << devicePath ) );

    LOGGER_INFO_RAW( cc::info( "Mobile interface " ) + cc::note( interfaceName ) +
                     cc::info( " has been removed, device: " ) + cc::note( devicePath ) );

    onInterfaceDestroyed( interfaceName, devicePath, isModemEmbedded( devicePath ) );
}

void GSMManager::addModemEventToQueue( const std::string& eventType, const std::string& devicePath ) {
    if( eventType == "add" ) {
        LOGGER_INFO( cc::info( "Recieved event to add GSM modem with device path: " ) + cc::note( devicePath ) );
        m_eventQueue.addEvent( [this, devicePath]() { addGSMInterface( devicePath ); }, std::chrono::seconds( 5 ) );
    }
    else if( eventType == "remove" ) {
        LOGGER_INFO( cc::info( "Recieved event to remove GSM modem with device path: " ) + cc::note( devicePath ) );
        m_eventQueue.addEvent( [this, devicePath]() { removeGSMInterface( devicePath ); }, std::chrono::seconds( 0 ) );
    }
    else {
        LOGGER_WARN_RAW( cc::warn( "GSMManager received unknown event type: " ) + cc::warn( eventType ) );
    }
}

bool GSMManager::isGSMInterface( const std::string& interfaceName ) const {
    bool result = false;

    foreachGSMInterface( [&]( const GSMInterface& gsmInterface ) {
        if( interfaceName == gsmInterface.getInterfaceName() ) {
            result = true;
        }
    } );

    return result;
}

const GSMInterface& GSMManager::getGSMInterface( const std::string& interfaceName ) const {
    std::lock_guard< std::mutex > lock( m_interfaceMutex );
    for( const auto& interfaceData : m_dynamicGSMInterfaces ) {
        if( interfaceName == interfaceData.second->getInterfaceName() )
            return *interfaceData.second;
    }
    throw InvalidGSMInterface( "Invalid mobile interface: " + interfaceName );
}

GSMInterface& GSMManager::getGSMInterface( const std::string& interfaceName ) {
    std::lock_guard< std::mutex > lock( m_interfaceMutex );
    for( const auto& interfaceData : m_dynamicGSMInterfaces ) {
        if( interfaceName == interfaceData.second->getInterfaceName() )
            return *interfaceData.second;
    }
    throw InvalidGSMInterface( "Invalid mobile interface: " + interfaceName );
}

void GSMManager::resetGSMInterfaces() {
    foreachGSMInterface( []( GSMInterface& gsmInterface ) { gsmInterface.reset(); } );
}

void GSMManager::foreachGSMInterface( const std::function< void( GSMInterface& ) >& callback ) {
    for( auto& gsmInterface : m_dynamicGSMInterfaces ) {
        callback( *gsmInterface.second );
    }
}

void GSMManager::foreachGSMInterface( const std::function< void( const GSMInterface& ) >& callback ) const {
    for( const auto& gsmInterface : m_dynamicGSMInterfaces ) {
        callback( *gsmInterface.second );
    }
}

bool GSMManager::noInterfaces() const {
    return m_dynamicGSMInterfaces.empty();
}

bool GSMManager::noEmbeddedInterfaces() const {
    for( const auto& gsmInterface : m_dynamicGSMInterfaces ) {
        if( gsmInterface.second->getGenericModem().isEmbedded() ) {
            return false;
        }
    }

    return true;
}

void GSMManager::embeddedModemWatchdog() {
    std::vector< std::unique_ptr< GSMModem > > modems;
    for( auto& model : onboardModems )
        modems.emplace_back( createGSMModemNoLinkedToPhysical( model ) );

    std::lock_guard< std::mutex > lock( m_interfaceMutex );
    std::set< std::pair< std::string, bool > > knownModems;
    foreachGSMInterface( [&knownModems]( const GSMInterface& interface ) {
        auto deviceInfo = interface.getDeviceInfo();
        knownModems.emplace( json_get_str( deviceInfo, "model" ), json_get_bool( deviceInfo, "embedded" ) );
    } );

    for( auto& modem : modems ) {
        auto deviceInfo = modem->getDeviceInfo();
        if( knownModems.count( { json_get_str( deviceInfo, "model" ), true } ) == 0 ) {
            LOGGER_WARN_RAW( cc::warn( "Reboot unused embedded modem: " ) + deviceInfo.dump() );
            modem->reboot();
        }
    }
}

// TODO: add support for multi-embedded modems (if any)
void GSMManager::onInterfaceCreated( const std::string& interfaceName,
                                     const std::string& devicePath,
                                     const std::string& model,
                                     bool embedded ) {
    if( embedded ) {
        m_embeddedModemWatchdog.stop();
    }
}

// TODO: add support for multi-embedded modems (if any)
void GSMManager::onInterfaceDestroyed( const std::string& interfaceName,
                                       const std::string& devicePath,
                                       bool embedded ) {
    if( embedded ) {
        m_embeddedModemWatchdog.start( std::chrono::seconds( m_configuration.m_modemWatchdogTimer ), true );
    }
}

// TODO: add support for multi-embedded modems (if any)
void GSMManager::onInterfaceCreationFailed( const std::string& devicePath, bool embedded ) {
    if( embedded ) {
        m_embeddedModemWatchdog.stop();
        m_embeddedModemWatchdog.start( std::chrono::seconds( m_configuration.m_modemWatchdogTimer ), true );
    }
}

Json GSMManager::handleGSMErrors( const std::exception& ex ) {
    if( auto clarifiedEx = dynamic_cast< const GSM::GSMModemNoSIMCard* >( &ex ) ) {
        return Errors::get( Errors::MODEM_SIM_NO_PRESENT );
    }
    if( auto clarifiedEx = dynamic_cast< const GSM::GSMModemPINCodeRequired* >( &ex ) ) {
        return Errors::get( Errors::MODEM_SIM_PIN_CODE_IS_REQUIRED );
    }
    if( auto clarifiedEx = dynamic_cast< const GSM::GSMModemInvalidPINCode* >( &ex ) ) {
        return Errors::get( Errors::MODEM_INVALID_PIN );
    }
    if( auto clarifiedEx = dynamic_cast< const GSM::GSMModemNotSupportedOperator* >( &ex ) ) {
        return Errors::get( Errors::MODEM_NOALLOWED_OPERATOR );
    }
    if( auto clarifiedEx = dynamic_cast< const GSM::GSMModemBadPPPAuthentication* >( &ex ) ) {
        return Errors::get( Errors::MODEM_BAD_AUTH );
    }
    if( auto clarifiedEx = dynamic_cast< const GSM::GSMModemBadInternetParameters* >( &ex ) ) {
        return Errors::get( Errors::MODEM_BAD_INTERNET_PARAMETERS );
    }
    if( auto clarifiedEx = dynamic_cast< const GSM::GSMModemSIMCardError* >( &ex ) ) {
        return Errors::get( Errors::MODEM_SIM_CARD_ERROR );
    }
    if( auto clarifiedEx = dynamic_cast< const GSM::GSMModemPUKCodeIsRequired* >( &ex ) ) {
        return Errors::get( Errors::MODEM_SIM_PUK_CODE_IS_REQUIRED );
    }
    if( auto clarifiedEx = dynamic_cast< const GSM::GSMModemPUKCodeLastAttempt* >( &ex ) ) {
        return Errors::get( Errors::MODEM_SIM_PUK_CODE_LAST_ATTEMPT );
    }
    if( auto clarifiedEx = dynamic_cast< const GSM::GSMOperationNotAllowed* >( &ex ) ) {
        return Errors::get( Errors::MODEM_OPERATION_NOALLOWED );
    }
    if( auto clarifiedEx = dynamic_cast< const GSM::GSMModemHangs* >( &ex ) ) {
        return Errors::get( Errors::MODEM_HUNG );
    }

    return JSON_EMPTY_OBJ;
}

GSMManager::EventQueue::EventQueue( Services::TaskLoop& taskLoop )
: m_taskLoop( taskLoop ) {}

void GSMManager::EventQueue::addEvent( Event&& event, const Delay& delay ) {
    std::lock_guard< std::mutex > lock( m_mutex );
    m_queue.emplace_back( std::move( event ), delay );
    if( m_queue.size() == 1 )
        scheduleEvent();
}

void GSMManager::EventQueue::processEvent() {
    std::unique_lock< std::mutex > lock( m_mutex );
    if( m_queue.empty() ) {
        LOGGER_WARN_RAW( cc::warn( "Modem event was removed before processing" ) );
        return;
    }
    auto& event = m_queue.front().first;
    lock.unlock();

    try {
        event();
    }
    catch( const std::exception& e ) {
        LOGGER_WARN_RAW( cc::warn( "Exception occurs while processing modem event: " ) + cc::note( e.what() ) );
    }
    catch( ... ) {
        LOGGER_WARN_RAW( cc::warn( "Unknown exception was caught during processing GSM event" ) );
    }

    lock.lock();
    m_queue.pop_front();
    if( !m_queue.empty() ) {
        scheduleEvent();
    }
}

void GSMManager::EventQueue::scheduleEvent() {
    if( m_queue.empty() )
        return;

    auto& delay = m_queue.front().second;
    m_taskLoop.createPostponedTask( [this]( Services::PostponedTask& ) { processEvent(); } )->start( delay, false );
}

JsonObject GSMManager::getModemInfo() const {
    JsonObject modemsInfo;

    foreachGSMInterface( [&modemsInfo]( const GSMInterface& gsmInterface ) {
        modemsInfo.push_back( gsmInterface.getReporterModemInfo() );
    } );

    return modemsInfo;
}

}  // namespace Network
}  // namespace HubPlatform
