#include <unistd.h>
#include "contact_id_interface.h"
#include "jsonrpc/error_codes.h"
#include "jsonrpc/jsonrpc.h"
#include "cored/model/resources/cored_resources.h"
#include "crypto_utils/digests.h"
#include "service_interfaces/service.h"

namespace HubPlatform {
namespace CoreD {
namespace Protect {
ProtectContactIdInterface::ProtectContactIdInterface( Services::Service& service )
: m_hostService( service ) {}

ProtectContactIdInterface::~ProtectContactIdInterface() = default;

JsonObject ProtectContactIdInterface::list( const JsonObject& params ) {
    Json answer;
    std::string versionParams;
    const Json& version = params[ "version" ];
    if( !version.empty() ) {
        if( !version.is_string() ) {
            setErrorWithField( answer, Errors::WRONG_PARAMS, "Wrong value type", "version" );
            return answer;
        }
        versionParams = json_get_str( params, "version" );
    }

    using namespace Model::Resources;
    auto contactIds = m_hostService.getDBClient().findAll( DBNames::ContactIdsDB, JSON_EMPTY_OBJ, nullptr );
    std::stringstream ss;
    ss << contactIds;

    ss << getpid();

    auto hash = CryptoUtils::Digests::sha1( ss );

    if( !version.empty() && hash == versionParams ) {
        return JSON( "version" << hash );
    }

    return JSON( "version" << hash << "aliases" << contactIds );
}

}  // namespace Protect
}  // namespace CoreD
}  // namespace HubPlatform