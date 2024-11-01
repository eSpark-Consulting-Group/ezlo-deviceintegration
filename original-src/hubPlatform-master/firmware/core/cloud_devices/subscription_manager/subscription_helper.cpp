#include "cloud_devices/subscription_manager/subscription_helper.h"

#include "cloud_devices/subscription_manager/subscription_exceptions.h"
#include "cloud_devices/resources/cloud_devices_resources.h"

#include "service_interfaces/service.h"
#include "cored/remote_interface/cored_interface.h"
#include "json_utils/json_utils.h"
#include "jsonrpc/jsonrpc.h"
#include "cored/model/resources/cored_resources.h"

#include "cored/model/cored_item.h"
#include "cored/model/values/value_factory.h"
#include "cored/model/cored_device.h"

#include <chrono>

namespace HubPlatform {
namespace CloudDevices {

namespace {
const std::chrono::seconds subscriptionListTimout{ 30 };
const std::chrono::minutes subscriptionListResendDelay{ 1 };
const std::chrono::minutes subscriptionListResendLongDelay{ 10 };
constexpr size_t retriesBurstLimit = 3;
}  // namespace

SubscriptionHelper::SubscriptionHelper( Services::Service& hostService, CoreDRemoteInterface::CoreDInterface& core )
: m_service( hostService )
, m_core( core ) {}

SubscriptionHelper::~SubscriptionHelper() {
    SubscriptionHelper::tearDown();
}

SubscriptionHelper::Result SubscriptionHelper::addDevice( const CoreD::Model::Device& device ) {
    return m_core.addDevice( device );
}

SubscriptionHelper::Result SubscriptionHelper::addItem( const CoreD::Model::Item& item ) {
    return m_core.addItem( item );
}

void SubscriptionHelper::modifyItem( const std::string& itemId, const CoreD::Model::Item& item ) {
    m_core.modifyItem( itemId, item );
}

void SubscriptionHelper::removeDevice( const Id& deviceId ) {
    m_core.removeDevice( deviceId );
}

void SubscriptionHelper::removeItem( const Id& itemId ) {
    m_core.removeItem( itemId );
}

Json SubscriptionHelper::getItem( const Id& itemId ) {
    return m_core.getItem( itemId );
}

Json SubscriptionHelper::getDevice( const ISubscriptionHelper::Id& deviceId ) {
    return m_core.getSingleDevice( deviceId );
}

std::string SubscriptionHelper::getGatewayId() {
    using namespace Resources;

    if( !m_gatewayId.empty() )
        return m_gatewayId;

    std::vector< std::pair< std::string, std::string > > unreachableReasons;
    std::vector< std::pair< std::string, std::string > > unreachableActions;

    CoreD::Model::Gateway gateway( SubscriptionGateway::GatewayName,
                                   unreachableReasons,
                                   unreachableActions,
                                   SubscriptionGateway::GatewayPluginId,
                                   {},
                                   {},
                                   {},
                                   GatewayBuiltInScripts::SetItemValueScript );

    auto result = m_core.ensureGateway( gateway );

    if( result.second )
        return result.first;

    throw SubscriptionError( "Unable to ensure gateway" );
}

Json SubscriptionHelper::getGatewayByName( const std::string& gatewayName ) {
    return m_core.getGatewayByPluginId( gatewayName );
}

Json SubscriptionHelper::getGatewayById( const std::string& gatewayId ) {
    return m_core.getGatewayById( gatewayId );
}

void SubscriptionHelper::sendSubscriptionList( Json message ) {
    LOGGER_DEBUG_RAW( cc::debug( "Sending subscription list request to cloud" ) );
    std::unique_lock< std::recursive_mutex > lock( m_heapMutex );
    m_messageHeap.emplace_front( std::move( message ) );
    m_retries = 0;
    sendSubscriptionList();
}

void SubscriptionHelper::sendSubscriptionList() {
    std::unique_lock< std::recursive_mutex > lock( m_heapMutex );

    if( m_messageHeap.empty() || m_waitingResponse )
        return;

    auto responseHandler = [this]( const JsonObject& response ) {
        bool responseError = false;

        if( hasError( response ) ) {
            responseError = true;
            LOGGER_ERROR_RAW( cc::error( "Error sending subscription list: " ) + cc::j( response[ "error" ] ) );
        }

        std::unique_lock< std::recursive_mutex > lock( m_heapMutex );
        m_waitingResponse = false;

        if( m_resendSubscriptionListTask ) {
            m_resendSubscriptionListTask->stop();
            m_resendSubscriptionListTask.reset();
        }

        if( responseError ) {
            LOGGER_DEBUG_RAW( cc::debug( "Subscription list resend scheduled" ) );
            m_resendSubscriptionListTask = m_service.createPostponedTask( [this]( Services::PostponedTask& ) {
                LOGGER_DEBUG_RAW( cc::debug( "Resending subscription list" ) );
                sendSubscriptionList();
            } );
            auto& delay =
                ( m_retries++ < retriesBurstLimit ) ? subscriptionListResendDelay : subscriptionListResendLongDelay;
            m_resendSubscriptionListTask->start( delay, false );
        }
        else if( m_messageHeap.size() > 1 ) {
            LOGGER_DEBUG_RAW( cc::debug( "Processing next subscription list request" ) );
            m_retries = 0;
            sendSubscriptionList();
        }
        else {
            m_retries = 0;
            m_messageHeap.clear();
        }
    };

    m_messageHeap.resize( 1 );
    m_waitingResponse = true;

    m_service.sendAsyncRequest( GroupsNames::UI_GROUP,
                                m_messageHeap.front(),
                                CLOUD_MESSAGE,
                                std::move( responseHandler ),
                                subscriptionListTimout );
}

void SubscriptionHelper::tearDown() {
    std::unique_lock< std::recursive_mutex > lock( m_heapMutex );
    m_messageHeap.clear();
    if( m_resendSubscriptionListTask ) {
        m_resendSubscriptionListTask->stop();
        m_resendSubscriptionListTask.reset();
    }
}

void SubscriptionHelper::sendExecutePluginScript( const std::string& script, const Json& scriptParams ) {
    LOGGER_DEBUG_RAW( cc::debug( "Execute script: " ) + cc::p( script ) + cc::debug( ", with params: " ) +
                      cc::j( scriptParams ) );
    auto data = prepareMethod( "executeScript" );
    data[ "params" ][ "script" ] = script;
    if( !scriptParams.empty() )
        data[ "params" ][ "scriptParams" ] = scriptParams;

    Services::Message message = { "addon.lua", JSON_RPC, CALL_METHOD, data };
    m_service.send( message );
}

bool SubscriptionHelper::itemBelongsToGateway( const std::string& itemId, const std::string& gatewayName ) {
    // TODO: EFWR-3917: Cache this value to avoid multiple database searches
    Json itemDbRecord = this->getItem( itemId );
    if( itemDbRecord.empty() ) {
        return false;
    }
    auto deviceId = json_get_str( itemDbRecord, CoreD::Model::Resources::ItemFields::DeviceIdFieldName );
    Json deviceDbRecord = this->getDevice( deviceId );
    if( deviceDbRecord.empty() ) {
        return false;
    }
    auto itemGatewayId = json_get_str( deviceDbRecord, CoreD::Model::Resources::DeviceFields::GatewayIdFieldName );
    if( itemGatewayId.empty() ) {
        return false;
    }
    auto gatewayDbRecord = this->getGatewayById( itemGatewayId );
    if( gatewayDbRecord.empty() ) {
        return false;
    }
    auto itemGatewayName = json_get_str( gatewayDbRecord, CoreD::Model::Resources::GatewayFields::NameFieldName );
    if( itemGatewayName == gatewayName ) {
        return true;
    }
    return false;
}

}  // namespace CloudDevices
}  // namespace HubPlatform
