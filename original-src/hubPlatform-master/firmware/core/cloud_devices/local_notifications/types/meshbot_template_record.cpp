#include "meshbot_template_record.h"

#include "json_utils/json_utils.h"

namespace HubPlatform {
namespace CloudDevices {

Json MeshbotTemplateRecord::toJson() const {
    return JSON( "uuid" << uuid << "version" << version << "title" << title << "description" << description << "id"
                        << id << "oem_id" << oemId << "partner_uuid" << partnerUUID << "local_meshbot" << localMeshbot
                        << "rules" << rules );
}

MeshbotTemplateRecord MeshbotTemplateRecord::fromJson( const Json& j ) {
    MeshbotTemplateRecord record;
    record.uuid = json_get_str( j, "uuid" );
    record.version = json_get_str( j, "version" );
    record.title = json_get_str( j, "title" );
    record.description = json_get_str( j, "description" );
    record.id = json_get_str( j, "id" );
    record.oemId = json_get_str( j, "oem_id" );
    record.partnerUUID = json_get_str( j, "partner_uuid" );
    record.localMeshbot = json_get_str( j, "local_meshbot" );
    json_get_arr( j, "rules", record.rules );
    return record;
}

}  // namespace CloudDevices
}  // namespace HubPlatform
