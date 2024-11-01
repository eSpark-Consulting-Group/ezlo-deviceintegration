#include "scenes_manager/scenes_processor/response_builders/scenes_block_data_list/scenes_data_list_response_builder.h"
#include "service_interfaces/tests/fakes/fake_service.h"
#include "database_client/dbqueryparser.h"
#include "json_utils/json_utils.h"
#include "database_client/dbid.h"
#include "test_utils/component_tests/common_nma_test_fixture.h"
#include "cored/model/values/value_type.h"
#include "cored/remote_interface/cored_interface_factory.h"

namespace HubPlatform {
namespace ScenesList {
namespace UT {

using namespace ::testing;
using namespace HubPlatform::ScenesManager;

class ScenesDataListBuilderTestSuite : public Test {
public:
    void SetUp() override;
    JsonArray getList( const JsonObject& response, const std::string& key, const char* file, int codeLine ) {
        const auto& errorInfo = errorAt( file, codeLine );
        JsonObject obj;
        EXPECT_EQ( json_get_obj( response, key, obj ), true ) << file << ":" << codeLine;
        JsonArray arr;
        EXPECT_EQ( json_get_arr( obj, "list", arr ), true ) << file << ":" << codeLine;
        return arr;
    }

    JsonObject createResponse( bool valueTypes,
                               bool scalableValueTypes,
                               bool valueScales,
                               bool sceneValueTypes,
                               bool valueTypeFamilies,
                               bool comparisonOperators,
                               bool comparisonMethods ) {
        ScenesDataListResponseBuilder responseBuilder( *m_core );
        responseBuilder.setValueTypes( valueTypes );
        responseBuilder.setScalableValueTypes( scalableValueTypes );
        responseBuilder.setValueScales( valueScales );
        responseBuilder.setSceneValueTypes( sceneValueTypes );
        responseBuilder.setValueTypeFamilies( valueTypeFamilies );
        responseBuilder.setComparisonOperators( comparisonOperators );
        responseBuilder.setComparisonMethods( comparisonMethods );

        return responseBuilder.getDataWithFilter();
    }

    JsonObject createResponseAll() {
        ScenesDataListResponseBuilder responseBuilder( *m_core );
        responseBuilder.setAll( true );
        return responseBuilder.getDataWithFilter();
    }

    void checkComparisonMethods( const JsonObject& response, bool shouldExist, const char* file, int codeLine ) {
        const auto& errorInfo = errorAt( file, codeLine );
        JsonObject field;
        EXPECT_EQ( json_get_obj( response, "comparisonMethods", field ), shouldExist ) << errorInfo;
        if( !shouldExist ) {
            return;
        }

        auto checkComparisonMethod =
            [this]( Json record, std::string methodName, std::string comparatorField, const char* file, int line ) {
                auto errorInfo = errorAt( file, line );
                JsonObject comparisonMethod;
                EXPECT_EQ( json_get_obj( record, methodName, comparisonMethod ), true ) << errorInfo;

                JsonArray dataSource;
                EXPECT_EQ( json_get_arr( comparisonMethod, "dataSource", dataSource ), true ) << errorInfo;

                if( !comparatorField.empty() ) {
                    JsonObject comparator;
                    EXPECT_EQ( json_get_obj( comparisonMethod, comparatorField, comparator ), true ) << errorInfo;
                }
            };

        JsonObject info;
        EXPECT_EQ( json_get_obj( field, "info", info ), true ) << errorInfo;
        checkComparisonMethod( field, "isItemState", "", __FILE__, __LINE__ );
        checkComparisonMethod( field, "compareNumbers", "comparator", __FILE__, __LINE__ );
        checkComparisonMethod( field, "compareNumberRange", "comparator", __FILE__, __LINE__ );
        checkComparisonMethod( field, "compareStrings", "comparator", __FILE__, __LINE__ );
        checkComparisonMethod( field, "stringOperation", "comparator", __FILE__, __LINE__ );
        checkComparisonMethod( field, "compareValues", "comparator", __FILE__, __LINE__ );
        // checkComparisonMethod( field, "inArray", "comparator", __FILE__, __LINE__ );
    }

