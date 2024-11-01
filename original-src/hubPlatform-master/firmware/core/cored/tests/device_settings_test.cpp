#include "cored/model/values/value.h"
#include "cored/model/cored_device_setting.h"
#include "cored/model/device_setting/device_setting_from_json.h"
#include "cored/model/resources/cored_resources.h"
#include "cored/db_modifier/device_setting_db_modifier.h"
#include "cored/tests/service/test_core_service.h"
#include "cored/remote_interface/cored_remote_interface.h"
#include "test_utils/test_utils.h"

#include "common_utils/cpp14.h"

#include <gtest/gtest.h>

namespace HubPlatform {
namespace UT {

class DeviceSettingsTest : public ::testing::Test {
    static constexpr const char* ms_deviceId = "123";
    static constexpr const char* ms_settingId = "456";

public:
    void SetUp() override {
        m_service = std::make_unique< CoreD::Tests::TestCoreServiceWithoutHouseModes >();
        m_settingDbModifier = std::make_unique< CoreD::DeviceSettingDbModifier >(
            *m_service, [this]( MessageSubClasses subClass, const Json& broadcast ) {
                m_broadcastMessage.msgSubClass = subClass;
                m_broadcastMessage.data = broadcast;
            } );

        cleanDB();

        m_service->setUp();
    }

    void TearDown() override {
        cleanDB();
        if( m_service )
            m_service->tearDown();
    }

    void checkBroadcastMessage( int subClass, const Json& data, const std::string& message ) const {
        ASSERT_EQ( m_broadcastMessage.msgSubClass, subClass ) << message;
        using namespace CoreD::Model::Resources;
        if( data.count( ScalableFields::Scale ) ) {
            checkBroadcastTemperatureMessage( subClass, data, message );
        }
        else {
            ASSERT_EQ( m_broadcastMessage.data, data )
                << message << ", difference: " << jsonDiff( data, m_broadcastMessage.data );
        }
    }

    void checkBroadcastTemperatureMessage( int subClass, const Json& data, const std::string& message ) const {
        ASSERT_EQ( m_broadcastMessage.msgSubClass, subClass );
        using namespace CoreD::Model::Resources;
        auto dataScale = json_get_str( data, ScalableFields::Scale );
        auto messageScale = json_get_str( m_broadcastMessage.data, ScalableFields::Scale );

        if( dataScale == messageScale ) {
            ASSERT_EQ( m_broadcastMessage.data, data ) << message;
            return;
        }

        double dataValue = json_get_float( data, "value", -101 );
        ASSERT_NE( dataValue, -101 ) << "Data should contain 'value' field";
        double messageValue = json_get_float( m_broadcastMessage.data, "value", -104 );
        ASSERT_NE( messageValue, -104 ) << "Broadcast should contain 'value' field";

        CoreD::Model::ValueType valueType( CoreD::Model::ValueType::Enum::Temperature );
        auto dataValueConverted = valueType.getConverter()( dataValue, dataScale.c_str(), messageScale.c_str() );

        ASSERT_FLOAT_EQ( messageValue, dataValueConverted )
            << "Temperatures should match in the same scale: " << messageScale << ", message: " << message;
    }

    Json checkSettingAdded( Json&& jsonIn, const std::string& settingId = "" ) {
        jsonIn[ "deviceId" ] = ms_deviceId;
        if( settingId.empty() )
            addSetting( makeSetting( jsonIn ) );
        else
            addSetting( makeSetting( jsonIn ), settingId );

        auto jsonOut = getSetting( settingId.empty() ? ms_settingId : settingId );
        EXPECT_FALSE( jsonOut.empty() );
        jsonIn[ "_id" ] = jsonOut[ "_id" ];

        auto it = jsonOut.find( "status" );
        if( it != jsonOut.end() )
            jsonIn[ "status" ] = *it;

        EXPECT_EQ( jsonOut, jsonIn );

        return std::move( jsonIn );
    }

