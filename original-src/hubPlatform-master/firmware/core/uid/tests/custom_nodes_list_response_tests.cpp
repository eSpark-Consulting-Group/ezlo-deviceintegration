#include "uid/response_builders/custom_nodes/nodes_list_response_builder.h"
#include "service_interfaces/tests/fakes/fake_service.h"
#include "extensions_manager/plugin_model/model_resources.h"
#include "database_client/dbqueryparser.h"
#include "service_interfaces/service.h"
#include "json_utils/json_utils.h"
#include "database_client/dbid.h"

namespace HubPlatform {
namespace CustomNodes {
namespace UT {

using namespace ::testing;
using namespace ::ExtensionsManager::Resources::ConfigFields;
using namespace HubPlatform::Uid::CustomNodes;

class CustomNodesListBuilderTestSuite : public Test {
public:
    std::string createDbItem( const std::string& pluginName,
                              const std::string& widePluginName,
                              const std::string& author ) {
        JsonObject data;

        JsonObject placement = JSON( "custom" << true );

        std::string generatedId = db::DbId::genOID();
        data = { { db::ID_FIELD_NAME, generatedId },
                 { DbFields::Plugin, pluginName },
                 { DbFields::ShortName, pluginName },
                 { DbFields::MetaName, widePluginName },
                 { DbFields::Version, "version" },
                 { DbFields::MetaDescription, "description" },
                 { DbFields::MetaAuthor, author },
                 { DbFields::MetaPrefix, "prefix" },
                 { DbFields::MetaSize, 123 },
                 { DbFields::MetaType, "type" },
                 { DbFields::MetaLanguage, "language" },
                 { DbFields::Placement, placement } };

        std::string configurationScript = "HUB:" + pluginName + "/scripts/init.lua";
        data[ Root::Configuration ] = JSON( "type"
                                            << "static"
                                            << "script" << configurationScript << "inputs" << Json::array() );

        m_fakeService.getDBClient().insert( HaExtensionsManagerCustomNodesDBName, data );

        return generatedId;
    }

    JsonObject createResponse( const std::string& pluginName,
                               const std::string& author,
                               const std::set< std::string >& ids,
                               bool showMeta,
                               bool showAdvanced ) {
        NodesListResponseBuilder responseBuilder( getService() );

        responseBuilder.setPluginName( pluginName );
        responseBuilder.setPluginShortName( pluginName );
        responseBuilder.setAuthorFilteringParam( author );
        responseBuilder.setIds( ids );
        responseBuilder.setShowMeta( showMeta );
        responseBuilder.setShowAdvanced( showAdvanced );
        auto res = responseBuilder.getDataWithFilter();
        auto nodes = res[ "nodes" ];
        return nodes;
    }

