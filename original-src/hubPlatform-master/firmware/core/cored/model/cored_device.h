#pragma once

#include "device_model/h/device_additional_info.h"
#include "device_model/h/firmware_info.h"
#include "json_utils/json_fwd.h"

#include <string>
#include <vector>
#include <set>

namespace HubPlatform {

namespace UT {
class HouseModesProcessorFixture;
}

namespace CoreDRemoteInterface {
class CoreDRemoteInterface;
}

namespace Tests {
class ParentRoomTest;
}

namespace CoreD {
namespace Model {

namespace Security {

static const char* No = "no";
static const char* Low = "low";
static const char* Middle = "middle";
static const char* High = "high";

static const std::set< std::string > PossibleValues = { No, Low, Middle, High };

}  // namespace Security

/* @brief cored's representation of a device */
class Device final {
    friend CoreDRemoteInterface::CoreDRemoteInterface;
    friend HubPlatform::UT::HouseModesProcessorFixture;
    friend HubPlatform::Tests::ParentRoomTest;

public:
    using AdditionalInfo = DeviceAdditionalInfo;

    Device( const std::string& gatewayId,
            const std::string& name,
            const std::string& type,
            const std::string& deviceTypeId,
            const std::string& category,
            bool batteryPowered,
            const std::string& protectConfig = "default" );

    const std::string& getGatewayId() const;

    Device& setAdditionalInfo( std::vector< AdditionalInfo >&& additionalInfo );
    Device& setFirmware( std::vector< FirmwareInfo >&& firmware );
    Device& setReachable( bool reachable );
    Device& setParentRoom( bool parentRoom );
    Device& setParentDeviceId( const std::string& parentDeviceId );
    Device& setSubcategory( const std::string& subcategory );
    Device& setRoomId( const std::string& roomId );
    bool setSecurity( const std::string& security );
    bool setStatus( const std::string& status );
    Device& setReady( bool enableReady );
    Device& setServiceNotification( bool enable );
    Device& setPersistent( bool value );
    Device& setHouseModesOptions( Json options );

private:
    Json toJson() const;
    static void checkDeviceAsJson( const Json& device, bool isGetDeviceListCall = false );

private:
    const std::string m_name;          // required
    const std::string m_type;          // required
    const std::string m_deviceTypeId;  // required (identify a specific device)
    const std::string m_category;      // required
    const std::string m_gatewayId;     // required TODO: hidden, get it from a gateway this device belongs to

    bool m_batteryPowered;
    bool m_reachable;                                // optional, by default true
    std::string m_roomId;                            // optional, by default empty
    std::string m_parentDeviceId;                    // optional, by default empty
    std::string m_subcategory;                       // optional, by default empty
    std::vector< AdditionalInfo > m_additionalInfo;  // optional
    std::vector< FirmwareInfo > m_firmware;          // optional
    std::string m_security;                          // optional, by default "no"
    std::string m_status;                            // optional, by default "idle"
    bool m_ready;                                    // optional, by default true
    bool m_serviceNotification;                      // optional, by default false
    bool m_persistent;                               // optional, by default false
    Json m_houseModesOptions;                        // optional
    bool m_parentRoom;                               // optional
    std::string m_protectConfig;                     // optional, by default "default"

    static const int m_maxAdditionalInfoSize = 20;
    static const int m_minFieldsAmount = 17;
};

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