    void checkSettingValueUpdated( const Json& valueFields, const std::string& message = "" ) {
        using namespace CoreD::Model::Resources;

        Json json;
        json[ DeviceSettingFields::Id ] = ms_settingId;
        json[ DeviceSettingFields::DeviceId ] = ms_deviceId;
        static std::vector< std::string > fieldNames = {
            DeviceSettingFields::Value,    DeviceSettingFields::ValueDefault, DeviceSettingFields::ValueMin,
            DeviceSettingFields::ValueMax, DeviceSettingFields::ValueStep,    DeviceSettingFields::ValueType
        };

        for( auto name : fieldNames ) {
            const auto fieldIt = valueFields.find( name );
            if( fieldIt == valueFields.end() )
                continue;

            const auto& field = *fieldIt;
            if( field.is_object() ) {
                auto scaleIt = field.find( ScalableFields::Scale );
                json[ name ] = field.at( DeviceSettingFields::Value );
                if( scaleIt != field.end() ) {
                    json[ ScalableFields::Scale ] = *scaleIt;
                }
            }
            else {
                json[ name ] = field;
            }
        }
        checkBroadcastMessage( HUB_DEVICE_SETTING_UPDATED, json, message );
    }

    void checkSettingDictionaryValueAdded( const std::string& key, const Json& value, const std::string& message ) {
        checkSettingDictionaryUpdated( "added", key, value, message );
    }

    void checkSettingDictionaryValueUpdated( const std::string& key, const Json& value, const std::string& message ) {
        checkSettingDictionaryUpdated( "updated", key, value, message );
    }

    void checkSettingDictionaryValueRemoved( const std::string& key, const std::string& message ) {
        checkSettingDictionaryUpdated( "removed", key, {}, message );
    }

    static Json makeCommonSettingJson( Json&& json ) {
        auto jsonBase = getCommonFields();
        for( auto it = jsonBase.begin(); it != jsonBase.end(); ++it ) {
            if( json.find( it.key() ) == json.end() ) {
                json[ it.key() ] = it.value();
            }
        }

        return std::move( json );
    }

    Services::Service& getService() {
        if( !m_service )
            throw std::runtime_error( "m_service is null, could not be dereferenced" );
        return *m_service;
    }

protected:
    void cleanDB() {
        getDBClient().clean();
    }

    IDbClient& getDBClient() {
        return m_service->getDBClient();
    }

    void addSetting( const CoreD::Model::DeviceSetting& setting, const std::string& settingId ) {
        using namespace CoreD::Model::Resources;

        auto settingJson = setting.toJson();
        settingJson[ DeviceSettingFields::Id ] = settingId;
        m_settingDbModifier->insert( settingJson );
    }

    void addSetting( const CoreD::Model::DeviceSetting& setting ) {
        addSetting( setting, ms_settingId );
    }

    void updateSettingValue( const Json& value ) {
        using namespace CoreD::Model::Resources;

        auto json = JSON( DeviceSettingFields::Value << value );
        m_settingDbModifier->update( ms_settingId, json );
    }

    /** @valueFields is a dictionary of [value, valueDefault, minValue, maxValue] fields */
    void updateSettingFields( const Json& valueFields ) {
        using namespace CoreD::Model::Resources;
        static std::set< std::string > fieldsAllowed = { DeviceSettingFields::Value,
                                                         DeviceSettingFields::ValueDefault,
                                                         DeviceSettingFields::ValueMin,
                                                         DeviceSettingFields::ValueMax,
                                                         DeviceSettingFields::ValueType };
        Json request;
        if( valueFields.is_object() ) {
            for( auto fieldIt = valueFields.begin(); fieldIt != valueFields.end(); fieldIt++ ) {
                if( fieldsAllowed.find( fieldIt.key() ) != fieldsAllowed.end() )
                    request[ fieldIt.key() ] = fieldIt.value();
            }
        }

        m_settingDbModifier->update( ms_settingId, request );
    }

    Json updateSettingDictionary( const std::string& operation, const std::string& key, const Json& newValue ) {
        using namespace CoreD::Model::Resources;

        auto setting = getSetting();
        auto& settingValues = setting[ DeviceSettingFields::Value ];

        auto oldValue = settingValues[ key ];
        settingValues[ key ] = newValue;
        const auto& updatedValue = operation == "removed" ? oldValue : newValue;

        auto data = JSON( DeviceSettingFields::Value << settingValues );
        m_settingDbModifier->updateDictionary( ms_settingId, data, { key, updatedValue, operation } );

        return updatedValue;
    }

