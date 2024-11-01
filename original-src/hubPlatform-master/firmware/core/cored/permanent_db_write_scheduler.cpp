#include "cored/permanent_db_write_scheduler.h"
#include "cored/core_service.h"
#include "cored/model/resources/cored_resources.h"
#include "cored/db_modifier/item_db_modifier.h"
#include "cored/db_modifier/device_db_modifier.h"
#include "database_client/idbclient.h"
#include "json_utils/json_wrapper.h"
#include "log_utils/ezlo_log.h"
#include "cored/house_modes/resources/house_mode_resources.h"

namespace HubPlatform {
namespace CoreD {

PermanentDBWriteScheduler::PermanentDBWriteScheduler( Services::TaskLoop& loop,
                                                      ItemDbModifier& itemDbModifier,
                                                      DeviceDbModifier& deviceDbModifier )
: m_itemDbModifier( itemDbModifier )
, m_deviceDbModifier( deviceDbModifier )
, m_timer( loop, [this] { worker(); } ) {}

void PermanentDBWriteScheduler::doNow() {
    worker();
}

void PermanentDBWriteScheduler::schedule( const Services::TaskLoopTimer::Interval& interval ) {
    m_timer.start( interval, interval );
}

void PermanentDBWriteScheduler::cancelSchedule() {
    m_timer.stop();
}

void PermanentDBWriteScheduler::worker() {
    // Prevent simultaneous execution from different threads.
    bool expected = false;
    if( !m_isRunning.compare_exchange_strong( expected, true ) ) {
        Log::debug( "Is already writing, nothing to do" );
        return;
    }

    struct Guard {
        using ValueType = decltype( m_isRunning );

        Guard( ValueType& val )
        : val( val ) {}

        ~Guard() {
            val = false;
        }
        ValueType& val;
    };
    Guard guard( m_isRunning );

    Log::debug( "Started writing temporary data to permanent db" );

    updateItems();
    updateDevices();
    updateCommandTimestamps();

    Log::debug( "Done writing temporary data to permanent db" );
}

void PermanentDBWriteScheduler::updateItems() {
    if( !m_itemDbModifier.hasChanges() )
        return;

    // Get all items' id.
    auto& dbClient = m_itemDbModifier.getDBClient();
    using namespace Model::Resources;
    const JsonObject fieldsToReturn = JSON( ItemFields::IdFieldName << 1 );
    const auto allItemsId = dbClient.findAll( DBNames::ItemsDB, JSON_EMPTY_OBJ, &fieldsToReturn );

    // Iterate all items in temp DB.
    for( const auto& itemIdObj : allItemsId ) {
        const JsonObject fieldsToReturn = JSON( ItemFields::ValueFieldName << 1 );  // dynamic fields
        const auto itemData = dbClient.findOne( DBNames::TempItemsDB, itemIdObj, &fieldsToReturn );
        if( itemData.empty() )
            continue;

        // Update dynamic fields in permanent DB if changed.
        const JsonObject& newValue = itemData[ ItemFields::ValueFieldName ];
        dbClient.update( DBNames::ItemsDB,
                         JSON( ItemFields::ValueFieldName << JSON( "$ne" << newValue ) << itemIdObj ),
                         JSON( "$set" << JSON( ItemFields::ValueFieldName << newValue ) ) );
        Log::trace( "Processed value of item " + itemIdObj[ ItemFields::IdFieldName ].get< std::string >() );
    }

    m_itemDbModifier.setChangedStatus( false );
}

void PermanentDBWriteScheduler::updateDevices() {
    if( !m_deviceDbModifier.hasChanges() )
        return;

    auto& dbClient = m_deviceDbModifier.getDBClient();
    using namespace Model::Resources;

    // Get all devices' id.
    const JsonObject fieldsToReturn = JSON( DeviceFields::IdFieldName << 1 );
    const auto allDevicesId = dbClient.findAll( DBNames::DevicesDB, JSON_EMPTY_OBJ, &fieldsToReturn );

    // Iterate all devices in temp DB.
    for( const auto& deviceIdObj : allDevicesId ) {
        const JsonObject fieldsToReturn = JSON( DeviceFields::ReachableFieldName << 1 );  // dynamic fields
        const auto deviceData = dbClient.findOne( DBNames::TempDevicesDB, deviceIdObj, &fieldsToReturn );
        if( deviceData.empty() )
            continue;

        // Update dynamic fields in permanent DB.
        const JsonObject& newValue = deviceData[ DeviceFields::ReachableFieldName ];
        dbClient.update( DBNames::DevicesDB,
                         JSON( DeviceFields::ReachableFieldName << JSON( "$ne" << newValue ) << deviceIdObj ),
                         JSON( "$set" << JSON( DeviceFields::ReachableFieldName << newValue ) ) );
        Log::trace( "Processed reachable state of device " +
                    deviceIdObj[ DeviceFields::IdFieldName ].get< std::string >() );
    }

    m_deviceDbModifier.setChangedStatus( false );
}

void PermanentDBWriteScheduler::updateCommandTimestamps() {
    auto& dbClient = m_itemDbModifier.getDBClient();
    using namespace CoreDHouseModes::Resources;

    const JsonObject fieldsToReturn =
        JSON( HouseModeCommandFields::CommandFieldName << 1 << HouseModeCommandFields::ModeIdFieldName << 1 );
    const auto allCommandTimestamps =
        dbClient.findAll( DBNames::HouseModesCommandTimestampsDBName, JSON_EMPTY_OBJ, &fieldsToReturn );

    // Iterate all timestamps in temp DB.
    for( const auto& singleTimestamp : allCommandTimestamps ) {
        const JsonObject fieldsToReturn = JSON( HouseModeCommandFields::TimestampFieldName << 1 );
        const auto timestampData =
            dbClient.findOne( HaTempHouseModesCommandTimestampsDBName, singleTimestamp, &fieldsToReturn );

        if( timestampData.empty() )
            continue;

        const JsonObject& newValue = timestampData[ HouseModeCommandFields::TimestampFieldName ];

        dbClient.update(
            DBNames::HouseModesCommandTimestampsDBName,
            JSON( HouseModeCommandFields::TimestampFieldName << JSON( "$ne" << newValue ) << singleTimestamp ),
            JSON( "$set" << JSON( HouseModeCommandFields::TimestampFieldName << newValue ) ) );

        LOGGER_TRACE_RAW( cc::trace(
            "Processed value of command " +
            singleTimestamp[ HouseModeCommandFields::CommandFieldName ].get< std::string >() + " of HoseMode id" +
            singleTimestamp[ HouseModeCommandFields::ModeIdFieldName ].get< std::string >() ) );
    }
}

}  // namespace CoreD
}  // namespace HubPlatform
