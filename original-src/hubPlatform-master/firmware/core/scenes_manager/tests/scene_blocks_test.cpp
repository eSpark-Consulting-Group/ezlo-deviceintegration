#include "scenes_manager/scenes_engine/scene_blocks.h"
#include "scenes_manager/tests/resources/generic_scene_json.h"

#include "common_utils/cpp14.h"

#include <sstream>
#include <iostream>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace SceneEngine {
namespace UT {

using namespace testing;

class SceneBlocksTest : public Test {
public:
    using MethodName = std::string;
    using ItemId = std::string;
    using MethodsNumber = int;
    using OptionItem = std::tuple< SceneBlocks::ItemId, SceneBlocks::ItemName, std::string >;
    using MethodItemsMap = std::map< MethodName, std::vector< ItemId > >;
    using MethodNumberMap = std::map< MethodName, MethodsNumber >;
    using MethodOptionItemMap = std::map< MethodName, std::vector< OptionItem > >;

    static void getTestInfo( const JsonArray& blocks,
                             MethodItemsMap& methodItemsMap,
                             MethodNumberMap& methodNumberMap,
                             int codeLine ) {
        std::string codeLineStr = "code line: " + std::to_string( codeLine );
        for( const JsonObject& block : blocks ) {
            std::string methodName;
            std::string itemId;
            EXPECT_TRUE( json_get( block, { "blockOptions", "method", "name" }, methodName ) ) << codeLineStr;
            EXPECT_TRUE( json_get( block, { "fields", 0, "value" }, itemId ) ) << codeLineStr;
            methodItemsMap[ methodName ].emplace_back( itemId );
            methodNumberMap[ methodName ]++;
        }
    }

    static void getOptionItemsInfo( const JsonArray& blocks, MethodOptionItemMap& methodOptionItemMap, int codeLine ) {
        std::string codeLineStr = "code line: " + std::to_string( codeLine );
        for( const JsonObject& block : blocks ) {
            std::string methodName;
            std::string itemId;
            JsonObject optionItem;
            EXPECT_TRUE( json_get( block, { "blockOptions", "method", "name" }, methodName ) ) << codeLineStr;
            EXPECT_TRUE( json_get( block, { "fields", 0, "value" }, itemId ) ) << codeLineStr;
            EXPECT_TRUE( json_get( block, { "fields", 2, "optionItemValue" }, optionItem ) ) << codeLineStr;
            auto it = optionItem.begin();
            methodOptionItemMap[ methodName ].emplace_back( OptionItem( itemId, it.key(), json_get_str( *it ) ) );
        }
    }

    static SceneBlocks createSceneBlocks( SceneBlocks::ItemsToGet& items, std::set< std::string >& itemIds ) {
        return SceneBlocks( [&items, &itemIds]( const SceneBlocks::ItemToSave& itemToSave ) {
            auto& item = itemToSave.item;
            if( !itemIds.insert( item.id ).second ) {
                return;
            }
            SceneBlocks::SortedOptionItemsByBlockId sortedOptionItemsByBlockId = { { itemToSave.blockId,
                                                                                     itemToSave.optionItem } };
            items.push_back( { { item.id,
                                 item.name,
                                 item.deviceId,
                                 item.enumList,
                                 { item.device.category, item.device.subcategory },
                                 item.type,
                                 { item.parent.category, item.parent.subcategory } },
                               std::move( sortedOptionItemsByBlockId ) } );
        } );
    }

    bool handleValidateBlock( const JsonObject& block ) {
        return SceneBlocks::validateWhenBlock( block );
    }