    void checkSettingDictionaryUpdated( const std::string& operation,
                                        const std::string& key,
                                        const Json& value,
                                        const std::string& message ) {
        using namespace CoreD::Model::Resources;

        Json updatedValue;
        try {
            updatedValue = updateSettingDictionary( operation, key, value );
        }
        catch( const std::exception& e ) {
            FAIL() << e.what() << ": " << message;
        }

        Json json;
        json[ DeviceSettingFields::Id ] = ms_settingId;
        json[ DeviceSettingFields::DeviceId ] = ms_deviceId;
        json[ DictionaryEditRequestFields::OperationFieldName ] = operation;
        json[ DictionaryEditRequestFields::ElementFieldName ][ key ][ DictionaryEditRequestFields::ValueFieldName ] =
            updatedValue;

        checkBroadcastMessage( HUB_DEVICE_SETTING_DICTIONARY_UPDATED, json, message );
    }

    Json getSetting( std::string settingId = "" ) {
        using namespace CoreD::Model::Resources;

        const auto query = JSON( DeviceSettingFields::Id << ( settingId.empty() ? ms_settingId : settingId ) );
        return m_service->getDBClient().findOne( DBNames::DevicesSettingsDB, query, nullptr );
    }

    static CoreD::Model::DeviceSetting makeSetting( const Json& json ) {
        CoreD::Model::DeviceSettingFromJson converter;

        return converter.convert( json );
    }

    static Json getCommonFields() {
        return R"({
            "label": {
                "lang_tag": "label_tag",
                "text": "label text"
            },
            "description": {
                "lang_tag": "description_tag",
                "text": "description text"
            }
        })"_json;
    }

private:
    Services::Message m_broadcastMessage;
    std::unique_ptr< CoreD::DeviceSettingDbModifier > m_settingDbModifier;
    std::unique_ptr< CoreD::Tests::TestCoreServiceWithoutHouseModes > m_service;
};

TEST_F( DeviceSettingsTest, addZwaveConfigurationDictionarySettingTest ) {
    auto settingJson = checkSettingAdded( R"({
        "label": {
            "lang_tag": "label_tag",
            "text": "label text"
        },
        "description": {
            "lang_tag": "description_tag",
            "text": "description text"
        },
        "valueType": "dictionary.zwave_configuration",
        "value": {}
    })"_json );

    settingJson[ "valueType" ] = "dictionary";
    settingJson[ "elementType" ] = "zwave_configuration";

    EXPECT_EQ( settingJson[ "status" ], "synced" );

    checkBroadcastMessage( HUB_DEVICE_SETTING_ADDED, settingJson, MAKE_MESSAGE( "" ) );
}

TEST_F( DeviceSettingsTest, addZwaveConfigurationDictionarySettingWithElementsMaxNumberTest ) {
    auto settingJson = checkSettingAdded( R"({
        "label": {
            "lang_tag": "label_tag",
            "text": "label text"
        },
        "description": {
            "lang_tag": "description_tag",
            "text": "description text"
        },
        "valueType": "dictionary.zwave_configuration",
        "value": {},
        "elementsMaxNumber": 10
    })"_json );

    settingJson[ "valueType" ] = "dictionary";
    settingJson[ "elementType" ] = "zwave_configuration";

    EXPECT_EQ( settingJson[ "status" ], "synced" );

    checkBroadcastMessage( HUB_DEVICE_SETTING_ADDED, settingJson, MAKE_MESSAGE( "" ) );
}

TEST_F( DeviceSettingsTest, addZwaveAssociationGroupsSettingTest ) {
    auto settingJson = checkSettingAdded( R"({
        "label": {
            "lang_tag": "label_tag",
            "text": "label text"
        },
        "description": {
            "lang_tag": "description_tag",
            "text": "description text"
        },
        "valueType": "dictionary.association_group",
        "value": {
            "0_1": {
                "channel": 0,
                "commands": [
                    "DEVICE_RESET_LOCALLY_NOTIFICATION",
                    "CENTRAL_SCENE_NOTIFICATION",
                    "SCENE_ACTIVATION_SET",
                    "WAKE_UP_NOTIFICATION"
                ],
                "maxAssociationsNumber": 1,
                "name": "Lifeline",
                "profile": "General:Lifeline"
            },
            "0_2": {
                "channel": 0,
                "commands": [
                    "BASIC_SET",
                    "SWITCH_MULTILEVEL_START_LEVEL_CHANGE",
                    "SWITCH_MULTILEVEL_STOP_LEVEL_CHANGE",
                    "SWITCH_ALL_ON",
                    "SWITCH_ALL_OFF"
                ],
                "maxAssociationsNumber": 8,
                "name": "Button 1 - MONO - Controlled nodes",
                "profile": "Control:Key01"
            }
        }
    })"_json );

    settingJson[ "valueType" ] = "dictionary";
    settingJson[ "elementType" ] = "association_group";

    EXPECT_EQ( settingJson[ "status" ], "synced" );
    EXPECT_FALSE( settingJson[ "value" ].empty() );

    checkBroadcastMessage( HUB_DEVICE_SETTING_ADDED, settingJson, MAKE_MESSAGE( "" ) );
}

