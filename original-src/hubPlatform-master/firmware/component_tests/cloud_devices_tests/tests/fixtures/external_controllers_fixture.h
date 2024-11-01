#include "cloud_devices_tests/tests/fixtures/common_cloud_devices_fixture.h"

#include "external_controllers/remote_interface/ec_remote_interface.h"

#include "json_utils/json_wrapper.h"

namespace HubPlatform {
namespace Tests {

class ExternalControllersTest : public CommonCloudDevicesTests {
public:
    enum class FwCertsType { Correct, Broken, ExpiredRoot };

public:
    ExternalControllersTest();

    virtual ExternalControllers::RemoteInterface& externalControllers();

    static std::vector< std::string > dump( const std::vector< JsonObject >& jsons );

    static std::string generateItem( const std::string& itemId );
    static std::string generateItem( const std::string& controllerId, const std::string& itemId );
    static JsonArray generateItems( const std::string& controllerId, const std::vector< std::string >& itemIds );

    static std::string generateDevice( const std::string& deviceId );
    static std::string generateDevice( const std::string& controllerId, const std::string& deviceId );
    static JsonArray generateDevices( const std::string& controllerId, const std::vector< std::string >& deviceIds );

    static Json makeRawCommandResult( const std::string& id, const std::string& method, Json innerCommandResult );

    void subscribeOnItemChanges( const std::string& controllerId, const std::vector< std::string >& itemsIds );
    void subscribeOnDeviceChanges( const std::string& controllerId, const std::vector< std::string >& devicesIds );
    void setCredentials( int version,
                         const std::string& controllerId,
                         const std::string& controllerUuid,
                         const std::vector< std::string >& keys,
                         FwCertsType certsType = FwCertsType::Correct );

    JsonObject setBridgeControllerConnection( const std::string& controllerUuid,
                                              const std::vector< std::string >& ips );
    JsonObject getBridgeConnectionsList();

private:
    static std::string ms_firmwareCertificate;
    static std::string ms_firmwareKey;
    static std::string ms_firmwareParent;
    static std::string ms_firmwareExpiredParent;

    ExternalControllers::RemoteInterface m_externalControllers;
};

}  // namespace Tests
}  // namespace HubPlatform
