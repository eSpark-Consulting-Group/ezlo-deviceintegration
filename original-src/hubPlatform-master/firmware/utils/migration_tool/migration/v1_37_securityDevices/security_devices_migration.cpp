#include "security_devices_migration.h"
#include "resources.h"

namespace HubPlatform {
namespace MigrationTool {
namespace v1_37_securityDevices {

class MigrationError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

SecurityDeviceMigration::SecurityDeviceMigration( IDbClient& dbClient )
: m_dbClient( dbClient ) {}

size_t SecurityDeviceMigration::recordsInserted() const {
    return m_recordsInserted;
}

size_t SecurityDeviceMigration::recordsDeleted() const {
    return m_recordsDeleted;
}

size_t SecurityDeviceMigration::securityDevicesAdded() const {
    return m_securityDevicesAdded;
}

size_t SecurityDeviceMigration::removedFromDisarmedDevices() const {
    return m_removedFromDisarmedDevices;
}

size_t SecurityDeviceMigration::insertedInModes() const {
    return m_insertedInModes;
}

size_t SecurityDeviceMigration::changedArmedTo( bool value ) const {
    return value ? m_changedArmedToTrue : m_changedArmedToFalse;
}

Json SecurityDeviceMigration::fetchDevice( const std::string& deviceId ) const {
    return m_dbClient.findOne( Tables::DevicesDb, JSON( Fields::id << deviceId ), nullptr );
}

bool SecurityDeviceMigration::migrate() {
    try {
        return tryMigrate();
    }
    catch( const MigrationError& ex ) {
        Log::error( cc::error( "Security devices Migration error: " ) + cc::note( ex.what() ) );
        return false;
    }
    catch( const std::exception& ex ) {
        Log::error( cc::error( "Security devices exception: " ) + cc::note( ex.what() ) + cc::error( ", type: " ) +
                    cc::note( typeid( ex ).name() ) );
        return false;
    }
}

void SecurityDeviceMigration::updateDeviceArmedField( const std::string& deviceId, bool armed ) {
    m_dbClient.update( Tables::DevicesDb,
                       JSON( Fields::id << deviceId << Fields::armed << JSON( "$exists" << true ) ),
                       JSON( "$set" << JSON( Fields::armed << armed ) ) );
}

bool SecurityDeviceMigration::tryMigrate() {
    LOGGER_DEBUG_RAW( "SecurityDeviceMigration::migrate called" );

    std::vector< Json > devicesToAdd;
    std::vector< Json > devicesToRemove;

    auto securityThreatItems = fetchSecurityItems();
    auto houseModes = fetchHouseModes();
    /** pair of {deviceId, { _id: string(deviceId), security_threat: string(itemId) }} */
    auto securityDevices = fetchSecurityDevices();

    std::map< std::string, std::string > devicesWithSecurity;
    for( const auto& itemIdItem : securityThreatItems ) {
        devicesWithSecurity.insert( { json_get_str( itemIdItem.second, Fields::deviceId ), itemIdItem.first } );
    }

    // add to security devices all devices with security-threat items (place in queue).
    for( const auto& securityItem : securityThreatItems ) {
        LOGGER_TRACE( "securityItem: " + cc::j( securityItem.second ) );

        auto itemId = json_get_str( securityItem.second, Fields::id );
        auto deviceId = json_get_str( securityItem.second, Fields::deviceId );
        auto device = fetchDevice( deviceId );

        devicesToAdd.push_back( JSON( Fields::deviceId << deviceId << Fields::itemId << itemId ) );
    }

    // mark for removal all armed devices without security_threat items
    LOGGER_TRACE( "Check armedDeviceIds..." );
    auto armedDeviceIds = fetchArmedDevices();
    for( const auto& armedDeviceId : armedDeviceIds ) {
        LOGGER_TRACE( "      armedDeviceId " + cc::j( armedDeviceId ) );
        if( securityDevices.count( armedDeviceId ) == 0 ) {
            // check if it's not in devices to add first
            if( devicesWithSecurity.count( armedDeviceId ) == 0 ) {
                LOGGER_TRACE( "marked for removal from house modes: " + armedDeviceId );
                devicesToRemove.push_back( JSON( Fields::deviceId << armedDeviceId ) );
            }
            else {
                LOGGER_TRACE( "Was not marked for removal, has security_threat item: " + armedDeviceId );
            }
        }
    }

    // remove devices
    {
        std::vector< std::string > deviceIds;
        // remove a device from houseModes[].disarmed records
        for( const auto& it : devicesToRemove ) {
            LOGGER_TRACE( "  devicesToRemove: " + cc::j( it ) );
            auto deviceId = json_get_str( it, Fields::deviceId );
            deviceIds.push_back( deviceId );

            // set device.armed = false
            updateDeviceArmedField( deviceId, false );
            m_changedArmedToFalse++;
        }
        if( !deviceIds.empty() ) {
            removeFromDisarmedDevices( deviceIds );
            m_removedFromDisarmedDevices = deviceIds.size();
        }
    }

    // add devices
    {
        LOGGER_TRACE( "devicesToAdd: " );
        for( const auto& deviceItem : devicesToAdd ) {
            LOGGER_TRACE( "  devicesToAdd: " + cc::j( deviceItem ) );
            auto deviceId = json_get_str( deviceItem, Fields::deviceId );
            auto itemId = json_get_str( deviceItem, Fields::itemId );

            if( deviceId.empty() || itemId.empty() )
                throw MigrationError( "add security device error: empty device or item id" );

            addSecurityDevice( deviceId, itemId );

            auto device = fetchDevice( deviceId );
            addToHouseModes( deviceId, device );
        }
    }

    return true;
}

SecurityDeviceMigration::Collection SecurityDeviceMigration::fetchSecurityItems() const {
    auto items = m_dbClient.findAll(
        Tables::ItemsDb, JSON( Fields::name << Values::securityThreat << Fields::hasGetter << true ), nullptr );
    Collection results;
    for( const auto& item : items ) {
        results.insert( { json_get_str( item, Fields::id ), item } );
    }
    return results;
}

SecurityDeviceMigration::Collection SecurityDeviceMigration::fetchSecurityDevices() const {
    auto devices = m_dbClient.findAll( Tables::HouseModesSecurityDevicesDb, JSON_EMPTY_OBJ, nullptr );
    Collection results;
    for( const auto& device : devices ) {
        results.insert( { json_get_str( device, Fields::id ), device } );
    }
    return results;
}

SecurityDeviceMigration::Collection SecurityDeviceMigration::fetchHouseModes() const {
    auto records = m_dbClient.findAll( Tables::HouseModesDb, JSON_EMPTY_OBJ, nullptr );
    Collection results;
    for( const auto& mode : records ) {
        results.insert( { json_get_str( mode, Fields::id ), mode } );
    }
    return results;
}

std::string SecurityDeviceMigration::fetchCurrentModeId() const {
    Json coredSettings =
        m_dbClient.findOne( Tables::CoredSettingsDb, JSON( Fields::id << Fields::houseMode ), nullptr );
    return json_get_str( coredSettings, Fields::houseMode );
}

void SecurityDeviceMigration::addSecurityDevice( const std::string& deviceId, const std::string& securityThreatId ) {
    m_dbClient.insert( Tables::HouseModesSecurityDevicesDb,
                       JSON( Fields::id << deviceId << Values::securityThreat << securityThreatId ) );
    m_securityDevicesAdded++;
}

std::set< std::string > SecurityDeviceMigration::fetchArmedDevices() {
    std::set< std::string > results;

    Json fields = JSON( Fields::id << 1 );
    auto devices = m_dbClient.findAll( Tables::DevicesDb, JSON( Fields::armed << true ), &fields );
    for( const auto& device : devices ) {
        results.insert( json_get_str( device, Fields::id ) );
    }
    return results;
}

void SecurityDeviceMigration::removeFromDisarmedDevices( const std::vector< std::string >& deviceIds ) {
    LOGGER_TRACE( "" + cc::j( Json( deviceIds ) ) );

    // local house modes; remove all ids from deviceIds from mode.disarmed array; save back;
    auto houseModes = fetchHouseModes();

    for( auto& it : houseModes ) {
        const auto& modeId = it.first;
        auto& mode = it.second;  // modify in-place
        LOGGER_TRACE( "      Process modeId=" + modeId );
        LOGGER_TRACE( cc::j( mode ) );

        if( removeFromModeDisarmed( mode, deviceIds ) ) {
            // update db record for the modeId
            updateHouseModeRecord( modeId, mode );
        }
    }
}

bool SecurityDeviceMigration::removeFromModeDisarmed( Json& mode, const std::vector< std::string >& deviceIds ) {
    LOGGER_TRACE( "mode: " + cc::j( mode ) );
    if( deviceIds.empty() )
        return false;

    std::set< std::string > devicesFromDb;

    bool isOk = false;
    {
        JsonArray strings;
        if( json_get_arr( mode, Fields::disarmedDevices, strings ) ) {
            try {
                fromJsonStringArray( strings,
                                     [&devicesFromDb]( const std::string& value ) { devicesFromDb.emplace( value ); } );
                isOk = true;
            }
            catch( ... ) {
            }
        }
    }

    if( isOk && !devicesFromDb.empty() ) {
        std::set< std::string > toDelete( begin( deviceIds ), end( deviceIds ) );

        std::vector< std::string > difference;
        std::set_difference( begin( devicesFromDb ),
                             end( devicesFromDb ),
                             begin( toDelete ),
                             end( toDelete ),
                             std::back_inserter( difference ) );

        mode[ Fields::disarmedDevices ] = difference;

        LOGGER_TRACE( "After deleting: " + cc::j( mode ) );
        return difference.size() != devicesFromDb.size();
    }

    return false;
}

bool SecurityDeviceMigration::addToModeDisarmed( Json& mode, const std::vector< std::string >& deviceIds ) {
    LOGGER_TRACE( "mode: " + cc::j( mode ) );
    if( deviceIds.empty() )
        return false;

    std::set< std::string > devicesFromDb;

    bool isOk = false;
    {
        JsonArray strings;
        if( json_get_arr( mode, Fields::disarmedDevices, strings ) ) {
            try {
                fromJsonStringArray( strings,
                                     [&devicesFromDb]( const std::string& value ) { devicesFromDb.emplace( value ); } );
                isOk = true;
            }
            catch( ... ) {
            }
        }
    }

    if( isOk ) {
        std::set< std::string > toAdd( begin( deviceIds ), end( deviceIds ) );

        std::vector< std::string > unitedDeviceIds;
        std::merge( begin( devicesFromDb ),
                    end( devicesFromDb ),
                    begin( toAdd ),
                    end( toAdd ),
                    std::back_inserter( unitedDeviceIds ) );

        mode[ Fields::disarmedDevices ] = unitedDeviceIds;

        LOGGER_TRACE( "After adding: mode.disarmedDevices = " + cc::j( mode[ Fields::disarmedDevices ] ) );
        return unitedDeviceIds.size() != devicesFromDb.size();
    }

    return false;
}

void SecurityDeviceMigration::updateHouseModeRecord( const std::string& modeId, const Json& mode ) {
    if( !modeId.empty() && !mode.empty() ) {
        LOGGER_TRACE( "To update modeId = " + modeId + " with mode: " + cc::j( mode ) );

        Json disarmedDevices = JSON_EMPTY_ARR;
        if( json_get( mode, Fields::disarmedDevices, disarmedDevices ) ) {
            m_dbClient.update( Tables::HouseModesDb,
                               JSON( Fields::id << modeId ),
                               JSON( "$set" << JSON( Fields::disarmedDevices << disarmedDevices ) ),
                               false,
                               false );
        }
    }
}

/** @brief Analyzes device for category/subcategory match to every house mode, analyzes houseModesOptions and decides
 * should the device be added to a mode.disarmedDevices */
void SecurityDeviceMigration::addToHouseModes( const std::string& deviceId, const Json& device ) {
    LOGGER_TRACE( cc::j( device ) );

    auto currentModeId = fetchCurrentModeId();
    auto houseModes = fetchHouseModes();
    auto category = json_get_str( device, Fields::category );
    auto subcategory = json_get_str( device, Fields::subcategory );

    std::set< std::string > disarmedModes;
    CategoriesIndex categoriesIndex;

    JsonArray disarmedOptions;
    {
        Json houseModeOptions;
        if( json_get( device, Fields::houseModeOptions, houseModeOptions ) ) {
            if( json_get_arr( houseModeOptions, Fields::disarmed, disarmedOptions ) ) {
                fromJsonStringArray( disarmedModes,
                                     [&disarmedModes]( const std::string& value ) { disarmedModes.insert( value ); } );
            }
        }
    }

    for( auto& it : houseModes ) {
        const auto& modeId = it.first;
        auto& mode = it.second;  // modify in-place
        LOGGER_TRACE( "      Process modeId=" + modeId );
        LOGGER_TRACE( cc::j( mode ) );

        bool armedInMode = false;
        if( !disarmedModes.empty() ) {
            armedInMode = disarmedModes.count( modeId ) == 0;
        }
        else {
            auto armedCategories = makeCategoriesIndex( mode );
            armedInMode = armedCategories.count( category ) && armedCategories[ category ].count( subcategory ) > 0;
        }

        LOGGER_TRACE( "modeId: " + modeId + ", deviceId: " + deviceId +
                      ", is armed in mode: " + cc::flag( armedInMode ) );
        if( !armedInMode && addToModeDisarmed( mode, { deviceId } ) ) {
            // update db record for the modeId
            updateHouseModeRecord( modeId, mode );
            m_insertedInModes++;
        }

        if( armedInMode && modeId == currentModeId ) {
            // To Andrey: should I update device.armed = true in migration?
            updateDeviceArmedField( deviceId, true );
            m_changedArmedToTrue++;
        }
    }
}

SecurityDeviceMigration::CategoriesIndex SecurityDeviceMigration::makeCategoriesIndex( const Json& mode ) {
    CategoriesIndex categoriesIndex;
    JsonArray armedCategories;

    if( json_get_arr( mode, Fields::armedCategories, armedCategories ) ) {
        for( auto& pair : armedCategories ) {
            auto category = json_get_str( pair, Fields::category );
            JsonArray subcategories;
            if( json_get_arr( pair, Fields::subcategories, subcategories ) ) {
                fromJsonStringArray( subcategories, [&categoriesIndex, &category]( const std::string& value ) {
                    categoriesIndex[ category ].insert( value );
                } );
            }
        }
    }

    return categoriesIndex;
}

}  // namespace v1_37_securityDevices
}  // namespace MigrationTool
}  // namespace HubPlatform