TEST_F( DeviceSettingsTest, addZwaveAssociationsSettingTest ) {
    auto settingJson = checkSettingAdded( R"({
        "label": {
            "lang_tag": "label_tag",
            "text": "label text"
        },
        "description": {
            "lang_tag": "description_tag",
            "text": "description text"
        },
        "targetChannelSupported": false,
        "valueType": "dictionary.array.association",
        "value": {}
    })"_json );

    settingJson[ "valueType" ] = "dictionary";
    settingJson[ "elementType" ] = "array.association";

    EXPECT_EQ( settingJson[ "status" ], "synced" );
    EXPECT_EQ( settingJson[ "targetChannelSupported" ], false );

    checkBroadcastMessage( HUB_DEVICE_SETTING_ADDED, settingJson, MAKE_MESSAGE( "" ) );
}

TEST_F( DeviceSettingsTest, addSettingWithInfoTest ) {
    auto settingJson = checkSettingAdded( makeCommonSettingJson( R"({
        "valueType": "action",
        "value": {
            "text": "Button label"
        },
        "info": {
            "zwave.parameter_number": 1,
            "zwave.parameter_size": 1
        }
    })"_json ) );

    EXPECT_EQ( settingJson[ "status" ], "synced" );

    checkBroadcastMessage( HUB_DEVICE_SETTING_ADDED, settingJson, MAKE_MESSAGE( "" ) );
}

TEST_F( DeviceSettingsTest, addActionSettingTest ) {
    auto settingJson = checkSettingAdded( makeCommonSettingJson( R"({
        "valueType": "action",
        "value": {
            "text": "Button label"
        }
    })"_json ) );

    EXPECT_EQ( settingJson[ "status" ], "synced" );

    checkBroadcastMessage( HUB_DEVICE_SETTING_ADDED, settingJson, MAKE_MESSAGE( "" ) );
}

TEST_F( DeviceSettingsTest, addBoolSettingTest ) {
    auto settingJson = checkSettingAdded( makeCommonSettingJson( R"({
        "valueType": "bool",
        "value": true
    })"_json ) );

    EXPECT_EQ( settingJson[ "status" ], "synced" );

    checkBroadcastMessage( HUB_DEVICE_SETTING_ADDED, settingJson, MAKE_MESSAGE( "" ) );
}

TEST_F( DeviceSettingsTest, addStringSettingTest ) {
    {
        auto settingJson = checkSettingAdded( makeCommonSettingJson( R"({
            "valueType": "string",
            "value": "abc"
        })"_json ) );

        EXPECT_EQ( settingJson[ "status" ], "synced" );

        checkBroadcastMessage( HUB_DEVICE_SETTING_ADDED, settingJson, MAKE_MESSAGE( "" ) );
    }

    {
        auto settingJson = checkSettingAdded( makeCommonSettingJson( R"({
            "valueType": "string",
            "value": "",
            "status": "pending"
        })"_json ) );

        EXPECT_EQ( settingJson[ "status" ], "pending" );

        checkBroadcastMessage( HUB_DEVICE_SETTING_ADDED, settingJson, MAKE_MESSAGE( "" ) );
    }
}

TEST_F( DeviceSettingsTest, addBoolSettingWithDefaultTest ) {
    auto settingJson = checkSettingAdded( makeCommonSettingJson( R"({
        "valueType": "bool",
        "value": true,
        "valueDefault": false
    })"_json ) );

    checkBroadcastMessage( HUB_DEVICE_SETTING_ADDED, settingJson, MAKE_MESSAGE( "" ) );
}

