#include "security_config_interface.h"
#include <unistd.h>
#include "jsonrpc/error_codes.h"
#include "jsonrpc/jsonrpc.h"
#include "cored/model/resources/cored_resources.h"
#include "crypto_utils/digests.h"
#include "service_interfaces/service.h"
#include "cored/protect/resources.h"
#include "cored/protect/protect_utils.h"
#include "cored/protect/security_config.h"

namespace HubPlatform {
namespace CoreD {
namespace Protect {

const char* const Ids = "ids";

ProtectSecurityConfigInterface::ProtectSecurityConfigInterface( Services::Service& service )
: m_hostService( service ) {}

ProtectSecurityConfigInterface::~ProtectSecurityConfigInterface() = default;

JsonObject ProtectSecurityConfigInterface::list( const JsonObject& params ) {
    using namespace Resources;
    Json answer;
    std::set< std::string > ids;
    JsonArray strings;

    if( json_get( params, Ids, strings ) ) {
        if( !strings.is_array() ) {
            setErrorWithField( answer, Errors::WRONG_PARAMS, "Wrong value type", Ids );
            return answer;
        }
        try {
            fromJsonStringArray( strings, [&ids]( const std::string& value ) { ids.emplace( value ); } );
        }
        catch( std::exception& ex ) {
            std::string reason = "Wrong value type: ";
            reason += ex.what();
            setErrorWithField( answer, Errors::WRONG_PARAMS, reason, Ids );
            return answer;
        }
    }

    std::string versionParams;
    const Json& version = params[ "version" ];
    if( !version.empty() ) {
        if( !version.is_string() ) {
            setErrorWithField( answer, Errors::WRONG_PARAMS, "Wrong value type", "version" );
            return answer;
        }
        versionParams = json_get_str( params, "version" );
    }

    bool filterPersistent = false;
    JsonObject persistent;
    bool persistentValue = false;
    if( json_get( params, SecurityConfig::Persistent, persistent ) ) {
        if( !persistent.is_boolean() ) {
            setErrorWithField( answer, Errors::WRONG_PARAMS, "Wrong value type", SecurityConfig::Persistent );
            return answer;
        }
        filterPersistent = true;
        persistentValue = persistent.get< bool >();
    }

    std::string armedWhenValue;
    JsonObject armedWhen;
    if( json_get( params, SecurityConfig::ArmedWhen, armedWhen ) ) {
        if( !armedWhen.is_string() ) {
            setErrorWithField( answer, Errors::WRONG_PARAMS, "Wrong value type", SecurityConfig::ArmedWhen );
            return answer;
        }
        armedWhenValue = armedWhen.get< std::string >();
    }

    using namespace Model::Resources;
    auto securityConfigsArray =
        m_hostService.getDBClient().findAll( DBNames::SecurityConfigsDB, JSON_EMPTY_OBJ, nullptr );
    JsonArrayBuilder filteredArray;
    std::stringstream ss;

    auto processParams = [&filteredArray, &ss]( const JsonObject& securityConfig ) {
        filteredArray << securityConfig;
        ss << securityConfig;
    };

    for( const auto& securityConfig : securityConfigsArray ) {
        auto id = json_get_str( securityConfig, "_id" );
        auto securityConfigPersistent = json_get_bool( securityConfig, SecurityConfig::Persistent );
        JsonArray armedWhenArray;
        json_get_arr( securityConfig, SecurityConfig::ArmedWhen, armedWhenArray );

        bool matchesId = ids.empty() || ids.count( id );
        bool matchesPersistent = !filterPersistent || ( securityConfigPersistent == persistentValue );
        bool matchesArmedWhen =
            armedWhenValue.empty() ||
            std::any_of( armedWhenArray.begin(), armedWhenArray.end(), [&armedWhenValue]( const Json& value ) {
                return value.get< std::string >() == armedWhenValue;
            } );

        if( matchesId && matchesPersistent && matchesArmedWhen ) {
            processParams( securityConfig );
        }
    }

    ss << getpid();
    auto hash = CryptoUtils::Digests::sha1( ss );

    if( !version.empty() && hash == versionParams ) {
        return JSON( "version" << hash );
    }

    return JSON( "version" << hash << "securityConfigs" << filteredArray );
}

bool ProtectSecurityConfigInterface::isBypassSet( const std::string& securityConfigId ) {
    using namespace Resources;

    auto securityConfig = ProtectUtils::getSecurityConfigObject( m_hostService.getDBClient(), securityConfigId );
    SecurityConfigInfo securityConfigInfo;
    securityConfigInfo.fromJson( securityConfig );
    auto signal = securityConfigInfo.getSignalObject();
    JsonObject bypass;
    json_get_obj( signal, SignalFields::Bypass, bypass );
    return json_get_bool( bypass, "send" );
}
}  // namespace Protect
}  // namespace CoreD
}  // namespace HubPlatform