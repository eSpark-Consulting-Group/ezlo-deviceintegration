#include "alert_notification_cotegory_source.h"
#include "zigbee/resources/zigbee_resources.h"

namespace HubPlatform {
namespace Zigbee {
namespace AlertNotificationCategory {

struct AlertNotificationCategorySourceData {
    Category id;
    const char* name;
};

static const struct AlertNotificationCategorySourceData category[] = {
    { Category::Warning, Resources::Fields::Command::Formats::AlertStructure::Category::Warning },
    { Category::Danger, Resources::Fields::Command::Formats::AlertStructure::Category::Danger },
    { Category::Failure, Resources::Fields::Command::Formats::AlertStructure::Category::Failure },
    { Category::Reserved, Resources::Fields::Command::Formats::AlertStructure::Reserved }
};

const char* toString( Category id ) {
    for( const auto& data : category ) {
        if( data.id == id ) {
            return data.name;
        }
    }

    return Resources::Fields::Command::Formats::AlertStructure::Reserved;
}

Category AlertNotificationCategorySourceFromString( const std::string& name ) {
    for( const auto& data : category ) {
        if( name == data.name ) {
            return data.id;
        }
    }

    return HubPlatform::Zigbee::AlertNotificationCategory::Category::Reserved;
}

}  // namespace AlertNotificationCategory
}  // namespace Zigbee
}  // namespace HubPlatform