    static const std::string isItemState;
    static const std::string isItemState1;
    static const std::string isItemState2;
    static const std::string isItemState3;
    static const std::string isItemState0000002a;
    static const std::string isItemState000000E0;
    static const std::string compareNumbers;
    static const std::string isUserLockOperation;
};

const std::string SceneBlocksTest::isItemState = "isItemState";
const std::string SceneBlocksTest::isItemState1 = "isItemState1";
const std::string SceneBlocksTest::isItemState2 = "isItemState2";
const std::string SceneBlocksTest::isItemState3 = "isItemState3";
const std::string SceneBlocksTest::isItemState0000002a = "isItemState0000002a";
const std::string SceneBlocksTest::isItemState000000E0 = "isItemState000000E0";
const std::string SceneBlocksTest::compareNumbers = "compareNumbers";
const std::string SceneBlocksTest::isUserLockOperation = "isUserLockOperation";

TEST_F( SceneBlocksTest, setSceneBlocksSource ) {
    SceneBlocks sceneBlocks( []( const SceneBlocks::ItemToSave& ) {} );
    ASSERT_TRUE( sceneBlocks.setSceneBlockSource( "id1" ) );
    ASSERT_TRUE( sceneBlocks.setSceneBlockSource( "id2" ) );
    EXPECT_EQ( sceneBlocks.getBlockSourceIds(), SceneBlocks::BlockSourcesContainer( { "id1", "id2" } ) );
    sceneBlocks.removeSceneBlocksSource( "id1" );
    EXPECT_EQ( sceneBlocks.getBlockSourceIds(), SceneBlocks::BlockSourcesContainer( { "id2" } ) );
    sceneBlocks.removeSceneBlocksSource( "id2" );
    EXPECT_EQ( sceneBlocks.getBlockSourceIds(), SceneBlocks::BlockSourcesContainer() );
}

TEST_F( SceneBlocksTest, singleBlocksSource ) {
    auto ssPtr = std::make_unique< std::stringstream >();
    *ssPtr << sceneBlocksTemplates.dump();

    SceneBlocks::ItemsToGet items;
    std::set< std::string > itemIds;
    SceneBlocks sceneBlocks = createSceneBlocks( items, itemIds );

    ASSERT_TRUE( sceneBlocks.setBlockSourceAndFilterItems( "id1", *ssPtr, itemsList ) );

    std::vector< SceneBlocks::StreamUPtr > streams;
    streams.emplace_back( std::move( ssPtr ) );
    JsonObject blocks = sceneBlocks.getBlockList( items, streams );

    SceneBlocksTest::MethodItemsMap methodItemsMap;
    SceneBlocksTest::MethodNumberMap methodNumberMap;
    getTestInfo( blocks, methodItemsMap, methodNumberMap, __LINE__ );

    const SceneBlocksTest::MethodNumberMap expectedMethodNumbers = {
        { SceneBlocksTest::isItemState, 5 },
        { SceneBlocksTest::isItemState1, 2 },
        { SceneBlocksTest::isItemState3, 1 },
        { SceneBlocksTest::compareNumbers, 5 },
    };
    const SceneBlocksTest::MethodItemsMap expectedMethodItems = {
        { SceneBlocksTest::isItemState,
          { "5dd2a8efc1b5be6d20008c57",
            "5dd2a8efc1b5be6d20008c58",
            "5dd2a8efc1b5be6d20008c59",
            "5dd2a8efc1b5be6d20008c5a",
            "5dd2a8efc1b5be6d20008c5b" } },
        { SceneBlocksTest::isItemState1, { "5dd2a8efc1b5be6d20008c56", "5dd2a8efc1b5be6d20008c60" } },
        { SceneBlocksTest::isItemState3, { "5dd2a8efc1b5be6d20008c56" } },
        { SceneBlocksTest::compareNumbers,
          { "5dd2a8efc1b5be6d20008c57",
            "5dd2a8efc1b5be6d20008c58",
            "5dd2a8efc1b5be6d20008c59",
            "5dd2a8efc1b5be6d20008c5a",
            "5dd2a8efc1b5be6d20008c5b" } },
    };
    EXPECT_EQ( methodNumberMap, expectedMethodNumbers );
    EXPECT_EQ( methodItemsMap, expectedMethodItems );

    streams.clear();
    blocks = sceneBlocks.getBlockList( items, streams );
    EXPECT_TRUE( json_get_str( blocks, "when" ).empty() );
}

TEST_F( SceneBlocksTest, coupleBlocksSource ) {
    SceneBlocks::ItemsToGet items;
    std::set< std::string > itemIds;
    SceneBlocks sceneBlocks = createSceneBlocks( items, itemIds );

    std::vector< SceneBlocks::StreamUPtr > streams;
    auto ssPtr = std::make_unique< std::stringstream >();
    *ssPtr << sceneBlocksTemplates.dump();
    streams.emplace_back( std::move( ssPtr ) );

    ssPtr = std::make_unique< std::stringstream >();
    *ssPtr << sceneBlocksTemplates.dump();
    streams.emplace_back( std::move( ssPtr ) );

    ASSERT_TRUE( sceneBlocks.setBlockSourceAndFilterItems( "id1", *streams[ 0 ], itemsList ) );
    ASSERT_TRUE( sceneBlocks.setBlockSourceAndFilterItems( "id2", *streams[ 1 ], itemsList ) );

    JsonObject blocks = sceneBlocks.getBlockList( items, streams );

    SceneBlocksTest::MethodItemsMap methodItemsMap;
    SceneBlocksTest::MethodNumberMap methodNumberMap;
    getTestInfo( blocks, methodItemsMap, methodNumberMap, __LINE__ );

    const SceneBlocksTest::MethodNumberMap expectedMethodNumbers = {
        { SceneBlocksTest::isItemState, 10 },
        { SceneBlocksTest::isItemState1, 4 },
        { SceneBlocksTest::isItemState3, 2 },
        { SceneBlocksTest::compareNumbers, 10 },
    };
    const SceneBlocksTest::MethodItemsMap expectedMethodItems = {
        { SceneBlocksTest::isItemState,
          { "5dd2a8efc1b5be6d20008c57",
            "5dd2a8efc1b5be6d20008c57",
            "5dd2a8efc1b5be6d20008c58",
            "5dd2a8efc1b5be6d20008c58",
            "5dd2a8efc1b5be6d20008c59",
            "5dd2a8efc1b5be6d20008c59",
            "5dd2a8efc1b5be6d20008c5a",
            "5dd2a8efc1b5be6d20008c5a",
            "5dd2a8efc1b5be6d20008c5b",
            "5dd2a8efc1b5be6d20008c5b" } },
        { SceneBlocksTest::isItemState1,
          {
              "5dd2a8efc1b5be6d20008c56",
              "5dd2a8efc1b5be6d20008c56",
              "5dd2a8efc1b5be6d20008c60",
              "5dd2a8efc1b5be6d20008c60",
          } },
        { SceneBlocksTest::isItemState3, { "5dd2a8efc1b5be6d20008c56", "5dd2a8efc1b5be6d20008c56" } },
        { SceneBlocksTest::compareNumbers,
          { "5dd2a8efc1b5be6d20008c57",
            "5dd2a8efc1b5be6d20008c57",
            "5dd2a8efc1b5be6d20008c58",
            "5dd2a8efc1b5be6d20008c58",
            "5dd2a8efc1b5be6d20008c59",
            "5dd2a8efc1b5be6d20008c59",
            "5dd2a8efc1b5be6d20008c5a",
            "5dd2a8efc1b5be6d20008c5a",
            "5dd2a8efc1b5be6d20008c5b",
            "5dd2a8efc1b5be6d20008c5b" } },
    };
    EXPECT_EQ( methodNumberMap, expectedMethodNumbers );
    EXPECT_EQ( methodItemsMap, expectedMethodItems );

    streams.clear();
    blocks = sceneBlocks.getBlockList( items, streams );
    EXPECT_TRUE( json_get_str( blocks, "when" ).empty() );
}

TEST_F( SceneBlocksTest, validateBlocks ) {
    const JsonObject block = R"(
        {
            "item_filter": {
                 "names":[ "testItem1", "testItem2" ]
            },
            "label": {
                  "lang_tag": "ui7_token",
                  "text": "English string"
            },
            "blockType":"when",
            "blockOptions":{
                "method":{
                    "name":"isItemState",
                    "args":{
                        "item":"item",
                        "value":"value"
                    }
                }
            },
            "fields":[
                {
                    "name":"item",
                    "type":"item",
                    "value":"value"
                },
                {
                    "name":"value",
                    "type":"token",
                    "value": "manual_unlock_operation"
                }
            ]
        }
    )"_json;
    EXPECT_TRUE( handleValidateBlock( block ) );

