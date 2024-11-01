#include "2gig/2gig_devices_repository.h"
#include "2gig/resources/resources.h"

#include "2gig/peristence_client.h"
#include "2gig/resources/enum_utils.h"

#include "database_client/idbclient.h"
#include "database_client/dbqueryparser.h"
#include "common_utils/cpp14.h"
#include "log_utils/ezlo_log.h"
#include "json_utils/json_utils.h"
#include "tiny/tiny_consolecolors.h"

#include <algorithm>

namespace HubPlatform {
namespace TwoGig {
namespace {

Json generateDeviceRequest( uint32_t serial ) {
    return JSON( db::ID_FIELD_NAME << std::to_string( serial ) );
}

bool containsDigit( const std::string& input ) {
    return std::any_of( input.begin(), input.end(), []( unsigned char c ) { return std::isdigit( c ); } );
}

}  // namespace

using namespace Resources::DB;

DevicesRepositoryImpl::DevicesRepositoryImpl( IDbClient& database )
: m_database( database ) {}

DevicesRepositoryImpl::~DevicesRepositoryImpl() = default;

bool DevicesRepositoryImpl::addDevice( const std::vector< uint32_t >& txIds ) {
    if( std::any_of( txIds.begin(), txIds.end(), [this]( uint32_t txId ) { return hasDevice( txId ); } ) )
        return false;

    for( uint32_t txId : txIds )
        m_database.insert( TableNames::Devices, generateDeviceRequest( txId ) );

    return true;
}

bool DevicesRepositoryImpl::addDevice( const std::vector< uint32_t >& txIds, ::TwoGig::DeviceType::Enum deviceType ) {
    if( std::any_of( txIds.begin(), txIds.end(), [this]( uint32_t txId ) { return hasDevice( txId ); } ) )
        return false;

    for( uint32_t txId : txIds ) {
        Json deviceStructure = generateDeviceRequest( txId );
        deviceStructure[ Fields::Type ] = Resources::EnumUtils< ::TwoGig::DeviceType >::toString( deviceType );
        m_database.insert( TableNames::Devices, deviceStructure );
    }

    return true;
}

bool DevicesRepositoryImpl::removeDevice( const std::vector< uint32_t >& txIds ) {
    if( !std::all_of( txIds.begin(), txIds.end(), [this]( uint32_t txId ) { return hasDevice( txId ); } ) )
        return false;

    for( uint32_t txId : txIds )
        m_database.remove( TableNames::Devices, generateDeviceRequest( txId ), true, false );

    return true;
}

bool DevicesRepositoryImpl::hasDevice( uint32_t txId ) const {
    return m_database.has( TableNames::Devices, generateDeviceRequest( txId ) );
}

void DevicesRepositoryImpl::forEachDevice( std::function< void( uint32_t ) > callback ) const {
    if( !callback ) {
        return;
    }

    const Json requiredFields = JSON( db::ID_FIELD_NAME << 0 );
    const std::vector< Json > allNodes = m_database.findAll( TableNames::Devices, db::Query(), &requiredFields );

    for( auto const& device : allNodes ) {
        auto strId = json_get_str( device, db::ID_FIELD_NAME );
        if( !strId.empty() && containsDigit( strId ) ) {
            callback( std::stoul( strId ) );
        }
    }
}

::TwoGig::DeviceType::Enum DevicesRepositoryImpl::getDeviceType( uint32_t txId ) const {
    auto record = m_database.findOne( TableNames::Devices, generateDeviceRequest( txId ), nullptr );

    auto typeStr = json_get_str( record, Fields::Type );

    return typeStr.empty() ? ::TwoGig::DefaultConfiguration::DeviceType :
                             Resources::EnumUtils< ::TwoGig::DeviceType >::fromString( typeStr );
}

uint8_t DevicesRepositoryImpl::getPinCodeLength( uint32_t txId ) const {
    auto record = m_database.findOne( TableNames::Devices, generateDeviceRequest( txId ), nullptr );

    return json_get_number< uint8_t >( record, Fields::PinCodeLength, ::TwoGig::DefaultConfiguration::PinCodeLength );
}

void DevicesRepositoryImpl::setPinCodeLength( uint32_t txId, uint8_t length ) {
    updateDevice( txId, JSON( Fields::PinCodeLength << length ) );
}

std::chrono::seconds DevicesRepositoryImpl::getPinCodeInputTimeout( uint32_t txId ) const {
    auto record = m_database.findOne( TableNames::Devices, generateDeviceRequest( txId ), nullptr );

    auto seconds = json_get_number< std::chrono::seconds::rep >(
        record, Fields::PinCodeInputTimeout, ::TwoGig::DefaultConfiguration::PinCodeInputTimeout.count() );

    return std::chrono::seconds( seconds );
}

void DevicesRepositoryImpl::setPinCodeInputTimeout( uint32_t txId, std::chrono::seconds duration ) {
    updateDevice( txId, JSON( Fields::PinCodeInputTimeout << duration.count() ) );
}

void DevicesRepositoryImpl::updateDevice( uint32_t txId, const Json& diff ) {
    m_database.update( TableNames::Devices, generateDeviceRequest( txId ), JSON( "$set" << diff ) );
}

void DevicesRepositoryImpl::reset() {
    m_database.clear( TableNames::Devices );
}

bool DevicesRepositoryImpl::migrateOldDevices(::TwoGig::PersistenceClient& oldStorage ) {
    size_t size = 0;

    oldStorage.readBytes( (uint8_t*) &size, sizeof( size_t ) );

    Log::info( cc::num10( size ) + cc::info( " devices was found in old storage." ) );

    uint32_t deviceTtx = 0;
    for( size_t i = 0; i != size; ++i, deviceTtx = 0 ) {
        oldStorage.readBytes( (uint8_t*) &deviceTtx, sizeof( deviceTtx ) );
        if( deviceTtx == 0 ) {
            Log::warn( cc::warn( "TTX-ID of old device is wrong: " ) + cc::num10( deviceTtx ) );
            return false;
        }

        if( !addDevice( { deviceTtx } ) ) {
            Log::warn( cc::warn( "TTX-ID of old device already exists in the new storage: " ) +
                       cc::num10( deviceTtx ) );
        }
    }

    return true;
}

}  // namespace TwoGig
}  // namespace HubPlatform