    void checkComparisonOperators( const JsonObject& response, bool shouldExist, const char* file, int codeLine ) {
        const auto& errorInfo = errorAt( file, codeLine );

        JsonObject families;

        EXPECT_EQ( json_get_obj( response, "comparisonOperators", families ), shouldExist ) << errorInfo;

        if( !shouldExist ) {
            return;
        }

        JsonArray familiesArr;
        json_get_arr( families, "families", familiesArr );
        EXPECT_TRUE( familiesArr.size() > 0 ) << errorInfo;
        for( auto family : familiesArr ) {
            JsonArray methods;
            json_get_arr( family, "methods", methods );
            EXPECT_TRUE( methods.size() > 0 ) << errorInfo;
        }
    }

    void checkAllValueTypes( const JsonObject& response, const char* file, int codeLine ) {
        JsonObject scalableValueTypesField;
        json_get_obj( response, "scalableValueTypes", scalableValueTypesField );
        auto scalableValueTypesListArr = getList( response, "scalableValueTypes", file, codeLine );

        JsonObject valueTypesField;
        json_get_obj( response, "valueTypes", valueTypesField );

        auto valueTypesListArr = getList( response, "scalableValueTypes", file, codeLine );

        EXPECT_GE( valueTypesListArr.size() + scalableValueTypesListArr.size(),
                   (int) HubPlatform::CoreD::Model::ValueType::Enum::SimpleTypesCount )
            << file << ":" << std::to_string( codeLine );
    }

    void checkScalableValueTypes( const JsonObject& response, bool shouldExist, const char* file, int codeLine ) {
        JsonObject field;
        EXPECT_EQ( json_get_obj( response, "scalableValueTypes", field ), shouldExist )
            << file << ":" << std::to_string( codeLine );

        if( !shouldExist ) {
            return;
        }
    }

    void checkScenesValueTypes( const JsonObject& response, bool shouldExist, const char* file, int codeLine ) {
        JsonObject field;
        const auto& errorInfo = errorAt( file, codeLine );
        EXPECT_EQ( json_get_obj( response, "scenesValueTypes", field ), shouldExist ) << errorInfo;

        if( !shouldExist ) {
            return;
        }

        auto scalableValueTypesListArr = getList( response, "scenesValueTypes", file, codeLine );
        EXPECT_EQ( scalableValueTypesListArr.size(), 3 ) << errorInfo;
    }

    void checkValueTypes( const JsonObject& response, bool shouldExist, const char* file, int codeLine ) {
        const auto& errorInfo = errorAt( file, codeLine );
        JsonObject field;
        EXPECT_EQ( json_get_obj( response, "valueTypes", field ), shouldExist ) << errorInfo;

        if( !shouldExist ) {
            return;
        }

        auto scalableValueTypesListArr = getList( response, "valueTypes", file, codeLine );
        EXPECT_GT( scalableValueTypesListArr.size(), 0 ) << errorInfo;
    }

    void checkValueFamilies( JsonObject& response, bool shouldExist, const char* file, int codeLine ) {
        JsonObject families;

        const auto& errorInfo = errorAt( file, codeLine );

        EXPECT_EQ( json_get_obj( response, "valueTypeFamilies", families ), shouldExist ) << errorInfo;

        if( !shouldExist ) {
            return;
        }

        JsonObject numericFamily;
        EXPECT_EQ( json_get( families, "numeric", numericFamily ), true ) << errorInfo;
        JsonObject stringsFamily;
        EXPECT_EQ( json_get( families, "strings", stringsFamily ), true ) << errorInfo;
    }