    JsonObject invalidBlock = block;
    invalidBlock.erase( "item_filter" );
    EXPECT_FALSE( handleValidateBlock( invalidBlock ) );

    invalidBlock = block;
    invalidBlock[ "blockOptions" ][ "method" ].erase( "name" );
    EXPECT_FALSE( handleValidateBlock( invalidBlock ) );

    invalidBlock = block;
    invalidBlock[ "blockOptions" ][ "method" ][ "args" ].erase( "item" );
    EXPECT_FALSE( handleValidateBlock( invalidBlock ) );

    invalidBlock = block;
    invalidBlock.erase( "fields" );
    EXPECT_FALSE( handleValidateBlock( invalidBlock ) );
}

TEST_F( SceneBlocksTest, filterOutNewItem ) {
    auto ssPtr = std::make_unique< std::stringstream >();
    *ssPtr << sceneBlocksTemplates.dump();

    SceneBlocks::ItemToSave savedItem;
    SceneBlocks sceneBlocks( [&savedItem]( const SceneBlocks::ItemToSave& item ) { savedItem = item; } );
    JsonObject newItem = R"(
    {
        "_id": "5df8c51f7f000004b1eb358c",
        "deviceId": "5df8c51f7f000004b1eb3582",
        "name": "electric_meter_amper"
    }
    )"_json;
    sceneBlocks.filterOutNewItem( newItem, SceneBlocks::CoredItemsList(), "sourceID", *ssPtr );
    EXPECT_EQ( savedItem.item.id, "" );
    EXPECT_EQ( savedItem.item.name, "" );
    EXPECT_EQ( savedItem.item.deviceId, "" );
    EXPECT_EQ( savedItem.sourceId, "" );

    newItem = R"(
    {
        "_id": "5dd2a8efc1b5be6d20008c56",
        "deviceId": "5dd2a8eebfb5be6d20008c55",
        "name": "switch"
    }
    )"_json;
    sceneBlocks.filterOutNewItem( newItem, SceneBlocks::CoredItemsList(), "sourceID", *ssPtr );
    EXPECT_EQ( savedItem.item.id, "5dd2a8efc1b5be6d20008c56" );
    EXPECT_EQ( savedItem.item.name, "switch" );
    EXPECT_EQ( savedItem.item.deviceId, "5dd2a8eebfb5be6d20008c55" );
    EXPECT_TRUE( savedItem.item.enumList.empty() );
    EXPECT_EQ( savedItem.sourceId, "sourceID" );

    newItem = R"(
    {
        "_id": "5dd2a8efc1b5be6d20008c57",
        "deviceId": "5dd2a8eebfb5be6d20008c56",
        "name": "lock_operation",
        "enum": [
            "manual_lock_operation",
            "manual_unlock_operation",
            "rf_lock_operation",
            "rf_unlock_operation",
            "keypad_lock_operation",
            "keypad_unlock_operation",
            "manual_not_fully_locked_operation",
            "rf_not_fully_locked_operation",
            "auto_lock_not_fully_locked_operation",
            "unlock_by_rf_with_invalid_user_code",
            "lock_by_rf_with_invalid_user_code"
        ]
    }
    )"_json;
    sceneBlocks.filterOutNewItem( newItem, SceneBlocks::CoredItemsList(), "sourceID", *ssPtr );
    EXPECT_EQ( savedItem.item.id, "5dd2a8efc1b5be6d20008c57" );
    EXPECT_EQ( savedItem.item.name, "lock_operation" );
    EXPECT_EQ( savedItem.item.deviceId, "5dd2a8eebfb5be6d20008c56" );
    EXPECT_FALSE( savedItem.item.enumList.empty() );
    EXPECT_EQ( savedItem.sourceId, "sourceID" );
    SceneBlocks::EnumList enumList = { "manual_lock_operation",
                                       "manual_unlock_operation",
                                       "rf_lock_operation",
                                       "rf_unlock_operation",
                                       "keypad_lock_operation",
                                       "keypad_unlock_operation",
                                       "manual_not_fully_locked_operation",
                                       "rf_not_fully_locked_operation",
                                       "auto_lock_not_fully_locked_operation",
                                       "unlock_by_rf_with_invalid_user_code",
                                       "lock_by_rf_with_invalid_user_code" };
    EXPECT_EQ( savedItem.item.enumList, enumList );
}

