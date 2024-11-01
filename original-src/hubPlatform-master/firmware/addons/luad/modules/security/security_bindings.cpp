#include "binding_interfaces/lua/lua_binding_interface.h"

#include "crypto_utils/crypto_utils.h"
#include "crypto_utils/aes_cipher.h"
#include "base_utils/utils.h"

#include "common_utils/cpp14.h"

#define SECURITY_LUA_CHECK_INPUT_PARAMETERS( number ) \
    CHECK_INPUT_PARAMETERS( ( std::string{ "security." } + __func__ ), number )

#define SECURITY_LUA_GET_INPUT_PARAMETERS( type, idx ) \
    GET_INPUT_PARAMETERS( type, idx, ( std::string{ "security." } + __func__ ) )

#define SECURITY_LUA_CHECK_INPUT_OPTIONAL_PARAMETERS( number ) \
    CHECK_INPUT_OPTIONAL_PARAMETERS( ( std::string{ "security." } + __func__ ), number )

namespace HubPlatform {
namespace Luad {
namespace {

constexpr static std::size_t InputDataSizeLimit = 1 * 1024 * 1024;  // 1 MB

CryptoUtils::Crypto& getSecurityManager( lua_State* /*luaStates*/ ) {
    return CryptoUtils::Crypto::getInstance();
}

std::unique_ptr< CryptoUtils::Cipher > getCipher( lua_State* luaState, int idx ) {
    using namespace CryptoUtils;
    luaL_checktype( luaState, idx, LUA_TTABLE );
    const auto key = lookupLuaTable< NativeString >( luaState, "key", idx, true ).value();
    const auto mode = lookupLuaTable< NativeString >( luaState, "mode", idx, false ).valueOr( std::string( "cbc" ) );
    const auto iv = lookupLuaTable< NativeString >( luaState, "iv", idx, false ).valueOr( std::string( "" ) );
    const auto padding = lookupLuaTable< NativeBool >( luaState, "auto_padding", idx, false ).valueOr( true );
    return std::make_unique< AesCipher >( AesCipher::toMode( mode ), key, iv, padding );
}

int md5( lua_State* luaState ) {
    SECURITY_LUA_CHECK_INPUT_PARAMETERS( 1 );

    const auto data = SECURITY_LUA_GET_INPUT_PARAMETERS( NativeString, 1 );

    if( data.size() > InputDataSizeLimit ) {
        return luaL_error( luaState, "input data size is more than %d bytes", InputDataSizeLimit );
    }

    try {
        const auto hash = getSecurityManager( luaState ).digests().md5( data );
        lua_pushlstring( luaState, hash.c_str(), hash.size() );

        return 1;
    }
    catch( const std::exception& e ) {
        return luaL_error( luaState, "fail to evaluate '%s' hash : %s", __FUNCTION__, e.what() );
    }
}

int sha1( lua_State* luaState ) {
    SECURITY_LUA_CHECK_INPUT_PARAMETERS( 1 );

    const auto data = SECURITY_LUA_GET_INPUT_PARAMETERS( NativeString, 1 );

    if( data.size() > InputDataSizeLimit ) {
        return luaL_error( luaState, "input data size is more than %d bytes", InputDataSizeLimit );
    }

    try {
        const auto hash = getSecurityManager( luaState ).digests().sha1( data );
        lua_pushstring( luaState, hash.c_str() );

        return 1;
    }
    catch( const std::exception& e ) {
        return luaL_error( luaState, "fail to evaluate '%s' hash : %s", __FUNCTION__, e.what() );
    }
}

int sha256( lua_State* luaState ) {
    SECURITY_LUA_CHECK_INPUT_PARAMETERS( 1 );

    const auto data = SECURITY_LUA_GET_INPUT_PARAMETERS( NativeString, 1 );

    if( data.size() > InputDataSizeLimit ) {
        return luaL_error( luaState, "input data size is more than %d bytes", InputDataSizeLimit );
    }

    try {
        const auto hash = getSecurityManager( luaState ).digests().sha256( data );
        lua_pushstring( luaState, hash.c_str() );

        return 1;
    }
    catch( const std::exception& e ) {
        return luaL_error( luaState, "fail to evaluate '%s' hash : %s", __FUNCTION__, e.what() );
    }
}

int digestByName( lua_State* luaState ) {
    SECURITY_LUA_CHECK_INPUT_PARAMETERS( 2 );

    const auto digestName = SECURITY_LUA_GET_INPUT_PARAMETERS( NativeString, 1 );
    const auto data = SECURITY_LUA_GET_INPUT_PARAMETERS( NativeString, 2 );

    if( digestName.empty() ) {
        return luaL_error( luaState, "digest name is empty" );
    }
    if( data.size() > InputDataSizeLimit ) {
        return luaL_error( luaState, "input data size is more than %d bytes", InputDataSizeLimit );
    }

    try {
        std::string digest;
        auto& digestInstance = CryptoUtils::Digests::getInstance();
        if( CryptoUtils::Digests::calculateDigest( digestName, data, digest ) ) {
            lua_pushstring( luaState, digest.c_str() );
        }

        return 1;
    }
    catch( const std::exception& e ) {
        return luaL_error(
            luaState, "fail to evaluate 'digest_by_name' digest with name '%s' : %s", digestName.c_str(), e.what() );
    }
}

int getAllDigestNames( lua_State* luaState ) {
    SECURITY_LUA_CHECK_INPUT_PARAMETERS( 0 );

    try {
        std::string digest;
        const auto& digestNames = CryptoUtils::Digests::getInstance().supportedDigests();

        if( !digestNames.empty() ) {
            Json result = digestNames;
            return pushJsonAsLuaObj( luaState, result );
        }

        return luaL_error( luaState, "supported digests list is empty" );
    }
    catch( const std::exception& e ) {
        return luaL_error( luaState, "fail to evaluate 'get_all_digest_names' : %s", e.what() );
    }
}

int encrypt( lua_State* luaState ) {
    SECURITY_LUA_CHECK_INPUT_OPTIONAL_PARAMETERS( 2 );

    const auto cipher = getCipher( luaState, 1 );
    const auto data = SECURITY_LUA_GET_INPUT_PARAMETERS( NativeString, 2 );

    if( data.size() > InputDataSizeLimit ) {
        return luaL_error( luaState, "input data size is more than %d bytes", InputDataSizeLimit );
    }

    try {
        auto encryptedData = getSecurityManager( luaState ).encrypt( *cipher, data );
        lua_pushlstring( luaState, encryptedData.data(), encryptedData.size() );
    }
    catch( const std::exception& e ) {
        return luaL_error( luaState, "Encryption failure : %s", e.what() );
    }
    return 1;
}

int decrypt( lua_State* luaState ) {
    SECURITY_LUA_CHECK_INPUT_OPTIONAL_PARAMETERS( 2 );

    const auto cipher = getCipher( luaState, 1 );
    const auto data = SECURITY_LUA_GET_INPUT_PARAMETERS( NativeString, 2 );

    if( data.size() > InputDataSizeLimit ) {
        return luaL_error( luaState, "input data size is more than %d bytes", InputDataSizeLimit );
    }

    try {
        auto decryptedData = getSecurityManager( luaState ).decrypt( *cipher, data );
        lua_pushlstring( luaState, decryptedData.data(), decryptedData.size() );
    }
    catch( const std::exception& e ) {
        return luaL_error( luaState, "Decryption failure : %s", e.what() );
    }
    return 1;
}

int to_base64( lua_State* luaState ) {
    SECURITY_LUA_CHECK_INPUT_OPTIONAL_PARAMETERS( 1 );
    const auto data = SECURITY_LUA_GET_INPUT_PARAMETERS( NativeString, 1 );
    if( data.size() > InputDataSizeLimit ) {
        return luaL_error( luaState, "input data size is more than %d bytes", InputDataSizeLimit );
    }
    lua_pushstring( luaState, getSecurityManager( luaState ).toBase64( data ).data() );
    return 1;
}

int from_base64( lua_State* luaState ) {
    SECURITY_LUA_CHECK_INPUT_OPTIONAL_PARAMETERS( 1 );
    const auto data = SECURITY_LUA_GET_INPUT_PARAMETERS( NativeString, 1 );
    if( data.size() > InputDataSizeLimit ) {
        return luaL_error( luaState, "input data size is more than %d bytes", InputDataSizeLimit );
    }
    auto result = getSecurityManager( luaState ).fromBase64( data );
    lua_pushlstring( luaState, result.data(), result.size() );
    return 1;
}

int fromHexString( lua_State* luaState ) {
    SECURITY_LUA_CHECK_INPUT_OPTIONAL_PARAMETERS( 1 );
    const auto data = SECURITY_LUA_GET_INPUT_PARAMETERS( NativeString, 1 );
    if( data.size() > InputDataSizeLimit ) {
        return luaL_error( luaState, "input data size is more than %d bytes", InputDataSizeLimit );
    }

    std::string result;
    if( !data.empty() && !getBinaryFromHex( data, result ) ) {
        return luaL_error( luaState, "input data couldn't be transformed to binary" );
    }
    lua_pushlstring( luaState, result.data(), result.size() );
    return 1;
}

constexpr char moduleName[] = "security";
constexpr luaL_Reg module[] = { { "md5", errorCatchWrapper< md5 > },
                                { "sha1", errorCatchWrapper< sha1 > },
                                { "sha256", errorCatchWrapper< sha256 > },
                                { "digest_by_name", errorCatchWrapper< digestByName > },
                                { "get_all_digest_names", errorCatchWrapper< getAllDigestNames > },
                                { "encrypt", errorCatchWrapper< encrypt > },
                                { "decrypt", errorCatchWrapper< decrypt > },
                                { "to_base64", errorCatchWrapper< to_base64 > },
                                { "from_base64", errorCatchWrapper< from_base64 > },
                                { "from_hex_string", errorCatchWrapper< fromHexString > },
                                { nullptr, nullptr } };

}  // namespace
}  // namespace Luad
}  // namespace HubPlatform

HUB_BASE_MODULE_ENTRY_DEFINITION( security, HubPlatform::Luad::module, HubPlatform::Luad::moduleName );
