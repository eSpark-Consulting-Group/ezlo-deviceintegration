#include "scenes_expressions_fixture.h"
#include "scenes_manager/remote_interface/scenes_remote_interface.h"

namespace HubPlatform {
namespace Tests {

ScenesExpressionsFixture::ScenesExpressionsFixture()
: m_remote( makeRemote( nma() ) ) {}

void ScenesExpressionsFixture::SetUp() {
    SceneManagerTestsFixture::SetUp();
}

void ScenesExpressionsFixture::TearDown() {
    deleteAllExpressions();
    SceneManagerTestsFixture::TearDown();
}

void ScenesExpressionsFixture::checkNoError( const Json& answer, const char* file, int line ) {
    Json error;
    ASSERT_TRUE( json_get( answer, "error", error ) )
        << "No error field expected, error = " << error << ", at " << file << ":" << line;
    ASSERT_TRUE( error.is_null() ) << "Should be null: " << error << ", at " << file << ":" << line;
}

void ScenesExpressionsFixture::checkHasError( const Json& answer, const char* file, int line ) {
    Json error;
    ASSERT_TRUE( json_get( answer, "error", error ) )
        << "No error field, error = " << error << ", at " << file << ":" << line;
    ASSERT_FALSE( error.is_null() ) << "Should not be null: " << error << ", at " << file << ":" << line;
}

namespace SceneCommands {
const std::string expressionsSet = R"({
        "method": "hub.scenes.expressions.set",
        "params": {
            "name": "",
            "params": {},
            "code": {},
            "valueType": "",
            "metadata": null,
            "variable": false
        }
    })";
const auto expressionDelete =
    R"({ "method": "hub.scenes.expressions.delete", "params": { "name": "twoPlusTwoMultiplyTwo" }})";
const auto expressionList = R"({ "method": "hub.scenes.expressions.list", "params": {}})";
}  // namespace SceneCommands

void ScenesExpressionsFixture::runExpressionsList( Json& answer, bool showCode ) {
    auto command = PARSE_JSON( SceneCommands::expressionList );
    command[ "params" ][ "showCode" ] = showCode;
    answer = nma().sendtoHubAndGetAnswer( command );
}

void ScenesExpressionsFixture::runExpressionsListWithFilter( Json& answer,
                                                             bool showCode,
                                                             bool strict,
                                                             const std::vector< std::string >& filters,
                                                             bool isVariable,
                                                             const std::vector< std::string >& names,
                                                             bool showValue ) {
    auto command = PARSE_JSON( SceneCommands::expressionList );
    command[ "params" ][ "showCode" ] = showCode;
    command[ "params" ][ "strict" ] = strict;

    JsonArrayBuilder filtersArr;

    for( const auto& filter : filters ) {
        filtersArr << filter;
    }

    if( filters.size() == 1 ) {
        command[ "params" ][ "filterType" ] = filters.front();
    }
    else {
        command[ "params" ][ "filterTypes" ] = filtersArr;
    }

    command[ "params" ][ "variable" ] = isVariable;

    JsonArrayBuilder namesArr;
    if( !names.empty() ) {
        for( const auto& name : names ) {
            namesArr << name;
        }

        command[ "params" ][ "names" ] = namesArr;
    }

    command[ "params" ][ "showValue" ] = showValue;

    answer = nma().sendtoHubAndGetAnswer( command );
}

void ScenesExpressionsFixture::checkExpressions( const Json& answer, Json& expressions, int line ) {
    JsonArray result;
    auto lineStr = getCodeLineString( line );
    ASSERT_TRUE( json_get( answer, "result", result ) ) << lineStr;
    ASSERT_TRUE( json_get_arr( result, "expressions", expressions ) )
        << "should have expressions array, at:" << lineStr;
}

void ScenesExpressionsFixture::checkExpressionByName( const Json& expressions,
                                                      const std::string& expressionName,
                                                      const KeyJsonList& fields,
                                                      int line ) {
    auto errorAt = getCodeLineString( line );
    auto foundIt = std::find_if( expressions.begin(), expressions.end(), [&]( const Json& item ) {
        return json_get_str( item, "name" ) == expressionName;
    } );
    ASSERT_TRUE( foundIt != expressions.end() ) << errorAt;
    const Json& result = *foundIt;
    Json fetchedValue;

    for( const auto& field : fields ) {
        JsonObject value;
        if( field.first == Resources::CheckBroadcast::HasNot ) {
            EXPECT_EQ( result.count( field.second ), 0 )
                << "Should have no field '" << field.second << "', " << errorAt;
        }
        else if( field.first == Resources::CheckBroadcast::Has ) {
            EXPECT_GT( result.count( field.second ), 0 ) << "Should have a field '" << field.second << "', " << errorAt;
        }
        else {
            EXPECT_TRUE( json_get( result, field.first, value ) ) << errorAt << ", field: " << field.first;
            EXPECT_EQ( value.dump(), field.second.dump() ) << errorAt << ", field: " << field.first;
        }
    }
}