TEST_F( SceneBlocksTest, alignOptions ) {
    std::vector< SceneBlocks::StreamUPtr > streams;

    auto ssPtr = std::make_unique< std::stringstream >();
    *ssPtr << sceneBlocksTemplates.dump();
    streams.emplace_back( std::move( ssPtr ) );

    SceneBlocks::ItemsToGet items;
    std::set< std::string > itemIds;
    SceneBlocks sceneBlocks = createSceneBlocks( items, itemIds );

    ASSERT_TRUE( sceneBlocks.setBlockSourceAndFilterItems( "id1", *streams[ 0 ], tokenItemsList ) );

    JsonObject blocks = sceneBlocks.getBlockList( items, streams );

    auto getOptionValues = []( const JsonObject& block ) -> JsonArray {
        JsonArray options;
        EXPECT_TRUE( json_get( block, { "fields", 1, "options" }, options ) );
        JsonArray values;
        for( const auto& option : options ) {
            values.push_back( json_get_str( option, "value" ) );
        }
        return values;
    };

    for( std::size_t i = 0; i < sceneBlocksTemplates.size(); ++i ) {
        JsonArray enums;
        EXPECT_TRUE( json_get( tokenItemsList[ i ], "enum", enums ) );
        EXPECT_EQ( enums.dump(), getOptionValues( blocks[ i ] ).dump() ) << "iteration: " << i;
    }
}

