#include "zwave/zwave_node_setting.h"
#include "json_utils/json_custom_getters.h"
#include "zwave/resources/zwave_resources.h"

#include "log_utils/ezlo_log.h"
#include "database_client/idbclient.h"

#include <unordered_set>

namespace HubPlatform {
namespace Zwave {
namespace {
std::string createNodeTypeId( uint16_t manufacturerId, uint16_t productType, uint16_t productId ) {
    return std::to_string( manufacturerId ) + "_" + std::to_string( productType ) + "_" + std::to_string( productId );
}
}  // namespace

std::unique_ptr< NodeSetting > NodeSetting::create( const Json& settings,
                                                    IDbClient& dbClient,
                                                    uint16_t manufacturerId,
                                                    uint16_t productType,
                                                    uint16_t productId ) {
    auto typeId = createNodeTypeId( manufacturerId, productType, productId );

    auto it = settings.find( typeId );
    const auto jsonNodeSettings = dbClient.findOne(
        Resources::DB::TableNames::DeviceSettings, JSON( Resources::DB::Fields::Id << typeId ), nullptr );

    if( it == settings.end() && jsonNodeSettings.empty() )
        return nullptr;

    return std::unique_ptr< NodeSetting >{ new NodeSetting{ it == settings.end() ? JSON_EMPTY_OBJ : *it,
                                                            jsonNodeSettings } };
}

NodeSetting::NodeSetting( Json setting, Json databaseSetting )
: m_setting( std::move( setting ) )
, m_databaseSetting( databaseSetting ) {
    auto it = m_setting.find( "ignore_cc_interview" );
    if( it != m_setting.end() ) {
        for( auto const& ccId : *it ) {
            m_ignoreCcInterview.emplace( ccId.get< uint8_t >() );
        }
    }
}

NodeSetting::~NodeSetting() = default;

void NodeSetting::forEachConfiguration( std::function< void( const znet::Configuration& ) > callback ) const {
    std::unordered_set< uint8_t > processedParameters;

    auto forEach = [&processedParameters, &callback]( const Json& setting ) {
        auto it = setting.find( "configurations" );
        if( it == setting.end() )
            return;

        using namespace JsonCustomGetters;
        using Configuration = znet::Configuration;

        const auto getter =
            makeStructGetter( makeMemberMapping( &Configuration::paramNumber, "number" ),
                              makeMemberMapping( &Configuration::size, "size", OptionalMemberPolicy( false ) ),
                              makeMemberMapping( &Configuration::value, "value" ) );

        for( const auto& joConfig : *it ) {
            Configuration configuration;

            if( getter( joConfig, configuration, false ) ) {
                if( processedParameters.insert( configuration.paramNumber ).second ) {
                    callback( configuration );
                }
            }
            else {
                Log::error( "Invalid json configuration: " + joConfig.dump() );
            }
        }
    };

    forEach( m_databaseSetting );
    forEach( m_setting );
}

void NodeSetting::forEachAssociation( std::function< void( const znet::Association& ) > callback ) const {
    using AssociationTuple = std::tuple< uint8_t, uint8_t, uint8_t >;
    std::set< AssociationTuple > processedAssociations;

    auto forEach = [&processedAssociations, &callback]( const Json& setting ) {
        auto it = setting.find( "associations" );
        if( it == setting.end() )
            return;

        using namespace JsonCustomGetters;
        using Association = znet::Association;

        const auto getter = makeStructGetter(
            makeMemberMapping( &Association::groupId, "group_id" ),
            makeMemberMapping( &Association::channelId, "channel_id", OptionalMemberPolicy( false ) ),
            makeMemberMapping( &Association::targetChannelId, "target_channel_id", OptionalMemberPolicy( false ) ) );

        for( const auto& joAssociation : *it ) {
            Association association;
            if( getter( joAssociation, association, false ) ) {
                if( processedAssociations
                        .insert( AssociationTuple{
                            association.channelId, association.groupId, association.targetChannelId } )
                        .second ) {
                    callback( association );
                }
            }
            else {
                Log::error( "Invalid json association: " + joAssociation.dump() );
            }
        }
    };

    forEach( m_databaseSetting );
    forEach( m_setting );
}

znet::utils::optional< uint32_t > NodeSetting::getWakeUpInterval() const {
    auto it = m_setting.find( "wake_up_interval" );
    return it == m_setting.end() ? znet::utils::optional< uint32_t >() : it->get< uint32_t >();
}

znet::utils::optional< bool > NodeSetting::shouldRequestNotifications() const {
    auto it = m_setting.find( "should_request_notifications" );
    return it == m_setting.end() ? znet::utils::optional< bool >() : it->get< bool >();
}

bool NodeSetting::ignoreCcInterciew( uint8_t ccId ) const {
    return m_ignoreCcInterview.find( ccId ) != m_ignoreCcInterview.end();
}

znet::utils::optional< bool > NodeSetting::isExtendedUserCodesRequestRestricted() const {
    auto supportedUserCodesIt = m_setting.find( "supported_user_codes" );

    if( supportedUserCodesIt == m_setting.end() || !supportedUserCodesIt->is_object() ) {
        return {};
    }

    auto it = supportedUserCodesIt->find( "extended_user_codes_request_restricted" );

    return it == supportedUserCodesIt->end() ? znet::utils::optional< bool >() : it->get< bool >();
}

}  // namespace Zwave
}  // namespace HubPlatform
