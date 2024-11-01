#include "cored/tests/fixtures/house_modes_processor_fixture.h"

#include "cored/model/values/value_factory.h"
#include "cored/house_mode_proc.h"
#include "cored/house_modes/resources/house_mode_resources.h"

#include "common_utils/cpp14.h"

namespace HubPlatform {
namespace UT {

void HouseModesProcessorFixture::TearDown() {
    if( m_service )
        m_service->tearDown();
}

HouseModesProcessorFixture::Service& HouseModesProcessorFixture::getService() {
    return *m_service;
}

HouseModesProcessorFixture::Service& HouseModesProcessorFixture::getBroadcaster() {
    return getService();
}

IDbClient& HouseModesProcessorFixture::getDBClient() {
    return getService().getDBClient();
}

const Services::UT::FakeResponseBus& HouseModesProcessorFixture::getTestBus() {
    return *dynamic_cast< Services::UT::FakeResponseBus* >( m_service->getBus() );
}

HouseModesProcessorFixture::DeviceDbModifier& HouseModesProcessorFixture::getDevicesDbModifier() {
    return getService().getDevicesDbModifier();
}

void HouseModesProcessorFixture::initService( const std::string& data ) {
    auto configReader = ConfigReaderFactory::createConfigReader( [data]( const std::string& ) { return data; } );
    m_service = std::make_unique< CoreD::Tests::TestCoreService >(
        std::make_unique< Services::BusWrapper >( Services::BusWrapper::BusTypeTag< Services::UT::FakeResponseBus >{} ),
        std::make_unique< HubPlatform::UT::FakeDbClient >(),
        std::move( configReader ) );
    m_service->setUp();
}

HouseModesProcessorFixture::DeviceIDs HouseModesProcessorFixture::initDevices( const std::vector< Device >& devices ) {
    std::vector< std::string > deviceIds;
    std::for_each( devices.begin(), devices.end(), [this, &deviceIds]( const Device& device ) {
        Json response = m_service->handleAddDevice( device.toJson() );
        deviceIds.push_back( json_get_str( response, "id" ) );
    } );
    return deviceIds;
}

void HouseModesProcessorFixture::initItems( const std::vector< Json >& items ) {
    for( const auto& item : items ) {
        m_service->handleAddItem( item );
    }
}

void HouseModesProcessorFixture::initHouseModeProcessor( const std::string& data ) {
    auto configReader = ConfigReaderFactory::createConfigReader( [data]( const std::string& ) { return data; } );
    m_houseModeProcessor.reset( new HouseModeProcessor( getService(), getDBClient(), std::move( configReader ) ) );
}

void HouseModesProcessorFixture::initRemoteInterface() {
    m_remoteInterface.reset( new RemoteInterface( getService() ) );
}

void HouseModesProcessorFixture::waitSwitchingHouseMode() {
    m_houseModeProcessor->wait();
}

JsonObject HouseModesProcessorFixture::generateHouseModes() {
    const JsonObject houseMode1{ { "name", "Name 1" },
                                 { "description", "Description 1" },
                                 { "id", 1 },
                                 { "disarmedDefault", false },
                                 { "protect", true },
                                 { "armed", false },
                                 { "ignoreDelay", true },
                                 { "armedCategories",
                                   JSON_ARRAY( JSON( "category"
                                                     << "category1"
                                                     << "subcategories"
                                                     << JSON_ARRAY( "subcategory1"
                                                                    << "subcategory2"
                                                                    << "subcategory3" ) ) ) },
                                 { "alarmCategories",
                                   JSON_ARRAY( JSON( "category"
                                                     << "category2"
                                                     << "subcategories"
                                                     << JSON_ARRAY( "subcategory1"
                                                                    << "subcategory2"
                                                                    << "subcategory3" ) ) ) },
                                 { "notificationsDefault", nullptr } };
    const JsonObject houseMode2{ { "name", "Name 2" },
                                 { "description", "Description 2" },
                                 { "id", 2 },
                                 { "disarmedDefault", true },
                                 { "protect", false },
                                 { "armed", true },
                                 { "ignoreDelay", false },
                                 { "armedCategories",
                                   JSON_ARRAY( JSON( "category"
                                                     << "category2"
                                                     << "subcategories"
                                                     << JSON_ARRAY( "subcategory1"
                                                                    << "subcategory2"
                                                                    << "subcategory3" ) ) ) },
                                 { "alarmCategories",
                                   JSON_ARRAY( JSON( "category"
                                                     << "category2"
                                                     << "subcategories"
                                                     << JSON_ARRAY( "subcategory1"
                                                                    << "subcategory2"
                                                                    << "subcategory3" ) ) ) },
                                 { "notificationsDefault",
                                   JSON_ARRAY( "user1"
                                               << "user2" ) } };
    JsonObject allData{
        { "house_modes", JSON_ARRAY( houseMode1 << houseMode2 ) },
        { "settings",
          { { "default", 1 },
            { "switchToDelay", 0 },
            { "alarmDelay", 0 },
            { "entryDelay", JSON( "normal" << 30 << "longExtended" << 120 << "extended" << 60 << "instant" << 0 ) },
            { "abortWindow", JSON( "default" << 30 << "minimum" << 0 << "maximum" << 45 ) } } }
    };
    return allData;
}

std::vector< HouseModesProcessorFixture::Device > HouseModesProcessorFixture::generateDevices( std::string gatewayId ) {
    if( gatewayId.empty() )
        gatewayId = "gatewayId";
    constexpr bool batteryPowered{ false };
    Device device1( gatewayId, "Device 1", "Type", "DeviceTypeId", "category1", batteryPowered );
    device1.setSubcategory( "subcategory2" );
    Device device2( gatewayId, "Device 2", "Type", "DeviceTypeId", "category00", batteryPowered );
    device2.setSubcategory( "subcategory00" );
    Device alarmDevice( gatewayId, "Device 3", "Type", "DeviceTypeId", "category2", batteryPowered );
    alarmDevice.setSubcategory( "subcategory3" );

    std::vector< Device > data{ device1, device2, alarmDevice };
    return data;
}

std::vector< Json > HouseModesProcessorFixture::generateSecurityItem( const std::string& deviceId ) {
    using namespace CoreDHouseModes::Resources;
    const CoreD::Model::ValueType& valueType = CoreD::Model::ValueType::Enum::Bool;
    CoreD::Model::Item item0( deviceId,
                              Items::SecurityThreat,
                              valueType,
                              true,
                              true,
                              CoreD::Model::ValueFactory::createDefaultItemValue( valueType ),
                              "" );

    std::vector< Json > results;
    Json result = m_service->handleAddItem( item0.toJson() );
    LOGGER_DEBUG( "Item creation result: " + cc::j( result ) );
    results.push_back( result );
    return results;
}

Json HouseModesProcessorFixture::generateSwitchItem( const std::string& deviceId ) {
    using namespace CoreDHouseModes::Resources;
    const CoreD::Model::ValueType& valueType = CoreD::Model::ValueType::Enum::Bool;
    CoreD::Model::Item item0( deviceId,
                              "switch",
                              valueType,
                              true,
                              true,
                              CoreD::Model::ValueFactory::createDefaultItemValue( valueType ),
                              "" );

    Json result = m_service->handleAddItem( item0.toJson() );
    LOGGER_DEBUG( "Item creation result: " + cc::j( result ) );
    return result;
}

CoreD::CoreService& HouseModesProcessorFixture::getCoreDService() {
    if( !m_service )
        throw std::runtime_error( "HouseModesProcessorFixture::m_service is a nullptr" );
    return *m_service;
}

bool HouseModesProcessorFixture::setItemValue( const JsonObject& idValue, const std::string& operationId ) {
    return getCoreDService().handleSetItemValue( idValue, operationId );
}

CoreD::CoreService::GatewayID HouseModesProcessorFixture::ensureGateway( const Json& gate ) {
    auto gatewayId = getCoreDService().m_deviceManager->ensureGateway( gate );
    return gatewayId[ "id" ];
}

}  // namespace UT
}  // namespace HubPlatform