TEST_F( SceneBlocksTest, optionItem ) {
    std::vector< SceneBlocks::StreamUPtr > streams;

    auto ssPtr = std::make_unique< std::stringstream >();
    *ssPtr << sceneBlocksTemplates.dump();
    streams.emplace_back( std::move( ssPtr ) );

    SceneBlocks::ItemsToGet items;
    std::set< std::string > itemIds;
    SceneBlocks sceneBlocks( [&items, &itemIds]( const SceneBlocks::ItemToSave& itemToSave ) {
        auto& item = itemToSave.item;
        if( !itemIds.insert( item.id ).second ) {
            return;
        }
        SceneBlocks::SortedOptionItemsByBlockId sortedOptionItemsByBlockId = { { itemToSave.blockId,
                                                                                 itemToSave.optionItem } };
        items.push_back(
            { { item.id, item.name, item.deviceId, item.enumList }, std::move( sortedOptionItemsByBlockId ) } );
    } );

    ASSERT_TRUE( sceneBlocks.setBlockSourceAndFilterItems( "id1", *streams[ 0 ], optionItemsList ) );
    JsonObject blocks = sceneBlocks.getBlockList( items, streams );

    SceneBlocksTest::MethodItemsMap methodItemsMap;
    SceneBlocksTest::MethodNumberMap methodNumberMap;
    getTestInfo( blocks, methodItemsMap, methodNumberMap, __LINE__ );

    const SceneBlocksTest::MethodNumberMap expectedMethodNumbers = {
        { SceneBlocksTest::isUserLockOperation, 4 },
    };
    const SceneBlocksTest::MethodItemsMap expectedMethodItems = { { SceneBlocksTest::isUserLockOperation,
                                                                    { "5e4a5fef7f0000707ff6e9b5",
                                                                      "5e4a5fef7f0000707ff6e9b6",
                                                                      "5e4a5fef7f0000707ff6e9b7",
                                                                      "5e4a5fef7f0000707ff6e9b8" } } };
    EXPECT_EQ( methodNumberMap, expectedMethodNumbers );
    EXPECT_EQ( methodItemsMap, expectedMethodItems );

    SceneBlocksTest::MethodOptionItemMap methodOptionItemMap;
    getOptionItemsInfo( blocks, methodOptionItemMap, __LINE__ );
    const SceneBlocksTest::MethodOptionItemMap expectedOptionItems = {
        { SceneBlocksTest::isUserLockOperation,
          { OptionItem( "5e4a5fef7f0000707ff6e9b5", "itemId", "5e4a5fef7f0000707ff6e9bd" ),
            OptionItem( "5e4a5fef7f0000707ff6e9b6", "itemId", "5e4a5fef7f0000707ff6e9be" ),
            OptionItem( "5e4a5fef7f0000707ff6e9b7", "itemId", "5e4a5fef7f0000707ff6e9bf" ),
            OptionItem( "5e4a5fef7f0000707ff6e9b8", "itemId", "5e4a5fef7f0000707ff6e9c0" ) } }
    };
    EXPECT_EQ( methodOptionItemMap, expectedOptionItems );
}

