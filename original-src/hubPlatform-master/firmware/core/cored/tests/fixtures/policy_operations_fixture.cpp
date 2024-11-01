#include "policy_operations_fixture.h"
#include "cored/model/resources/cored_resources.h"

namespace HubPlatform {
namespace UT {

using KeyStringList = std::vector< std::pair< std::string, std::string > >;

const std::string IdField = "_id";
const std::string NameField = "name";

void PolicyOperationsFixture::SetUp() {
    m_service = std::make_unique< CoreD::Tests::TestCoreService >();
    m_service->setUp();
    HubPlatform::CoreD::PolicyOperationsHandler::BroadcastCallback sendBroadcast =
        [this]( MessageSubClasses messageSubClass, const JsonObject& answer ) {};
    m_policyOperationsHandler = std::make_unique< HubPlatform::CoreD::PolicyOperationsHandler >(
        getService().getDBClient(), sendBroadcast, *m_service->m_core );
    using namespace CoreD::Model::Resources;
    m_service->getDBClient().clear( DBNames::PoliciesDB );
}

void PolicyOperationsFixture::TearDown() {
    if( m_service )
        m_service->tearDown();
}

Services::Service& PolicyOperationsFixture::getService() {
    return *m_service;
}

void PolicyOperationsFixture::addPolicy( const std::string& policyId, const std::string& name ) {
    using namespace CoreD::Model::Resources;
    getService().getDBClient().insert( DBNames::PoliciesDB, JSON( IdField << policyId << NameField << name ) );
}

void PolicyOperationsFixture::addPolicyJSON( const Json& obj ) {
    using namespace CoreD::Model::Resources;
    getService().getDBClient().insert( DBNames::PoliciesDB, obj );
}

void PolicyOperationsFixture::deletePolicy( const std::string& policyId ) {
    using namespace CoreD::Model::Resources;
    getService().getDBClient().remove( DBNames::PoliciesDB, JSON( IdField << policyId ) );
}

JsonObject PolicyOperationsFixture::validatePolicyCreate( JsonObject& message ) {
    return m_policyOperationsHandler->validatePolicyRequest( message, true );
}

JsonObject PolicyOperationsFixture::getPolicyUpdateBroadcastData( const JsonObject& updated, const JsonObject& saved ) {
    return m_policyOperationsHandler->processPolicyUpdateBroadcastData( updated, saved );
}

JsonObject PolicyOperationsFixture::getPolicyList( const JsonObject& params ) {
    return m_policyOperationsHandler->processPolicyListRequest( params );
}

JsonObject PolicyOperationsFixture::getPolicyGet( const JsonObject& params ) {
    return m_policyOperationsHandler->processPolicyGetRequest( params );
}

}  // namespace UT
}  // namespace HubPlatform