TEST_F( DeviceSettingsTest, addIntSettingTest ) {
    {
        auto settingJson = checkSettingAdded( makeCommonSettingJson( R"({
            "valueType": "int",
            "value": 71
        })"_json ) );

        EXPECT_EQ( settingJson[ "status" ], "synced" );

        checkBroadcastMessage( HUB_DEVICE_SETTING_ADDED, settingJson, MAKE_MESSAGE( "" ) );
    }

    {
        auto settingJson = checkSettingAdded( makeCommonSettingJson( R"({
            "valueType": "int",
            "value": 71,
            "status": "pending",
            "hasSetter": false
        })"_json ) );

        EXPECT_EQ( settingJson[ "status" ], "pending" );
        EXPECT_EQ( settingJson[ "hasSetter" ], false );

        checkBroadcastMessage( HUB_DEVICE_SETTING_ADDED, settingJson, MAKE_MESSAGE( "" ) );
    }
}

TEST_F( DeviceSettingsTest, addReadOnlySettingTest ) {
    auto settingJson = checkSettingAdded( makeCommonSettingJson( R"({
        "valueType": "int",
        "hasSetter": false,
        "value": 71
    })"_json ) );

    EXPECT_EQ( settingJson[ "status" ], "synced" );

    checkBroadcastMessage( HUB_DEVICE_SETTING_ADDED, settingJson, MAKE_MESSAGE( "" ) );
}

TEST_F( DeviceSettingsTest, addIntSettingWithDefaultTest ) {
    auto settingJson = checkSettingAdded( makeCommonSettingJson( R"({
        "valueType": "int",
        "value": 71,
        "valueDefault": 50
    })"_json ) );

    EXPECT_EQ( settingJson[ "status" ], "synced" );

    checkBroadcastMessage( HUB_DEVICE_SETTING_ADDED, settingJson, MAKE_MESSAGE( "" ) );
}

TEST_F( DeviceSettingsTest, addIntSettingWithMinTest ) {
    auto settingJson = checkSettingAdded( makeCommonSettingJson( R"({
        "valueType": "int",
        "value": 71,
        "valueMin": 0
    })"_json ) );

    checkBroadcastMessage( HUB_DEVICE_SETTING_ADDED, settingJson, MAKE_MESSAGE( "" ) );
}

TEST_F( DeviceSettingsTest, addIntSettingWithMaxTest ) {
    auto settingJson = checkSettingAdded( makeCommonSettingJson( R"({
        "valueType": "int",
        "value": 71,
        "valueMax": 100
    })"_json ) );

    checkBroadcastMessage( HUB_DEVICE_SETTING_ADDED, settingJson, MAKE_MESSAGE( "" ) );
}

TEST_F( DeviceSettingsTest, addIntSettingWithStepTest ) {
    auto settingJson = checkSettingAdded( makeCommonSettingJson( R"({
        "valueType": "int",
        "value": 71,
        "valueStep": 15
    })"_json ) );

    checkBroadcastMessage( HUB_DEVICE_SETTING_ADDED, settingJson, MAKE_MESSAGE( "" ) );
}

TEST_F( DeviceSettingsTest, addRgbSettingTest ) {
    auto settingJson = checkSettingAdded( makeCommonSettingJson( R"({
        "valueType": "rgb",
        "value": {
            "red": 255,
            "green": 255,
            "blue": 255
        }
    })"_json ) );

    EXPECT_EQ( settingJson[ "status" ], "synced" );

    checkBroadcastMessage( HUB_DEVICE_SETTING_ADDED, settingJson, MAKE_MESSAGE( "" ) );
}

TEST_F( DeviceSettingsTest, addRgbSettingWithDefaultTest ) {
    auto settingJson = checkSettingAdded( makeCommonSettingJson( R"({
        "valueType": "rgb",
        "value": {
            "red": 255,
            "green": 255,
            "blue": 255
        },
        "valueDefault": {
            "red": 0,
            "green": 0,
            "blue": 0
        }
    })"_json ) );

    EXPECT_EQ( settingJson[ "status" ], "synced" );

    checkBroadcastMessage( HUB_DEVICE_SETTING_ADDED, settingJson, MAKE_MESSAGE( "" ) );
}