    void checkValueScales( JsonObject& response, bool shouldExist, const char* file, int codeLine ) {
        JsonObject scales;
        const auto& errorInfo = errorAt( file, codeLine );
        EXPECT_EQ( json_get_obj( response, "valueScales", scales ), shouldExist ) << errorInfo;

        if( !shouldExist ) {
            return;
        }

        EXPECT_GT( scales.size(), 0 ) << errorInfo;
    }

    void checkResponse( bool valueTypes,
                        bool scalableValueTypes,
                        bool valueScales,
                        bool sceneValueTypes,
                        bool valueTypeFamilies,
                        bool comparisonOperators,
                        bool comparisonMethods,
                        const char* file,
                        int codeLine ) {
        auto response = createResponse( valueTypes,
                                        scalableValueTypes,
                                        valueScales,
                                        sceneValueTypes,
                                        valueTypeFamilies,
                                        comparisonOperators,
                                        comparisonMethods );
        checkComparisonMethods( response, comparisonMethods, file, codeLine );
        checkComparisonOperators( response, comparisonOperators, file, codeLine );
        checkScalableValueTypes( response, scalableValueTypes, file, codeLine );
        checkScenesValueTypes( response, sceneValueTypes, file, codeLine );
        checkValueTypes( response, valueTypes, file, codeLine );
        checkValueFamilies( response, valueTypeFamilies, file, codeLine );
        checkValueScales( response, valueScales, file, codeLine );
    }

    ScenesDataListResponseBuilder getScenesDataListResponseBuilder() {
        ScenesDataListResponseBuilder responseBuilder( *m_core );
        return responseBuilder;
    }

