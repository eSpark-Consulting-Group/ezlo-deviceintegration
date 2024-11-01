#include "local_notifiction_data_record.h"

#include "json_utils/json_utils.h"

namespace HubPlatform {
namespace CloudDevices {

Json LocalNotificationDataRecord::toJson() const {
    return JSON( "status" << status << "current_version" << currentVersion << "url" << url << "s3_id" << s3Id
                          << "modified" << modified );
}

LocalNotificationDataRecord LocalNotificationDataRecord::fromJson( const Json& j ) {
    LocalNotificationDataRecord record;
    record.status = json_get_str( j, "status" );
    record.currentVersion = json_get_int( j, "current_version" );
    record.url = json_get_str( j, "url" );
    record.s3Id = json_get_str( j, "s3_id" );
    record.modified = json_get_int( j, "modified" );
    return record;
}

}  // namespace CloudDevices
}  // namespace HubPlatform
