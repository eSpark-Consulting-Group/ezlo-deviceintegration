#include "common_test_resources/fixtures/core_test_fixture.h"
#include "cored/model/resources/cored_resources.h"
#include "cored/house_modes/resources/house_mode_resources.h"
#include "common_test_resources/fixtures/device_item_groups_fixture.h"

namespace HubPlatform {
namespace Tests {

class TestsHouseModeFixture : public DeviceItemGroupsFixture {
public:
    void SetUp() override;
    void TearDown() override;

    using HubModesGetPred = std::function< void( const Json& houseMode, std::string errorMessage ) >;
    static JsonObject callHouseModesGet( const std::string& errorMessage );
    static void checkHouseModes( const Json& commandResponse,
                                 const HubModesGetPred& modesPred,
                                 const std::string& errorMessage );

    bool findInCollection( const Json& answer, const std::string& collectionName, const std::string& deviceId );
    bool searchInArray( const JsonArray& collection, const Json& value );

    static void checkHouseModesGet( const HubModesGetPred& modesPred,
                                    const std::string& errorMessage,
                                    Json* answerReturned = nullptr );
    void checkModesChangedDisarmedDefault( bool value, const std::string& message );
    void checkHouseModeSwitched( const std::string& message );
    void resetDisarmedDefaultFlag();
    void setDisarmedDefault( const std::string& modeId, bool disarmedDefault, const std::string& message );
    JsonArray fetchCollection( const Json& answer, const std::string& name );
};

}  // namespace Tests
}  // namespace HubPlatform