TEST_F( SceneBlocksTest, checkForOptionItemUpdate ) {
    std::vector< SceneBlocks::StreamUPtr > streams;

    auto ssPtr = std::make_unique< std::stringstream >();
    *ssPtr << sceneBlocksTemplates.dump();
    streams.emplace_back( std::move( ssPtr ) );

    SceneBlocks::ItemsToGet items;
    std::set< std::string > itemIds;
    SceneBlocks sceneBlocks( [&items, &itemIds]( const SceneBlocks::ItemToSave& itemToSave ) {
        auto& item = itemToSave.item;
        if( !itemIds.insert( item.id ).second ) {
            std::for_each( items.begin(), items.end(), [&itemToSave]( SceneBlocks::ItemToGet& itemToGet ) {
                if( itemToGet.item.id != itemToSave.item.id ) {
                    return;
                }
                auto it = itemToGet.sortedOptionItemsByBlockId.find( itemToSave.blockId );
                if( it == itemToGet.sortedOptionItemsByBlockId.end() ) {
                    return;
                }
                SceneBlocks::OptionItem& optionItem = it->second;
                optionItem.value = itemToSave.optionItem.value;
            } );
            return;
        }
        SceneBlocks::SortedOptionItemsByBlockId sortedOptionItemsByBlockId = { { itemToSave.blockId,
                                                                                 itemToSave.optionItem } };
        items.push_back(
            { { item.id, item.name, item.deviceId, item.enumList, { item.device.category, item.device.subcategory } },
              std::move( sortedOptionItemsByBlockId ) } );
    } );

    ASSERT_TRUE( sceneBlocks.setBlockSourceAndFilterItems( "id1", *streams[ 0 ], {} ) );

    SceneBlocksTest::MethodOptionItemMap methodOptionItemMap;
    SceneBlocksTest::MethodOptionItemMap expectedOptionItems = {
        { SceneBlocksTest::isUserLockOperation, { OptionItem( "5e4a5fef7f0000707ff6e9b5", "itemId", "" ) } }
    };

    sceneBlocks.filterOutNewItem( optionItemsList[ 0 ], {}, "id1", *streams[ 0 ] );
    JsonObject blocks = sceneBlocks.getBlockList( items, streams );
    getOptionItemsInfo( blocks, methodOptionItemMap, __LINE__ );
    EXPECT_EQ( methodOptionItemMap, expectedOptionItems );

    sceneBlocks.checkForOptionItemUpdate( optionItemsList[ 1 ] );
    blocks = sceneBlocks.getBlockList( items, streams );
    expectedOptionItems = { { SceneBlocksTest::isUserLockOperation,
                              { OptionItem( "5e4a5fef7f0000707ff6e9b5", "itemId", "5e4a5fef7f0000707ff6e9bd" ) } } };
    methodOptionItemMap.clear();
    getOptionItemsInfo( blocks, methodOptionItemMap, __LINE__ );
    EXPECT_EQ( methodOptionItemMap, expectedOptionItems );

    sceneBlocks.filterOutNewItem( optionItemsList[ 2 ], {}, "id1", *streams[ 0 ] );
    sceneBlocks.checkForOptionItemUpdate( optionItemsList[ 3 ] );
    blocks = sceneBlocks.getBlockList( items, streams );
    expectedOptionItems = { { SceneBlocksTest::isUserLockOperation,
                              { OptionItem( "5e4a5fef7f0000707ff6e9b5", "itemId", "5e4a5fef7f0000707ff6e9bd" ),
                                OptionItem( "5e4a5fef7f0000707ff6e9b6", "itemId", "5e4a5fef7f0000707ff6e9be" ) } } };
    methodOptionItemMap.clear();
    getOptionItemsInfo( blocks, methodOptionItemMap, __LINE__ );
    EXPECT_EQ( methodOptionItemMap, expectedOptionItems );

    sceneBlocks.filterOutNewItem( optionItemsList[ 6 ], {}, "id1", *streams[ 0 ] );
    sceneBlocks.checkForOptionItemUpdate( optionItemsList[ 7 ] );
    blocks = sceneBlocks.getBlockList( items, streams );
    expectedOptionItems = { { SceneBlocksTest::isUserLockOperation,
                              { OptionItem( "5e4a5fef7f0000707ff6e9b5", "itemId", "5e4a5fef7f0000707ff6e9bd" ),
                                OptionItem( "5e4a5fef7f0000707ff6e9b6", "itemId", "5e4a5fef7f0000707ff6e9be" ),
                                OptionItem( "5e4a5fef7f0000707ff6e9b8", "itemId", "5e4a5fef7f0000707ff6e9c0" ) } } };
    methodOptionItemMap.clear();
    getOptionItemsInfo( blocks, methodOptionItemMap, __LINE__ );
    EXPECT_EQ( methodOptionItemMap, expectedOptionItems );
}

