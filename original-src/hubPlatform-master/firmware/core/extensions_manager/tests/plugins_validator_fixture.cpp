#include "plugins_validator_fixture.h"
#include "common_utils/cpp14.h"
#include "test_utils/component_tests/common_nma_test_fixture.h"
#include "extensions_manager/plugin_model/model_resources.h"

#include <gmock/gmock.h>

namespace ExtensionsManager {
namespace UT {

using ::testing::AtLeast;
using ::testing::Return;
using namespace HubPlatform::Tests;

namespace Resources {
const std::string configJson = R"(
        {
            "id": __ID__,
            "version": __VER__,
            "meta": {
                "name": __NAME__ ,
                "description": __DESC__ ,
                "author": __AUTH__ ,
                "prefix": __PREF__,
                "size": __SIZE__,
                "type": __TYPE__,
                "language": __LANG__,
                "placement": __PLC__
            }
        }
        )";

const std::string interfacesJson = R"(
{
    "configuration":{
        "type":__TYPE__,
        "script":__SCRIPT__,
        "inputs":[
                    {
                        "name":__NAME__,
                        "description":__DESC__,
                        "field":__FIELD__,
                        "required":__REQ__,
                        "type":__FIELD_TYPE__
                        __COMMA_FORMAT_PAIR__
                    }
        ]
    }
})";

const std::string pluginDataJsonConfig = R"(
{
        "version": "1.0",
        "id": "pluginPrefixValue_siren",
        "meta": {
            "name": { "text": "TestSiren plugin" },
            "description": { "text": "This plugin is intended to show up in Nodes list only" },
            "author": { "text": "Ezlo Innovation" },
            "prefix": "pluginPrefixValue",
            "size": 34252525,
            "type": "node",
            "language": "lua",
            "placement": {"static": true, "custom": true}
        }
})";

const std::string pluginDataJsonInterfaces = R"(
{
    "configuration":{
                  "type":"none",
                  "script":"HUB:plugin_name/path/to/configuration/script.lua",
                  "inputs":[
                     {
                        "name":"field",
                        "description":"Description of a field",
                        "field":"Name of a field",
                        "required":true,
                        "type":"boolean",
                        "format":"ip"
                     }
                  ]
   }
}
)";
}  // namespace Resources

std::string PluginsValidatorFixture::createConfigMeta( std::string meta,
                                                       std::string name,
                                                       std::string desc,
                                                       std::string auth,
                                                       std::string pref,
                                                       std::string size,
                                                       std::string type,
                                                       std::string lang,
                                                       std::string vers,
                                                       std::string plac,
                                                       std::string id ) {
    auto configJsonModify = CommonNmaTests::replaceTokensInString( Resources::configJson,
                                                                   { { "meta", meta },
                                                                     { "__NAME__", name },
                                                                     { "__ID__", id },
                                                                     { "__DESC__", desc },
                                                                     { "__AUTH__", auth },
                                                                     { "__PREF__", pref },
                                                                     { "__SIZE__", size },
                                                                     { "__TYPE__", type },
                                                                     { "__LANG__", lang },
                                                                     { "__VER__", vers },
                                                                     { "__PLC__", plac } } );

    return configJsonModify;
}

std::string PluginsValidatorFixture::createInterfaces( std::string config,
                                                       std::string script,
                                                       std::string field,
                                                       std::string format,
                                                       std::string field_type,
                                                       std::string name,
                                                       std::string desc,
                                                       std::string type,
                                                       std::string req ) {
    auto meta = CommonNmaTests::replaceTokensInString( Resources::interfacesJson,
                                                       { { "configuration", config },
                                                         { "__SCRIPT__", script },
                                                         { "__FIELD__", field },
                                                         { "__FIELD_TYPE__", field_type },
                                                         { "__NAME__", name },
                                                         { "__DESC__", desc },
                                                         { "__TYPE__", type },
                                                         { "__REQ__", req } } );

    std::string formatSubstitution = "";
    if( !format.empty() ) {
        formatSubstitution = R"(, "format": )" + format;
    }
    auto response = CommonNmaTests::replaceTokensInString( meta, { { "__COMMA_FORMAT_PAIR__", formatSubstitution } } );

    return response;
}

namespace Resources {
const char* const PluginPrefix = "pluginPrefixValue";
const char* const PluginPrefixedName = "pluginPrefixValue_siren";
}  // namespace Resources

/** TODO: author = "Ezlo Innovation" */
bool PluginsValidatorFixture::checkDbItem( const JsonObject& json,
                                           const std::string& author,
                                           const char* file,
                                           int line ) {
    using namespace ExtensionsManager::Resources::ConfigFields;
    std::map< std::string, std::string > table = { { DbFields::Plugin, Resources::PluginPrefixedName },
                                                   { DbFields::MetaAuthor, author },
                                                   { DbFields::MetaDescription,
                                                     "This plugin is intended to show up in Nodes list only" },
                                                   { DbFields::MetaPrefix, Resources::PluginPrefix },
                                                   { DbFields::MetaSize, "34252525" },
                                                   { DbFields::MetaType, "node" },
                                                   { DbFields::MetaName, "TestSiren plugin" },
                                                   { Root::Configuration, "{}" },
                                                   { DbFields::Version, "1.0" } };
    std::set< std::string > intFields{ DbFields::MetaSize };
    std::set< std::string > jsonFields{ Root::Configuration, DbFields::Placement };

    for( auto it = table.begin(); it != table.end(); it++ ) {
        if( json.find( it->first ) != json.cend() ) {
            const auto& key = it->first;
            const auto& jsonVal = json[ it->first ];

            if( intFields.count( key ) == 1 ) {
                int val = stoi( it->second );
                if( val != jsonVal.get< JsonObject::number_integer_t >() ) {
                    ADD_FAILURE_AT( file, line ) << "field '" << key << "' is not an int: " << jsonVal.dump()
                                                 << ", at: " << __FILE__ << ":" << __LINE__;
                    return false;
                }
            }
            else if( jsonFields.count( key ) == 1 ) {
            }
            else {
                if( jsonVal.get< JsonObject::string_t >() != it->second ) {
                    ADD_FAILURE_AT( file, line ) << "field '" << key << "' is not a string: " << jsonVal.dump()
                                                 << ", at: " << __FILE__ << ":" << __LINE__;
                    return false;
                }
            }
        }
        else {
            ADD_FAILURE_AT( file, line ) << "has no key '" << it->first << "'"
                                         << ", at:" << __FILE__ << ":" << __LINE__;
            return false;
        }
    }

    return true;
}

}  // namespace UT
}  // namespace ExtensionsManager