    Services::UT::FakeService& getService() {
        return m_fakeService;
    }

private:
    Services::UT::FakeService m_fakeService;
};

TEST_F( CustomNodesListBuilderTestSuite, getDataSuccess ) {
    const char* author = "author";
    auto id0 = createDbItem( "Siren0", "Siren Try 0", author );
    auto id1 = createDbItem( "Siren1", "Siren Try 1", author );
    auto id2 = createDbItem( "Siren2", "Siren Try 2", author );

    auto nodes = createResponse( "", author, { id0 }, true, true );
    EXPECT_EQ( nodes.size(), 1 ) << "filtered by id, author";

    nodes = createResponse( "", "", { id0, id1, id2 }, true, true );
    EXPECT_EQ( nodes.size(), 3 ) << "filtered by ids";

    nodes = createResponse( "Siren0", "", {}, true, true );
    EXPECT_EQ( nodes.size(), 1 ) << "filtered by pluginName";
    nodes = createResponse( "Siren1", "", {}, true, true );
    EXPECT_EQ( nodes.size(), 1 ) << "filtered by pluginName";
    nodes = createResponse( "Siren2", "", {}, true, true );
    EXPECT_EQ( nodes.size(), 1 ) << "filtered by pluginName";

    nodes = createResponse( "", "", {}, true, true );
    EXPECT_EQ( nodes.size(), 3 ) << "all records, all fields";

    ASSERT_TRUE( nodes.is_array() );
    ASSERT_FALSE( nodes.empty() );

    for( auto& node : nodes ) {
        ASSERT_FALSE( json_get_str( node, db::ID_FIELD_NAME ).empty() );
        ASSERT_FALSE( json_get_str( node, DbFields::Plugin ).empty() );
        ASSERT_FALSE( json_get_str( node, DbFields::ShortName ).empty() );
        ASSERT_FALSE( json_get_str( node, DbFields::Version ).empty() );

        JsonObject meta;
        ASSERT_TRUE( json_get_obj( node, Root::Meta, meta ) );

        ASSERT_EQ( json_get_str( meta, Meta::Author ), author );
        ASSERT_TRUE( json_get_str( meta, Meta::Name ).find( "Siren Try " ) != std::string::npos );
        ASSERT_EQ( json_get_str( meta, Meta::Description ), "description" );
        ASSERT_EQ( json_get_str( meta, Meta::Language ), "language" );
        ASSERT_EQ( json_get_str( meta, Meta::Prefix ), "prefix" );
        ASSERT_EQ( json_get_str( meta, Meta::Type ), "type" );
        ASSERT_EQ( json_get_int( meta, Meta::Size ), 123 );

        JsonObject placement;
        ASSERT_TRUE( json_get_obj( meta, Meta::Placement, placement ) );
        ASSERT_TRUE( placement.find( "custom" ) != placement.end() );
        ASSERT_TRUE( json_get_bool( placement, "custom" ) == true );

        JsonObject configuration;
        ASSERT_TRUE( json_get_obj( node, Root::Configuration, configuration ) );

        ASSERT_FALSE( json_get_str( configuration, "script" ).empty() );
        ASSERT_FALSE( json_get_str( configuration, "type" ).empty() );
        JsonArray inputs;
        ASSERT_TRUE( json_get_arr( configuration, "inputs", inputs ) );
    }
}

TEST_F( CustomNodesListBuilderTestSuite, getDataFailedName ) {
    createDbItem( "Siren", "name", "author" );
    createDbItem( "Siren", "name2", "author" );
    createDbItem( "Siren", "name", "author3" );

    auto nodes = createResponse( "Siren22", "", {}, true, true );

    ASSERT_TRUE( nodes.is_array() );
    ASSERT_TRUE( nodes.empty() );
}

TEST_F( CustomNodesListBuilderTestSuite, getDataFailedAuthor ) {
    createDbItem( "Siren", "name", "author" );
    createDbItem( "Siren", "name2", "author" );
    createDbItem( "Siren", "name", "author3" );

    auto nodes = createResponse( "", "author2", {}, true, true );

    ASSERT_TRUE( nodes.is_array() );
    ASSERT_TRUE( nodes.empty() );
}

TEST_F( CustomNodesListBuilderTestSuite, getDataFailedId ) {
    createDbItem( "Siren", "name", "author" );
    createDbItem( "Siren", "name2", "author" );
    createDbItem( "Siren", "name", "author3" );

    auto nodes = createResponse( "", "", { "Nonexistent" }, true, true );

    ASSERT_TRUE( nodes.is_array() );
    ASSERT_TRUE( nodes.empty() );
}

TEST_F( CustomNodesListBuilderTestSuite, getDataMultiSelectId ) {
    auto id0 = createDbItem( "Siren", "name", "author" );
    auto id1 = createDbItem( "Siren1", "name", "author" );
    auto id2 = createDbItem( "Siren2", "name", "author" );

    auto nodes = createResponse( "", "", { id0, id2 }, true, true );

    ASSERT_TRUE( nodes.is_array() );
    ASSERT_EQ( nodes.size(), 2 );
}

TEST_F( CustomNodesListBuilderTestSuite, getDataCheckWithoutMetaObjSuccess ) {
    createDbItem( "Siren", "name", "author" );
    createDbItem( "Siren", "name2", "author" );
    createDbItem( "Siren", "name", "author3" );

    auto nodes = createResponse( "", "", {}, false, true );

    ASSERT_FALSE( nodes == JSON_EMPTY_OBJ );
    ASSERT_TRUE( nodes.is_array() );
    ASSERT_EQ( nodes.size(), 3 );

    for( auto& node : nodes ) {
        ASSERT_TRUE( node.find( Root::Meta ) == node.cend() );
    }
}

TEST_F( CustomNodesListBuilderTestSuite, getDataCheckWithoutConfigurationObjSucsess ) {
    createDbItem( "Siren0", "name", "author" );
    createDbItem( "Siren1", "name2", "author" );
    createDbItem( "Siren2", "name", "author3" );

    auto nodes = createResponse( "", "", {}, true, false );

    ASSERT_TRUE( nodes.is_array() );
    ASSERT_EQ( nodes.size(), 3 );

    for( auto& node : nodes ) {
        ASSERT_TRUE( node.find( Root::Configuration ) == node.cend() );
    }
}

TEST_F( CustomNodesListBuilderTestSuite, getDataCheckEmptyFields ) {
    auto id0 = createDbItem( "Siren", "name", "author" );
    auto id1 = createDbItem( "Siren1", "name1", "author" );
    auto id2 = createDbItem( "Siren", "name", "author3" );

    auto nodes = createResponse( "Siren", "author", {}, true, true );

    ASSERT_TRUE( nodes.is_array() );
    ASSERT_EQ( nodes.size(), 1 );

    nodes = createResponse( "Siren", "", {}, true, true );

    ASSERT_TRUE( nodes.is_array() );
    ASSERT_EQ( nodes.size(), 2 );

    nodes = createResponse( "", "author", {}, true, true );

    ASSERT_TRUE( nodes.is_array() );
    ASSERT_EQ( nodes.size(), 2 );

    nodes = createResponse( "", "", {}, true, true );

    ASSERT_TRUE( nodes.is_array() );
    ASSERT_EQ( nodes.size(), 3 );

    nodes = createResponse( "", "", { "Bulb" }, true, true );

    ASSERT_TRUE( nodes.is_array() );
    ASSERT_EQ( nodes.size(), 0 );
}

}  // namespace UT
}  // namespace CustomNodes
}  // namespace HubPlatform