Json ScenesExpressionsFixture::getExpressionValueByName( const Json& expressions,
                                                         const std::string& expressionName,
                                                         int line ) {
    auto errorAt = getCodeLineString( line );
    auto foundIt = std::find_if( expressions.begin(), expressions.end(), [&]( const Json& item ) {
        return json_get_str( item, "name" ) == expressionName;
    } );

    const Json& result = *foundIt;
    Json fetchedValue;

    JsonObject value;
    const auto ValueField = "value";

    EXPECT_TRUE( json_get( result, ValueField, value ) ) << errorAt << ", field: " << ValueField;
    return value;
}

bool ScenesExpressionsFixture::isExpressionChangedBroadcast( const JsonObject& broadcast,
                                                             const std::string& expressionName ) {
    JsonObject result = JSON_EMPTY_OBJ;
    return json_get_obj( broadcast, "result", result ) &&
           json_get_str( broadcast, "msg_subclass" ) == "hub.expression.changed" &&
           json_get_str( result, "name" ) == expressionName;
}

CoreD::Model::Item ScenesExpressionsFixture::makeIntItem( const std::string& deviceId,
                                                          const std::string& itemName,
                                                          int value ) {
    using namespace CoreD::Model;
    auto valueType = ValueType::Enum::Int;
    CoreD::Model::Item item( deviceId,
                             itemName,
                             ValueType::Enum::Int,
                             true,
                             true,
                             ValueFactory::createItemValue( valueType, Json( value ) ) );
    return item;
}

CoreD::Model::Item ScenesExpressionsFixture::makeBoolItem( const std::string& deviceId,
                                                           const std::string& itemName,
                                                           bool value ) {
    using namespace CoreD::Model;
    auto valueType = ValueType::Enum::Bool;
    CoreD::Model::Item item( deviceId,
                             itemName,
                             ValueType::Enum::Bool,
                             true,
                             true,
                             ValueFactory::createItemValue( valueType, Json( value ) ) );
    return item;
}

CoreD::Model::Item ScenesExpressionsFixture::makeStringItem( const std::string& deviceId,
                                                             const std::string& itemName,
                                                             const std::string& value ) {
    using namespace CoreD::Model;
    auto valueType = ValueType::Enum::String;
    CoreD::Model::Item item( deviceId,
                             itemName,
                             ValueType::Enum::String,
                             true,
                             true,
                             ValueFactory::createItemValue( valueType, Json( value ) ) );
    return item;
}

CoreD::Model::Item ScenesExpressionsFixture::makeTemperatureItem( const std::string& deviceId,
                                                                  const std::string& itemName,
                                                                  float value,
                                                                  const char* scale ) {
    using namespace CoreD::Model;
    auto valueType = ValueType::Enum::Temperature;
    if( !scale || !*scale )
        scale = CoreD::Model::ValueScales::Fahrenheit;
    Json scalableValue = { { "value", value }, { "scale", scale } };
    CoreD::Model::Item item(
        deviceId, itemName, valueType, true, true, ValueFactory::createItemValue( valueType, scalableValue ) );
    return item;
}

std::string ScenesExpressionsFixture::makeDeviceSiren( int codeLine ) {
    return addDevice( m_remote, "Controller Siren", "siren", "siren", "siren", "", false, codeLine );
}

std::string ScenesExpressionsFixture::makeDeviceThermostat( int codeLine ) {
    return addDevice( m_remote, "Z-Wave Thermostat", "thermostat", "139_21586_21570", "hvac", "", false, codeLine );
}

void ScenesExpressionsFixture::deleteSceneExpression( const std::string& expressionName, int codeLine ) {
    if( expressionName.empty() )
        return;
    HubPlatform::ScenesManager::RemoteInterface scenesRemote( nma() );

    scenesRemote.deleteSceneExpression( expressionName );
    checkBroadcast( "", "hub.expression.deleted", { { "name", expressionName } }, codeLine );
}

void ScenesExpressionsFixture::scenesExpressionSet( const std::string& expressionName,
                                                    const std::string& code,
                                                    const Json& deviceItemsParams,
                                                    const Json& itemsParams,
                                                    bool expressionExists,
                                                    int codeLine ) {
    Json command = makeExpressionSetCommand(
        expressionName, code, deviceItemsParams, itemsParams, "", JSON_EMPTY_OBJ, false, true );

    auto answer = nma().sendtoHubAndGetAnswer( command );
    checkNoError( answer, __FILE__, codeLine );

    auto broadcastName = "hub.expression.added";
    if( expressionExists )
        broadcastName = "hub.expression.changed";

    checkBroadcast( "",
                    broadcastName,
                    { { "name", expressionName },
                      { Resources::CheckBroadcast::Has, "code" },
                      { Resources::CheckBroadcast::HasNot, "valueType" },
                      { Resources::CheckBroadcast::HasNot, "metadata" } },
                    codeLine );
}

