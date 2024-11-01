#include "hardware_manager/battery/bq27441g1/battery_manager.h"
#include "hardware_manager/battery/bq27441g1/classes.h"
#include "hardware_manager/battery/bq27441g1/op_config_register.h"

#include "update_manager/resources/um_resources.h"

#include "system_utils/filesystem.h"
#include "system_utils/environment.h"
#include "log_utils/ezlo_log.h"
#include "tiny/tiny_consolecolors.h"
#include "json_utils/json_utils.h"
#include "database_client/dbqueryparser.h"

#include <fstream>

namespace HubPlatform {
namespace HardwareManager {
namespace Bq27441g1 {

namespace {
const std::string BatteryDefaultId( "default" );
const std::string BatteryLearnedId( "learned" );
const auto BatteryInfoUpdateInterval = std::chrono::seconds( 2 );
const auto GaugeLearnInterval = std::chrono::hours( 12 );
}  // namespace

BatteryManager::BatteryManager( Services::TaskLoop& taskLoop, IDbClient& dbClient )
: m_taskLoop( taskLoop )
, m_dbClient( dbClient )
, m_batteryInfoCheckTask( m_taskLoop.createPostponedTask( [this]( Services::PostponedTask& ) {
    m_lastInfo = readInfo();
    m_batteryInfoChangedCallback( m_lastInfo );
} ) )
, m_lastMeasuredCurrentsSum( 0 )
, m_lastStatus( BatteryStatus::Missing ) {}

void BatteryManager::setUp( BatteryInfoChangedCallback callback ) {
    m_batteryInfoChangedCallback = std::move( callback );
    m_batteryInfoCheckTask->start( std::chrono::seconds( 0 ), BatteryInfoUpdateInterval );
}

void BatteryManager::tearDown() {
    m_batteryInfoCheckTask->stop();
}

void BatteryManager::reset() {
    try {
        Log::info( cc::info( "Resetting gauge" ) );
        m_battery.control( ControlFunction::RESET );
        m_dbClient.clear( TempBatteryInfoDBName );
        m_dbClient.clear( BatteryLearnedDBName );
        m_dbClient.clear( BatteryDefaultDBName );
    }
    catch( const CommunicationError& error ) {
        Log::error( error.what() );
    }
}

BatteryInfo BatteryManager::getInfo() const {
    return m_lastInfo;
}

bool BatteryManager::isHardwareBased() const {
    return true;
}

BatteryInfo BatteryManager::readInfo() {
    BatteryInfo info;

    try {
        if( !m_battery.ensureOpen() ) {
            info.status = BatteryStatus::Missing;
            return info;
        }

        auto controlStatus = m_battery.controlReadStatus();
        if( !controlStatus.INITCOMP ) {
            return info;
        }

        if( !service() ) {
            return info;
        }

        auto current = m_battery.readAverageCurrent();
        auto remainingCapacity = m_battery.readRemainingCapacity();

        if( current == 0 ) {
            info.status = BatteryStatus::Full;
        }
        else {
            if( current < 0 ) {
                info.status = BatteryStatus::Discharging;
            }
            else {
                info.status = BatteryStatus::Charging;
            }
        }

        if( m_lastStatus != info.status ) {
            m_lastMeasuredCurrentsSum = 0;
            m_lastMeasuredCurrents.clear();
        }
        m_lastStatus = info.status;

        m_lastMeasuredCurrents.push_back( current );
        m_lastMeasuredCurrentsSum += m_lastMeasuredCurrents.back();
        if( m_lastMeasuredCurrents.size() > 150 /* 5 minutes with 2 sec interval */ ) {
            m_lastMeasuredCurrentsSum -= m_lastMeasuredCurrents.front();
            m_lastMeasuredCurrents.pop_front();
        }
        int32_t averageCurrent = m_lastMeasuredCurrentsSum / static_cast< int32_t >( m_lastMeasuredCurrents.size() );

        switch( info.status ) {
        case BatteryStatus::Discharging:
            info.remainingTime = remainingCapacity * 60 / -averageCurrent;
            break;
        case BatteryStatus::Charging:
            info.remainingTime = ( m_battery.readFullChargeCapacity() - remainingCapacity ) * 60 / averageCurrent;
            break;
        default:
            break;
        }

        info.stateOfCharge = m_battery.readStateOfCharge();
        info.health = m_battery.readStateOfHealth().first;

        {
            // NOTE: keep this logic in sync with 909-battery_test.patch in openwrt_platform repository
            constexpr uint16_t DSG_DRAINED_VOLTAGE = 3050;
            constexpr uint16_t DSG_MIN_VOLTAGE = 3200;
            constexpr uint16_t CHG_MIN_VOLTAGE = 3400;
            uint16_t voltage = m_battery.readVoltage();
            auto flags = m_battery.readFlags();
            bool vok = voltage >= ( flags.DSG ? DSG_MIN_VOLTAGE : CHG_MIN_VOLTAGE );
            info.depleted = !vok;

            if( flags.DSG && voltage <= DSG_DRAINED_VOLTAGE ) {
                info.status = BatteryStatus::Drained;
                info.remainingTime = 0;
                info.stateOfCharge = 0;
            }
        }

        if( Log::getLogLevel() == Log::TRACE ) {
            static Json prevDump;
            auto newDump = m_battery.basicDump();
            auto diff = jsonDiff( prevDump, newDump );
            if( !diff.empty() ) {
                Log::trace( cc::trace( "Gauge dump diff: " ) + cc::j( diff ) );
            }
            prevDump = newDump;
        }
    }
    catch( const CommunicationError& error ) {
        Log::error( error.what() );
        info = {};
        info.status = BatteryStatus::Missing;
    }
    return info;
}

bool BatteryManager::service() {
    // TODO: utilize SOC_INT function of GPOUT pin of battery chip
    auto flags = m_battery.readFlags();
    if( flags.ITPOR ) {
        // RAM configuration registers have been reset to default values
        updateConfiguration();
        return false;
    }

    // Reset chip if we are asked to, or we do not have default configuration snapshot
    auto flagFilePath =
        Environment::getPersistentFolder() + "/" + UpdateManager::Resources::ResetBatteryRequiredFileName;
    if( filesystem::isFileExist( flagFilePath ) ) {
        filesystem::removeFile( flagFilePath.c_str() );
        reset();
        return false;
    }

    auto defaultConfig =
        m_dbClient.findOne( BatteryDefaultDBName, JSON( db::ID_FIELD_NAME << BatteryDefaultId ), nullptr );
    if( defaultConfig.empty() ) {
        reset();
        return false;
    }

    auto currentCheck = std::chrono::steady_clock::now();
    static bool prevDSG = flags.DSG;
    static auto prevCheck = currentCheck;
    if( ( prevDSG != flags.DSG ) || ( currentCheck - prevCheck > GaugeLearnInterval ) ) {
        storeLearned();
        prevDSG = flags.DSG;
        prevCheck = currentCheck;
    }

    return true;
}

void BatteryManager::updateConfiguration() {
    Log::info( cc::info( "Gauge was reset, updating its configuration" ) );
    // TODO: unseal gauge if it is sealed first

    // Switch to manual battery insert/remove control
    {
        using namespace ConfigurationClass::RegistersSubclass;
        auto dataMemory = m_battery.updateDataMemory( ID, BlockIdx );

        auto oldOpConfig = OpConfigRegister( dataMemory->readH2( Offsets::OpConfig ) );
        if( oldOpConfig.BIE ) {
            auto newOpConfig = oldOpConfig;
            newOpConfig.BIE = false;
            dataMemory->rewriteH2( Offsets::OpConfig, oldOpConfig.toUint16(), newOpConfig.toUint16() );
        }
        dataMemory->commit();
    }

    // Update battery configuration
    auto configPath = Environment::getConfigsFolder() + "/battery.cfg";
    if( filesystem::isFileExist( configPath ) ) {
        std::ifstream file( configPath );
        auto config = Json::parse( file );
        m_battery.writeData( config );
    }

    // Save default configuration snapshot
    auto defaultConfig = m_battery.readLearned();
    defaultConfig[ db::ID_FIELD_NAME ] = BatteryDefaultId;
    m_dbClient.update( BatteryDefaultDBName, JSON_EMPTY_OBJ, defaultConfig, true );

    // NOTE: temporary disable applying learned configuration
    // Update battery learned configuration
    // auto learned = m_dbClient.findOne( BatteryLearnedDBName, JSON( db::ID_FIELD_NAME << BatteryLearnedId ), nullptr
    // ); if( !learned.empty() ) {
    //     learned.erase( db::ID_FIELD_NAME );
    //     m_battery.writeData( learned );
    // }
}

void BatteryManager::storeLearned() {
    auto defaultConfig =
        m_dbClient.findOne( BatteryDefaultDBName, JSON( db::ID_FIELD_NAME << BatteryDefaultId ), nullptr );
    if( !defaultConfig.empty() ) {
        defaultConfig.erase( db::ID_FIELD_NAME );
        auto learned = m_battery.readLearned();
        auto diff = jsonDiff( defaultConfig, learned );
        if( !diff.empty() ) {
            diff[ db::ID_FIELD_NAME ] = BatteryLearnedId;
            auto oldLearned =
                m_dbClient.findOne( BatteryLearnedDBName, JSON( db::ID_FIELD_NAME << BatteryLearnedId ), nullptr );
            auto learnedDiff = jsonDiff( oldLearned, diff );
            if( !learnedDiff.empty() ) {
                m_dbClient.update( BatteryLearnedDBName, JSON_EMPTY_OBJ, diff, true );
                Log::debug( cc::debug( "Learned gauge parameters: " ) + cc::j( diff ) );
            }
        }
    }
}

}  // namespace Bq27441g1
}  // namespace HardwareManager
}  // namespace HubPlatform