TEST_F( DeviceSettingsTest, addScalableSettingTest ) {
    auto settingJson = checkSettingAdded( makeCommonSettingJson( R"({
        "valueType": "temperature",
        "value": {
            "value": 71,
            "scale": "celsius"
        }
    })"_json ) );

    // Fixme: EZZB-843
    using namespace CoreD::Model;
    using namespace CoreD::Model::Resources;
    settingJson[ ScalableFields::Scale ] = ValueScales::Celsius;
    settingJson[ ScalableFields::Value ] = 71;

    checkBroadcastTemperatureMessage( HUB_DEVICE_SETTING_ADDED, settingJson, MAKE_MESSAGE( "" ) );
}

TEST_F( DeviceSettingsTest, addScalableSettingWithDefaultTest ) {
    auto settingJson = checkSettingAdded( makeCommonSettingJson( R"({
        "valueType": "temperature",
        "value": {
            "value": 71,
            "scale": "celsius"
        },
        "valueDefault": {
            "value": 50,
            "scale": "celsius"
        }
    })"_json ) );

    // Fixme: EZZB-843
    using namespace CoreD::Model;
    using namespace CoreD::Model::Resources;
    settingJson[ ScalableFields::Scale ] = ValueScales::Celsius;
    settingJson[ DeviceSettingFields::Value ] = 71;
    settingJson[ DeviceSettingFields::ValueDefault ] = 50;

    checkBroadcastTemperatureMessage( HUB_DEVICE_SETTING_ADDED, settingJson, MAKE_MESSAGE( "" ) );
}

TEST_F( DeviceSettingsTest, addScalableSettingWithMinTest ) {
    auto settingJson = checkSettingAdded( makeCommonSettingJson( R"({
        "valueType": "temperature",
        "value": {
            "value": 71,
            "scale": "celsius"
        },
        "valueMin": {
            "value": 0,
            "scale": "celsius"
        }
    })"_json ) );

    // Fixme: EZZB-843
    using namespace CoreD::Model;
    using namespace CoreD::Model::Resources;

    settingJson[ ScalableFields::Scale ] = ValueScales::Celsius;
    settingJson[ DeviceSettingFields::Value ] = 71;
    settingJson[ DeviceSettingFields::ValueMin ] = 0;

    checkBroadcastTemperatureMessage( HUB_DEVICE_SETTING_ADDED, settingJson, MAKE_MESSAGE( "" ) );
}

TEST_F( DeviceSettingsTest, addScalableSettingWithMaxTest ) {
    auto settingJson = checkSettingAdded( makeCommonSettingJson( R"({
        "valueType": "temperature",
        "value": {
            "value": 71,
            "scale": "celsius"
        },
        "valueMax": {
            "value": 100,
            "scale": "celsius"
        }
    })"_json ) );

    // Fixme: EZZB-843
    using namespace CoreD::Model;
    using namespace CoreD::Model::Resources;

    settingJson[ ScalableFields::Scale ] = ValueScales::Celsius;
    settingJson[ DeviceSettingFields::Value ] = 71;
    settingJson[ DeviceSettingFields::ValueMax ] = 100;

    checkBroadcastTemperatureMessage( HUB_DEVICE_SETTING_ADDED, settingJson, MAKE_MESSAGE( "" ) );
}

TEST_F( DeviceSettingsTest, addTokenSettingTest ) {
    auto settingJson = checkSettingAdded( makeCommonSettingJson( R"({
        "valueType": "token",
        "enum": {
            "value_1": {
                "lang_tag": "value_1_tag",
                "text": "value 1 text"
            },
            "value_2": {
                "text": "value 2 text"
            }
        },
        "value": "value_2"
    })"_json ) );

    checkBroadcastMessage( HUB_DEVICE_SETTING_ADDED, settingJson, MAKE_MESSAGE( "" ) );
}

TEST_F( DeviceSettingsTest, addTokenSettingWithDefaultTest ) {
    auto settingJson = checkSettingAdded( makeCommonSettingJson( R"({
        "valueType": "token",
        "enum": {
            "value_1": {
                "lang_tag": "value_1_tag",
                "text": "value 1 text"
            },
            "value_2": {
                "text": "value 2 text"
            }
        },
        "value": "value_2",
        "valueDefault": "value_1"
    })"_json ) );

    checkBroadcastMessage( HUB_DEVICE_SETTING_ADDED, settingJson, MAKE_MESSAGE( "" ) );
}