    const std::string errorAt( const char* file, int codeLine ) {
        std::string res = file;
        res += ":" + std::to_string( codeLine );

        return res;
    }

private:
    Services::UT::FakeService m_fakeService;
    CoreDRemoteInterface::CoreDInterfaceUPtr m_core;
};

void ScenesDataListBuilderTestSuite::SetUp() {
    m_core = CoreDRemoteInterface::makeInstance( m_fakeService );
}

TEST_F( ScenesDataListBuilderTestSuite, checkAll ) {
    auto response = createResponseAll();
    checkComparisonMethods( response, true, __FILE__, __LINE__ );
    checkComparisonOperators( response, true, __FILE__, __LINE__ );
    checkScalableValueTypes( response, true, __FILE__, __LINE__ );
    checkScenesValueTypes( response, true, __FILE__, __LINE__ );
    checkValueTypes( response, true, __FILE__, __LINE__ );
    checkValueFamilies( response, true, __FILE__, __LINE__ );
    checkValueScales( response, true, __FILE__, __LINE__ );

    checkAllValueTypes( response, __FILE__, __LINE__ );
}

TEST_F( ScenesDataListBuilderTestSuite, getAllTypes ) {
    bool valueTypes = true;
    bool scalableValueTypes = true;
    bool valueScales = true;
    bool sceneValueTypes = true;
    bool valueTypeFamilies = true;
    bool comparisonOperators = true;
    bool comparisonMethods = true;

    checkResponse( valueTypes,
                   scalableValueTypes,
                   valueScales,
                   sceneValueTypes,
                   valueTypeFamilies,
                   comparisonOperators,
                   comparisonMethods,
                   __FILE__,
                   __LINE__ );
}

TEST_F( ScenesDataListBuilderTestSuite, getValueTypes ) {
    bool valueTypes = true;
    bool scalableValueTypes = false;
    bool valueScales = false;
    bool sceneValueTypes = false;
    bool valueTypeFamilies = false;
    bool comparisonOperators = false;
    bool comparisonMethods = false;

    checkResponse( valueTypes,
                   scalableValueTypes,
                   valueScales,
                   sceneValueTypes,
                   valueTypeFamilies,
                   comparisonOperators,
                   comparisonMethods,
                   __FILE__,
                   __LINE__ );
}

TEST_F( ScenesDataListBuilderTestSuite, getScalableValueTypes ) {
    bool valueTypes = false;
    bool scalableValueTypes = true;
    bool valueScales = false;
    bool sceneValueTypes = false;
    bool valueTypeFamilies = false;
    bool comparisonOperators = false;
    bool comparisonMethods = false;

    checkResponse( valueTypes,
                   scalableValueTypes,
                   valueScales,
                   sceneValueTypes,
                   valueTypeFamilies,
                   comparisonOperators,
                   comparisonMethods,
                   __FILE__,
                   __LINE__ );
}

TEST_F( ScenesDataListBuilderTestSuite, getValueScales ) {
    bool valueTypes = false;
    bool scalableValueTypes = false;
    bool valueScales = true;
    bool sceneValueTypes = false;
    bool valueTypeFamilies = false;
    bool comparisonOperators = false;
    bool comparisonMethods = false;

    checkResponse( valueTypes,
                   scalableValueTypes,
                   valueScales,
                   sceneValueTypes,
                   valueTypeFamilies,
                   comparisonOperators,
                   comparisonMethods,
                   __FILE__,
                   __LINE__ );
}

TEST_F( ScenesDataListBuilderTestSuite, getSceneValueTypes ) {
    bool valueTypes = false;
    bool scalableValueTypes = false;
    bool valueScales = false;
    bool sceneValueTypes = true;
    bool valueTypeFamilies = false;
    bool comparisonOperators = false;
    bool comparisonMethods = false;

    checkResponse( valueTypes,
                   scalableValueTypes,
                   valueScales,
                   sceneValueTypes,
                   valueTypeFamilies,
                   comparisonOperators,
                   comparisonMethods,
                   __FILE__,
                   __LINE__ );
}

TEST_F( ScenesDataListBuilderTestSuite, getSceneValueTypeFamilies ) {
    bool valueTypes = false;
    bool scalableValueTypes = false;
    bool valueScales = false;
    bool sceneValueTypes = false;
    bool valueTypeFamilies = true;
    bool comparisonOperators = false;
    bool comparisonMethods = false;

    checkResponse( valueTypes,
                   scalableValueTypes,
                   valueScales,
                   sceneValueTypes,
                   valueTypeFamilies,
                   comparisonOperators,
                   comparisonMethods,
                   __FILE__,
                   __LINE__ );
}

TEST_F( ScenesDataListBuilderTestSuite, getComparisonOperators ) {
    bool valueTypes = false;
    bool scalableValueTypes = false;
    bool valueScales = false;
    bool sceneValueTypes = false;
    bool valueTypeFamilies = false;
    bool comparisonOperators = true;
    bool comparisonMethods = false;

    checkResponse( valueTypes,
                   scalableValueTypes,
                   valueScales,
                   sceneValueTypes,
                   valueTypeFamilies,
                   comparisonOperators,
                   comparisonMethods,
                   __FILE__,
                   __LINE__ );
}

TEST_F( ScenesDataListBuilderTestSuite, getComparisonMethods ) {
    bool valueTypes = false;
    bool scalableValueTypes = false;
    bool valueScales = false;
    bool sceneValueTypes = false;
    bool valueTypeFamilies = false;
    bool comparisonOperators = false;
    bool comparisonMethods = true;

    checkResponse( valueTypes,
                   scalableValueTypes,
                   valueScales,
                   sceneValueTypes,
                   valueTypeFamilies,
                   comparisonOperators,
                   comparisonMethods,
                   __FILE__,
                   __LINE__ );
}

TEST_F( ScenesDataListBuilderTestSuite, getEmptyResponse ) {
    auto response = getScenesDataListResponseBuilder().getDataWithFilter();
    EXPECT_EQ( response, JSON_EMPTY_OBJ );
}

}  // namespace UT
}  // namespace ScenesList
}  // namespace HubPlatform
