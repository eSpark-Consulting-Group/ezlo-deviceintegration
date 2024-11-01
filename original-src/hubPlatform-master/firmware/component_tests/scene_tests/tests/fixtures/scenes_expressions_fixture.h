#pragma once

#include "common_test_resources/fixtures/scene_manager_tests_fixture.h"
#include <gtest/gtest.h>

namespace HubPlatform {
namespace Tests {

class ScenesExpressionsFixture : public SceneManagerTestsFixture {
public:
    ScenesExpressionsFixture();

    void SetUp() override;
    void TearDown() override;

    static void checkNoError( const Json& answer, const char* file, int line );
    static void checkHasError( const Json& answer, const char* file, int line );
    std::string makeDeviceSiren( int codeLine );
    std::string makeDeviceThermostat( int codeLine );
    static CoreD::Model::Item makeIntItem( const std::string& deviceId, const std::string& itemName, int value );
    static CoreD::Model::Item makeTemperatureItem( const std::string& deviceId,
                                                   const std::string& itemName,
                                                   float value,
                                                   const char* scale = CoreD::Model::ValueScales::Fahrenheit );
    static CoreD::Model::Item makeBoolItem( const std::string& deviceId, const std::string& itemName, bool value );
    static CoreD::Model::Item makeStringItem( const std::string& deviceId,
                                              const std::string& itemName,
                                              const std::string& value );
    static void runExpressionsList( Json& answer, bool showCode );
    static void runExpressionsListWithFilter( Json& answer,
                                              bool showCode,
                                              bool strict,
                                              const std::vector< std::string >& filters,
                                              bool isVariable,
                                              const std::vector< std::string >& names,
                                              bool showValue );

    void checkExpressions( const Json& answer, Json& expressions, int line );
    void checkExpressionByName( const Json& expressions,
                                const std::string& expressionName,
                                const KeyJsonList& fields,
                                int line );

    Json getExpressionValueByName( const Json& expressions, const std::string& expressionName, int line );

    bool isExpressionChangedBroadcast( const JsonObject& broadcast, const std::string& expressionName );

    void deleteSceneExpression( const std::string& expressionName, int codeLine );

    void scenesExpressionSet( const std::string& expressionName,
                              const std::string& code,
                              const Json& deviceItemsParams,
                              const Json& itemsParams,
                              bool expressionExists,
                              int codeLine );

    template < class T >
    void scenesVariableExpressionSetType( const std::string& expressionName,
                                          T value,
                                          const std::string& broadcastName,
                                          const std::string& valueType,
                                          const Json& metadata,
                                          bool expectedBroadcast,
                                          int codeLine );

    void scenesExpressionSetExt( const std::string& expressionName,
                                 const std::string& code,
                                 const Json& deviceItemsParams,
                                 const Json& itemsParams,
                                 bool expressionExists,
                                 const std::string& valueType,
                                 const Json& metadata,
                                 bool variable,
                                 bool expectedBroadcast,
                                 int codeLine );

    void scenesVariableExpressionSet( const std::string& expressionName,
                                      const Json& value,
                                      bool expressionExists,
                                      const std::string& valueType,
                                      const Json& metadata,
                                      bool expectedBroadcast,
                                      int codeLine );

    template < class T >
    Json makeVariableExpressionSetCommandValueType( const std::string& expressionName,
                                                    T value,
                                                    const std::string& valueType,
                                                    const Json& metadata,
                                                    bool overwrite );

    static Json makeExpressionSetCommand( const std::string& expressionName,
                                          const std::string& code,
                                          const Json& deviceItemsParams,
                                          const Json& itemsParams,
                                          const std::string& valueType,
                                          const Json& metadata,
                                          bool variable,
                                          bool overwrite );

    static Json makeVariableExpressionSetCommand( const std::string& expressionName,
                                                  const Json& value,
                                                  const std::string& valueType,
                                                  const Json& metadata,
                                                  bool overwrite );

protected:
    CoreDRemoteInterface::CoreDInterface& m_remote;
    void deleteAllExpressions();
};

template < class T >
inline void ScenesExpressionsFixture::scenesVariableExpressionSetType( const std::string& expressionName,
                                                                       T value,
                                                                       const std::string& broadcastName,
                                                                       const std::string& valueType,
                                                                       const Json& metadata,
                                                                       bool expectedBroadcast,
                                                                       int codeLine ) {
    Json command = makeVariableExpressionSetCommandValueType< T >( expressionName, value, valueType, metadata, false );
    auto answer = nma().sendtoHubAndGetAnswer( command );
    checkNoError( answer, getCallingFile().c_str(), codeLine );

    if( expectedBroadcast ) {
        checkBroadcast( "", broadcastName, { { "name", expressionName } }, codeLine );
    }
}

namespace SceneCommands {
extern const std::string expressionsSet;
}

template < class T >
inline Json ScenesExpressionsFixture::makeVariableExpressionSetCommandValueType( const std::string& expressionName,
                                                                                 T value,
                                                                                 const std::string& valueType,
                                                                                 const Json& metadata,
                                                                                 bool overwrite ) {
    Json command = PARSE_JSON( SceneCommands::expressionsSet );
    command[ "params" ][ "code" ] = "";
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

    return command;
}

}  // namespace Tests
}  // namespace HubPlatform
