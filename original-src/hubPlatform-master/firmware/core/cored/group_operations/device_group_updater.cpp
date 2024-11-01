#include "device_group_updater.h"
#include "group_operations_utils.h"
#include "scenes_manager/scenes_engine/helpers/json_helper.h"
#include "device_group.h"
#include "json_utils/json_path.h"
#include "jsonrpc/jsonrpc.h"
#include <algorithm>
#include "resources.h"
#include "cored/model/resources/cored_resources.h"

namespace HubPlatform {
namespace CoreD {

JsonObject DeviceGroupUpdater::getDeviceGroupUpdateData( const JsonObject& dbItem, const Json& params ) {
    using namespace Resources;
    auto savedName = json_get_str( dbItem, DeviceGroup::Name );
    auto updateName = json_get_str( params, DeviceGroup::Name );
    auto savedPersistent = json_get_bool( dbItem, DeviceGroup::Persistent );
    auto updatePersistent = json_get_bool( params, DeviceGroup::Persistent );
    bool hasUpdatedPersistent = json_get( params, DeviceGroup::Persistent, updatePersistent );
    Json updatedJson = params;

    if( !hasUpdatedPersistent ) {
        updatedJson[ DeviceGroup::Persistent ] = savedPersistent;
    }

    return updatedJson;
}

JsonObject DeviceGroupUpdater::getDeviceGroupUpdateBroadcastData( const JsonObject& updatedData,
                                                                  const JsonObject& savedData ) {
    using namespace Resources;

    JsonArray categoriesToUpdate;
    JsonArray savedCategories;
    json_get_arr( updatedData, DeviceGroup::Categories, categoriesToUpdate );
    json_get_arr( savedData, DeviceGroup::Categories, savedCategories );
    JsonArray updatedCategories;
    bool isUpdateCategories = !GroupOperationsUtils::areObjectArraysEqual( categoriesToUpdate, savedCategories );
    JsonArray devicesToUpdate;
    JsonArray savedDevices;
    json_get_arr( updatedData, DeviceGroup::Devices, devicesToUpdate );
    json_get_arr( savedData, DeviceGroup::Devices, savedDevices );
    JsonArray updatedDevices;
    bool areDevicesUpdated = !GroupOperationsUtils::areStringArraysEqual( devicesToUpdate, savedDevices );
    JsonArray exceptionsToUpdate;
    JsonArray savedExceptions;
    JsonArray devicesAdded;
    JsonArray devicesRemoved;
    JsonArray exceptionsAdded;
    JsonArray exceptionsRemoved;

    json_get_arr( updatedData, DeviceGroup::Exceptions, exceptionsToUpdate );
    json_get_arr( savedData, DeviceGroup::Exceptions, savedExceptions );
    JsonArray updatedExceptions;
    bool areExceptionsUpdated = !GroupOperationsUtils::areStringArraysEqual( exceptionsToUpdate, savedExceptions );

    auto savedName = json_get_str( savedData, DeviceGroup::Name );
    auto updateName = json_get_str( updatedData, DeviceGroup::Name );
    auto savedPersistent = json_get_bool( savedData, DeviceGroup::Persistent );
    auto updatePersistent = json_get_bool( updatedData, DeviceGroup::Persistent );
    bool hasSavedPersistent = json_get( savedData, DeviceGroup::Persistent, updatePersistent );
    bool hasUpdatedPersistent = json_get( updatedData, DeviceGroup::Persistent, updatePersistent );
    auto savedEntryDelay = json_get_str( savedData, DeviceGroup::EntryDelay );
    auto updateEntryDelay = json_get_str( updatedData, DeviceGroup::EntryDelay );

    /* find new and removed devices */
    for( const auto& device : devicesToUpdate ) {
        if( !json_array_is_string_present( savedDevices, device ) ) {
            devicesAdded += device;
        }
    }
    for( const auto& device : savedDevices ) {
        if( !json_array_is_string_present( devicesToUpdate, device ) ) {
            devicesRemoved += device;
        }
    }

    /* find new and removed exceptions */
    for( const auto& exception : exceptionsToUpdate ) {
        if( !json_array_is_string_present( savedExceptions, exception ) ) {
            exceptionsAdded += exception;
        }
    }
    for( const auto& exceptions : savedExceptions ) {
        if( !json_array_is_string_present( exceptionsToUpdate, exceptions ) ) {
            exceptionsRemoved += exceptions;
        }
    }

    Json updatedJson = JSON_EMPTY_OBJ;
    if( isUpdateCategories ) {
        updatedJson[ DeviceGroup::Categories ] = categoriesToUpdate;
    }

    if( areDevicesUpdated ) {
        updatedJson[ DeviceGroup::Devices ] = devicesToUpdate;
    }

    if( areExceptionsUpdated ) {
        updatedJson[ DeviceGroup::Exceptions ] = exceptionsToUpdate;
    }

    if( !hasUpdatedPersistent && hasSavedPersistent ) {
        updatedJson[ DeviceGroup::Persistent ] = savedPersistent;
    }

    if( savedName != updateName ) {
        updatedJson[ DeviceGroup::Name ] = updateName;
        updatedJson[ DeviceGroup::Sync ] = true;
    }
    else {
        if( !updatedJson.empty() ) {
            updatedJson[ DeviceGroup::Sync ] = false;
        }
    }
    if( savedEntryDelay != updateEntryDelay ) {
        updatedJson[ DeviceGroup::EntryDelay ] = updateEntryDelay;
    }

    if( !devicesAdded.empty() ) {
        updatedJson[ DeviceGroup::AddedDevices ] = devicesAdded;
    }
    if( !devicesRemoved.empty() ) {
        updatedJson[ DeviceGroup::RemovedDevices ] = devicesRemoved;
    }
    if( !exceptionsAdded.empty() ) {
        updatedJson[ DeviceGroup::AddedExceptions ] = exceptionsAdded;
    }
    if( !exceptionsRemoved.empty() ) {
        updatedJson[ DeviceGroup::RemovedExceptions ] = exceptionsRemoved;
    }

    if( !updatedJson.empty() ) {
        updatedJson[ DeviceGroup::Role ] = json_get_str( savedData, DeviceGroup::Role );
    }

    return updatedJson;
}

bool fillDeviceArray( const std::string& removedId, const JsonArray& groupDevices, JsonArrayBuilder& changedDevices ) {
    bool changed = false;
    for( const auto& device : groupDevices ) {
        auto deviceId = device.get< std::string >();
        if( deviceId == removedId ) {
            changed = true;
        }
        else {
            changedDevices << deviceId;
        }
    }

    return changed;
}

void DeviceGroupUpdater::removeDeviceFromGroupOperations( const std::string& deviceId,
                                                          IDbClient& dbClient,
                                                          const BroadcastCallback& sendBroadcast ) {
    using namespace Resources;
    using namespace Model::Resources;

    auto groups = dbClient.findAll( DBNames::DeviceGroupsDB, JSON_EMPTY_OBJ, nullptr );
    for( auto& groupOne : groups ) {
        CoreD::DeviceGroupInfo info;
        info.fromJson( groupOne );
        auto id = info.getId();
        auto groupDevices = info.getDevices();
        auto groupExceptions = info.getExceptions();
        JsonArrayBuilder changedDevices;
        JsonArrayBuilder changedExceptions;
        bool isDevicesChanged = fillDeviceArray( deviceId, groupDevices, changedDevices );
        bool isExceptionsChanged = fillDeviceArray( deviceId, groupExceptions, changedExceptions );

        dbClient.update( DBNames::DeviceGroupsDB,
                         JSON( "_id" << id ),
                         JSON( "$set" << JSON( DeviceGroup::Devices << changedDevices << DeviceGroup::Exceptions
                                                                    << changedExceptions ) ),
                         false,
                         true );

        JsonObject group = JSON_EMPTY_OBJ;
        if( sendBroadcast && ( isDevicesChanged || isExceptionsChanged ) ) {
            group[ DeviceGroup::Id ] = id;
            if( isDevicesChanged ) {
                group[ DeviceGroup::Devices ] = changedDevices;
            }
            if( isExceptionsChanged ) {
                group[ DeviceGroup::Exceptions ] = changedExceptions;
            }

            sendBroadcast( DEVICE_GROUP_UPDATED, group );
        }
    }
}

}  // namespace CoreD
}  // namespace HubPlatform
