#include "group_operations_fixture.h"
#include "cored/model/resources/cored_resources.h"

namespace HubPlatform {
namespace UT {

using KeyStringList = std::vector< std::pair< std::string, std::string > >;

const std::string IdField = "_id";
const std::string NameField = "name";
const std::string CategoriesField = "categories";
const std::string DevicesField = "devices";
const std::string ExceptionsField = "exceptions";

void GroupOperationsFixture::SetUp() {
    m_service = std::make_unique< CoreD::Tests::TestCoreService >();
    m_service->setUp();
    HubPlatform::CoreD::GroupOperationsHandler::BroadcastCallback sendBroadcast =
        [this]( MessageSubClasses messageSubClass, const JsonObject& answer ) {};
    m_groupOperationsHandler = std::make_unique< HubPlatform::CoreD::GroupOperationsHandler >(
        getService().getDBClient(), sendBroadcast, *m_service->m_core );
    using namespace CoreD::Model::Resources;
    m_service->getDBClient().clear( DBNames::DeviceGroupsDB );
}

void GroupOperationsFixture::TearDown() {
    if( m_service )
        m_service->tearDown();
}

Services::Service& GroupOperationsFixture::getService() {
    return *m_service;
}

JsonObject GroupOperationsFixture::validateDeviceGroupCreate( JsonObject& message ) {
    return m_groupOperationsHandler->validateCreateGroupRequest( message, true );
}

JsonObject GroupOperationsFixture::validateItemGroupCreate( JsonObject& message ) {
    return m_groupOperationsHandler->validateItemGroupRequest( message, true );
}

JsonObject GroupOperationsFixture::getDeviceGroupUpdateBroadcastData( const JsonObject& updated,
                                                                      const JsonObject& saved ) {
    return m_groupOperationsHandler->getDeviceGroupUpdateBroadcastData( updated, saved );
}

void GroupOperationsFixture::addDevice( const std::string& deviceId ) {
    getService().getDBClient().insert( HaDevicesDBName, JSON( IdField << deviceId ) );
}

void GroupOperationsFixture::addDeviceGroup( const std::string& groupId, const std::string& name ) {
    using namespace CoreD::Model::Resources;
    getService().getDBClient().insert( DBNames::DeviceGroupsDB, JSON( IdField << groupId << NameField << name ) );
}

void GroupOperationsFixture::addDeviceGroupWithCategoriesAndDevices( const std::string& groupId,
                                                                     const std::string& name,
                                                                     const JsonArray& categories,
                                                                     const JsonArray& devices,
                                                                     const JsonArray& exceptions ) {
    using namespace CoreD::Model::Resources;
    getService().getDBClient().insert( DBNames::DeviceGroupsDB,
                                       JSON( IdField << groupId << NameField << name << CategoriesField << categories
                                                     << DevicesField << devices << ExceptionsField << exceptions ) );
}

void GroupOperationsFixture::addItemGroup( const std::string& groupId, const std::string& name ) {
    using namespace CoreD::Model::Resources;
    getService().getDBClient().insert( DBNames::ItemGroupsDB, JSON( IdField << groupId << NameField << name ) );
}

JsonObject GroupOperationsFixture::getDeviceGroupsList( const JsonObject& params ) {
    return m_groupOperationsHandler->processDeviceGroupsListRequest( params );
}

JsonObject GroupOperationsFixture::getDeviceGroupGet( const JsonObject& params ) {
    return m_groupOperationsHandler->processDeviceGroupGetRequest( params );
}

JsonObject GroupOperationsFixture::getItemGroupsList( const JsonObject& params ) {
    return m_groupOperationsHandler->processItemGroupsListRequest( params );
}

JsonObject GroupOperationsFixture::getItemGroupGet( const JsonObject& params ) {
    return m_groupOperationsHandler->processItemGroupGetRequest( params );
}

JsonObject GroupOperationsFixture::getDeviceGroupFind( const JsonObject& params ) {
    return m_groupOperationsHandler->processDeviceGroupFindRequest( params );
}

JsonObject GroupOperationsFixture::deviceItemGroupExpand( const JsonObject& params ) {
    return m_groupOperationsHandler->processDeviceItemGroupExpandRequest( params );
}
JsonObject GroupOperationsFixture::getItemGroupUpdateBroadcastData( const JsonObject& updated,
                                                                    const JsonObject& saved ) {
    return m_groupOperationsHandler->getItemGroupUpdateBroadcastData( updated, saved );
}

JsonObject GroupOperationsFixture::createDeviceGroupRequest( const std::string& name,
                                                             const std::string& category,
                                                             const std::string& devices,
                                                             const std::string& exceptions ) {
    const auto request = R"(
        {
            "name": "__NAME__",
            "categories": [__CATEGORIES_LIST__],
            "devices": [__DEVICES__],
            "exceptions": [__EXCEPTIONS__]
        }
        )";

    KeyStringList keys = { { "__NAME__", name },
                           { "__CATEGORIES_LIST__", category },
                           { "__DEVICES__", devices },
                           { "__EXCEPTIONS__", exceptions } };
    auto result = Json::parse( replaceTokensInString( request, keys ) );
    result[ "entryDelay" ] = "none";
    result[ "role" ] = "user";
    result[ "packageId" ] = "";

    return result;
}

}  // namespace UT
}  // namespace HubPlatform