TEST_F( DeviceSettingsTest, addTokenArraySettingTest ) {
    auto settingJson = checkSettingAdded( makeCommonSettingJson( R"({
        "valueType": "array.token",
        "enum": {
            "value_1": {
                "lang_tag": "value_1_tag",
                "text": "value 1 text"
            },
            "value_2": {
                "text": "value 2 text"
            }
        },
        "value": [ "value_1" ]
    })"_json ) );

    checkBroadcastMessage( HUB_DEVICE_SETTING_ADDED, settingJson, MAKE_MESSAGE( "" ) );
}

TEST_F( DeviceSettingsTest, addTokenArraySettingWithDefaultTest ) {
    auto settingJson = checkSettingAdded( makeCommonSettingJson( R"({
        "valueType": "array.token",
        "enum": {
            "value_1": {
                "lang_tag": "value_1_tag",
                "text": "value 1 text"
            },
            "value_2": {
                "text": "value 2 text"
            }
        },
        "value": [ "value_1" ],
        "valueDefault": [ "value_1", "value_2" ]
    })"_json ) );

    checkBroadcastMessage( HUB_DEVICE_SETTING_ADDED, settingJson, MAKE_MESSAGE( "" ) );
}

TEST_F( DeviceSettingsTest, updateSettingValueTest ) {
    const Json boolValue = R"({
        "valueType": "bool",
        "value": true
    })"_json;

    using namespace CoreD::Model::Resources;
    Json tmp = boolValue;
    tmp[ DeviceSettingFields::Value ] = !json_get_bool( boolValue, DeviceSettingFields::Value );
    checkSettingAdded( makeCommonSettingJson( std::move( tmp ) ) );

    updateSettingFields( boolValue );

    checkSettingValueUpdated( boolValue, MAKE_MESSAGE( "updateSettingValueTest" ) );
}

TEST_F( DeviceSettingsTest, updateSettingTemperatureValueTest ) {
    const Json temperatureValue = R"({
        "valueType": "temperature",
        "value": { "value": 71, "scale": "fahrenheit" },
        "valueDefault": { "value": 70, "scale": "fahrenheit" },
        "valueMin": { "value": 60, "scale": "fahrenheit" },
        "valueMax": { "value": 80, "scale": "fahrenheit" }
    })"_json;
    Json tmp = temperatureValue;
    checkSettingAdded( makeCommonSettingJson( std::move( tmp ) ) );

    using namespace CoreD::Model;
    using namespace CoreD::Model::Resources;
    Json modifiedValue = JSON(
        "valueType"
        << "temperature"
        << "value" << JSON( ScalableFields::Value << 78 << ScalableFields::Scale << ValueScales::Fahrenheit )
        << "valueMax" << JSON( ScalableFields::Value << 81 << ScalableFields::Scale << ValueScales::Fahrenheit ) );

    updateSettingFields( modifiedValue );

    checkSettingValueUpdated( modifiedValue, MAKE_MESSAGE( "updateSettingTemperatureValueTest" ) );
}

TEST_F( DeviceSettingsTest, updateSettingTemperatureValueMixedTest ) {
    const Json temperatureValue = R"({
        "valueType": "temperature",
        "value": { "value": 71, "scale": "fahrenheit" },
        "valueDefault": { "value": 70, "scale": "fahrenheit" },
        "valueMin": { "value": 60, "scale": "fahrenheit" },
        "valueMax": { "value": 80, "scale": "fahrenheit" }
    })"_json;
    Json tmp = temperatureValue;
    checkSettingAdded( makeCommonSettingJson( std::move( tmp ) ) );

    using namespace CoreD::Model;
    using namespace CoreD::Model::Resources;
    Json modifiedValue = JSON(
        "valueType"
        << "temperature"
        // 81 F = 25.5555 C
        << "value"
        << JSON( ScalableFields::Value << 25.5555 << ScalableFields::Scale << ValueScales::Celsius )
        // 81 F == 27.2222
        << "valueMax" << JSON( ScalableFields::Value << 27.2222 << ScalableFields::Scale << ValueScales::Celsius ) );

    updateSettingFields( modifiedValue );

    checkSettingValueUpdated( modifiedValue, MAKE_MESSAGE( "Values in different scales should match" ) );
}