void ScenesExpressionsFixture::scenesVariableExpressionSet( const std::string& expressionName,
                                                            const Json& value,
                                                            bool expressionExists,
                                                            const std::string& valueType,
                                                            const Json& metadata,
                                                            bool expectedBroadcast,
                                                            int codeLine ) {
    Json command = makeVariableExpressionSetCommand( expressionName, value, valueType, metadata, false );
    auto answer = nma().sendtoHubAndGetAnswer( command );
    checkNoError( answer, __FILE__, codeLine );

    if( expectedBroadcast ) {
        auto broadcastName = "hub.expression.added";
        if( expressionExists )
            broadcastName = "hub.expression.changed";

        checkBroadcast( "",
                        broadcastName,
                        { { "name", expressionName }, { "valueType", valueType }, { "variable", true } },
                        codeLine );
    }
}

void ScenesExpressionsFixture::scenesExpressionSetExt( const std::string& expressionName,
                                                       const std::string& code,
                                                       const Json& deviceItemsParams,
                                                       const Json& itemsParams,
                                                       bool expressionExists,
                                                       const std::string& valueType,
                                                       const Json& metadata,
                                                       bool variable,
                                                       bool expectedBroadcast,
                                                       int codeLine ) {
    Json command = makeExpressionSetCommand(
        expressionName, code, deviceItemsParams, itemsParams, valueType, metadata, variable, false );
    auto answer = nma().sendtoHubAndGetAnswer( command );
    checkNoError( answer, __FILE__, codeLine );

    if( expectedBroadcast ) {
        auto broadcastName = "hub.expression.added";
        if( expressionExists )
            broadcastName = "hub.expression.changed";

        KeyJsonList fieldList = { { "name", expressionName }, { "variable", variable } };
        if( !valueType.empty() )
            fieldList.push_back( { "valueType", valueType } );
        if( !metadata.empty() )
            fieldList.push_back( { "metadata", metadata } );

        checkBroadcast( "", broadcastName, fieldList, codeLine );
    }
}

Json ScenesExpressionsFixture::makeExpressionSetCommand( const std::string& expressionName,
                                                         const std::string& code,
                                                         const Json& deviceItemsParams,
                                                         const Json& itemsParams,
                                                         const std::string& valueType,
                                                         const Json& metadata,
                                                         bool variable,
                                                         bool overwrite ) {
    Json command = PARSE_JSON( SceneCommands::expressionsSet );

    command[ "params" ][ "name" ] = expressionName;
    command[ "params" ][ "code" ] = code;
    command[ "params" ][ "params" ] = JSON_EMPTY_OBJ;

    if( variable || overwrite ) {
        command[ "params" ][ "variable" ] = variable;
    }
    if( !valueType.empty() || overwrite ) {
        command[ "params" ][ "valueType" ] = valueType;
    }
    if( !metadata.empty() || overwrite ) {
        command[ "params" ][ "metadata" ] = metadata;
    }

    if( !deviceItemsParams.empty() )
        command[ "params" ][ "params" ][ "device_item_names" ] = deviceItemsParams;
    if( !itemsParams.empty() )
        command[ "params" ][ "params" ][ "items" ] = itemsParams;

    return command;
}

Json ScenesExpressionsFixture::makeVariableExpressionSetCommand( const std::string& expressionName,
                                                                 const Json& value,
                                                                 const std::string& valueType,
                                                                 const Json& metadata,
                                                                 bool overwrite ) {
    Json command = PARSE_JSON( SceneCommands::expressionsSet );

    command[ "params" ][ "name" ] = expressionName;
    command[ "params" ][ "value" ] = value;
    command[ "params" ][ "params" ] = JSON_EMPTY_OBJ;
    command[ "params" ][ "variable" ] = true;

    if( !valueType.empty() || overwrite ) {
        command[ "params" ][ "valueType" ] = valueType;
    }
    if( !metadata.empty() || overwrite ) {
        command[ "params" ][ "metadata" ] = metadata;
    }

    LOGGER_ERROR_RAW( "command: " + cc::j( command ) );

    return command;
}

void ScenesExpressionsFixture::deleteAllExpressions() {
    Json answer;
    JsonArray expressions;
    runExpressionsList( answer, false );
    json_get_arr( answer[ "result" ], "expressions", expressions );
    if( expressions.empty() )
        return;

    HubPlatform::ScenesManager::RemoteInterface scenesRemote( nma() );

    for( auto& expression : expressions ) {
        auto name = json_get_str( expression, "name" );
        try {
            scenesRemote.deleteSceneExpression( name );
            checkBroadcast( "",
                            "hub.expression.deleted",
                            { { "name", name } },
                            NMA_TEST_MAKE_MESSAGE( "Cleanup orphaned expressions" ) );
        }
        catch( ... ) {
        }
    }
}

}  // namespace Tests
}  // namespace HubPlatform