TEST_F( SceneBlocksTest, filterByDeviceCategoryAndSubCategory ) {
    auto ssPtr = std::make_unique< std::stringstream >();
    *ssPtr << sceneBlocksTemplates.dump();

    SceneBlocks::ItemsToGet items;
    std::set< std::string > itemIds;
    SceneBlocks sceneBlocks = createSceneBlocks( items, itemIds );

    ASSERT_TRUE( sceneBlocks.setBlockSourceAndFilterItems( "id1", *ssPtr, switchItemsList ) );

    std::vector< SceneBlocks::StreamUPtr > streams;
    streams.emplace_back( std::move( ssPtr ) );
    JsonObject blocks = sceneBlocks.getBlockList( items, streams );

    SceneBlocksTest::MethodItemsMap methodItemsMap;
    SceneBlocksTest::MethodNumberMap methodNumberMap;
    getTestInfo( blocks, methodItemsMap, methodNumberMap, __LINE__ );

    const SceneBlocksTest::MethodNumberMap expectedMethodNumbers = {
        { SceneBlocksTest::isItemState1, 3 },
        { SceneBlocksTest::isItemState2, 1 },
        { SceneBlocksTest::isItemState3, 3 },
    };
    const SceneBlocksTest::MethodItemsMap expectedMethodItems = {
        { SceneBlocksTest::isItemState1,
          { "5dd2a8efc1b5be6d20008ca2", "5dd2a8efc1b5be6d20008ca3", "5dd2a8efc1b5be6d20008ca4" } },
        { SceneBlocksTest::isItemState2, { "5dd2a8efc1b5be6d20008ca1" } },
        { SceneBlocksTest::isItemState3,
          { "5dd2a8efc1b5be6d20008ca2", "5dd2a8efc1b5be6d20008ca3", "5dd2a8efc1b5be6d20008ca4" } },
    };

    EXPECT_EQ( methodNumberMap, expectedMethodNumbers );
    EXPECT_EQ( methodItemsMap, expectedMethodItems );

    streams.clear();
    blocks = sceneBlocks.getBlockList( items, streams );
    EXPECT_TRUE( json_get_str( blocks, "when" ).empty() );
}

TEST_F( SceneBlocksTest, filterByParentCategoryAndSubCategory ) {
    auto ssPtr = std::make_unique< std::stringstream >();
    *ssPtr << sceneBlocksTemplates.dump();

    SceneBlocks::ItemsToGet items;
    std::set< std::string > itemIds;
    SceneBlocks sceneBlocks = createSceneBlocks( items, itemIds );

    ASSERT_TRUE( sceneBlocks.setBlockSourceAndFilterItems( "id1", *ssPtr, motionItemsList ) );

    std::vector< SceneBlocks::StreamUPtr > streams;
    streams.emplace_back( std::move( ssPtr ) );
    JsonObject blocks = sceneBlocks.getBlockList( items, streams );

    SceneBlocksTest::MethodItemsMap methodItemsMap;
    SceneBlocksTest::MethodNumberMap methodNumberMap;
    getTestInfo( blocks, methodItemsMap, methodNumberMap, __LINE__ );

    const SceneBlocksTest::MethodNumberMap expectedMethodNumbers = {
        { SceneBlocksTest::isItemState000000E0, 1 },
    };
    const SceneBlocksTest::MethodItemsMap expectedMethodItems = {
        { SceneBlocksTest::isItemState000000E0, { "611cd54e00000051c09becaf" } },
    };

    EXPECT_EQ( methodNumberMap, expectedMethodNumbers );
    EXPECT_EQ( methodItemsMap, expectedMethodItems );
}

}  // namespace UT
}  // namespace SceneEngine