TEST_F( DeviceSettingsTest, updateSettingDictionaryValueTest ) {
    Json setting = R"({
        "label": {
            "lang_tag": "label_tag",
            "text": "label text"
        },
        "description": {
            "lang_tag": "description_tag",
            "text": "description text"
        },
        "valueType": "dictionary.zwave_configuration",
        "value": {}
    })"_json;

    checkSettingAdded( std::move( setting ) );

    Json dictionaryValue = R"({
        "name": "Parameter 1",
        "description": "Parameter 1",
        "format": "dec",
        "size": 1,
        "value": 1,
        "number": 1
    })"_json;
    checkSettingDictionaryValueAdded( "1", dictionaryValue, MAKE_MESSAGE( "Add value=1" ) );

    dictionaryValue[ "value" ] = 2;
    checkSettingDictionaryValueUpdated( "1", dictionaryValue, MAKE_MESSAGE( "Add value=2" ) );

    checkSettingDictionaryValueRemoved( "1", MAKE_MESSAGE( "removed" ) );
}

TEST_F( DeviceSettingsTest, updateSettingAssciationValueTest ) {
    Json setting = R"({
        "label": {
            "lang_tag": "label_tag",
            "text": "label text"
        },
        "description": {
            "lang_tag": "description_tag",
            "text": "description text"
        },
        "valueType": "dictionary.array.association",
        "value": {}
    })"_json;

    checkSettingAdded( std::move( setting ) );

    Json dictionaryValue = R"([
        {
            "targetNode": 10,
            "targetChannel": 1
        },
        {
            "targetNode": 11,
            "targetChannel": 1
        }
    ])"_json;
    checkSettingDictionaryValueAdded( "0_1", dictionaryValue, MAKE_MESSAGE( "Add value=1" ) );

    dictionaryValue[ 1 ][ "targetNode" ] = 12;
    checkSettingDictionaryValueUpdated( "0_1", dictionaryValue, MAKE_MESSAGE( "Add value=2" ) );

    checkSettingDictionaryValueRemoved( "0_1", MAKE_MESSAGE( "removed" ) );
}

TEST_F( DeviceSettingsTest, updateReadOnlySettingDictionaryValueTest ) {
    Json setting = R"({
        "label": {
            "lang_tag": "label_tag",
            "text": "label text"
        },
        "description": {
            "lang_tag": "description_tag",
            "text": "description text"
        },
        "valueType": "dictionary.zwave_configuration",
        "value": {}
    })"_json;

    checkSettingAdded( std::move( setting ) );

    Json dictionaryValue = R"({
        "name": "Parameter 1",
        "description": "Parameter 1",
        "format": "dec",
        "size": 1,
        "hasSetter": false,
        "number": 1
    })"_json;
    checkSettingDictionaryValueAdded( "1", dictionaryValue, MAKE_MESSAGE( "Add value=1" ) );

    checkSettingDictionaryValueRemoved( "1", MAKE_MESSAGE( "removed" ) );
}

TEST_F( DeviceSettingsTest, getDeviceSettingsBy ) {
    const Json setting = R"({
        "label": {
            "lang_tag": "label_tag",
            "text": "label text"
        },
        "description": {
            "lang_tag": "description_tag",
            "text": "description text"
        },
        "valueType": "dictionary.zwave_configuration",
        "value": {}
    })"_json;

    std::vector< std::string > ids;
    for( int i = 0; i < 5; i++ ) {
        Json copy = setting;
        Json settingAdded = checkSettingAdded( std::move( copy ), "1230" + std::to_string( i ) );
        ids.push_back( json_get_str( settingAdded, "_id" ) );
    }
    CoreDRemoteInterface::CoreDRemoteInterface remote( getService() );

    for( auto size = ids.size(); size > 0; --size ) {
        const auto slice = std::vector< std::string >( begin( ids ), begin( ids ) + size );
        const Json condition = JSON( "_id" << JSON( "$in" << JsonArray( slice ) ) );

        auto fetchedSettings = remote.getDeviceSettingsBy( condition );
        EXPECT_EQ( fetchedSettings.size(), slice.size() );
    }
}

}  // namespace UT
}  // namespace HubPlatform
