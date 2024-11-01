#include "scenes_manager/scenes_processor/response_builders/scenes_block_data_list/scenes_data_list_response_builder.h"
#include "cored/model/values/value_factory.h"
#include "uid/handlers/hub_settings_commands_handler.h"
#include "scenes_manager/scenes_engine/model/comparison_info.h"
#include "cored/remote_interface/cored_interface.h"
#include "system_utils/environment.h"
#include "system_utils/filesystem.h"
#include "cored/model/values/value_families.h"
#include "scenes_manager/scenes_engine/model/model_resources.h"

namespace HubPlatform {
namespace ScenesManager {

using namespace SceneEngine::Model;

ScenesDataListResponseBuilder::ScenesDataListResponseBuilder( CoreDRemoteInterface::CoreDInterface& coreInterface )
: m_coreInterface( coreInterface ) {}

void ScenesDataListResponseBuilder::setAll( bool all ) {
    if( all ) {
        m_valueTypes = true;
        m_scalableValueTypes = true;
        m_valueScales = true;
        m_sceneValueTypes = true;
        m_valueTypeFamilies = true;
        m_comparisonOperators = true;
        m_comparisonMethods = true;
        m_actionMethods = true;
    }
}

void ScenesDataListResponseBuilder::setValueTypes( bool valueTypes ) {
    m_valueTypes = valueTypes;
}

void ScenesDataListResponseBuilder::setScalableValueTypes( bool scalableValueTypes ) {
    m_scalableValueTypes = scalableValueTypes;
}

void ScenesDataListResponseBuilder::setValueScales( bool valueScales ) {
    m_valueScales = valueScales;
}

void ScenesDataListResponseBuilder::setSceneValueTypes( bool sceneValueTypes ) {
    m_sceneValueTypes = sceneValueTypes;
}

void ScenesDataListResponseBuilder::setValueTypeFamilies( bool valueTypeFamilies ) {
    m_valueTypeFamilies = valueTypeFamilies;
}

void ScenesDataListResponseBuilder::setComparisonOperators( bool comparisonOperators ) {
    m_comparisonOperators = comparisonOperators;
}

void ScenesDataListResponseBuilder::setComparisonMethods( bool comparisonMethods ) {
    m_comparisonMethods = comparisonMethods;
}

void ScenesDataListResponseBuilder::setActionMethods( bool actionsData ) {
    m_actionMethods = actionsData;
}

bool ScenesDataListResponseBuilder::getDefaultScale( const std::string& typeName ) {
    auto settingsList = m_coreInterface.getSettings();
    for( const auto& setting : settingsList ) {
        auto settingName = setting.getName();
        if( settingName.rfind( "scale.", 0 ) == 0 ) {
            std::size_t pos = settingName.find( '.' );
            std::string suffix = settingName.substr( pos + 1 );
            if( suffix == typeName ) {
                return true;
            }
        }
    }

    return false;
}

void ScenesDataListResponseBuilder::getValueTypeData( JsonArrayBuilder& valueTypes,
                                                      JsonArrayBuilder& scalableValueTypes,
                                                      Json& valueScales ) {
    using namespace CoreD::Model;
    const std::string List = "list";

    std::vector< std::string > valueTypesArr;
    std::vector< std::string > scalableValueTypesArr;

    if( m_valueTypes || m_scalableValueTypes || m_valueScales ) {
        for( auto type = ValueType::Enum::Int; type < ValueType::Enum::NoValue;
             type = static_cast< ValueType::Enum >( ( static_cast< int >( type ) + 1 ) ) ) {
            ValueType valueType( type );
            auto typeName = std::string( valueType.toString() );
            bool isScalable = valueType.getScales().size() > 0;

            if( m_valueTypes && !isScalable && !typeName.empty() ) {
                valueTypesArr.push_back( typeName );
            }

            if( m_scalableValueTypes && isScalable ) {
                scalableValueTypesArr.push_back( typeName );
            }

            if( m_valueScales && isScalable ) {
                std::vector< std::string > scales;

                Json valueScaleItem;
                JsonArrayBuilder arr;
                for( auto scaleItem : valueType.getScales() ) {
                    arr << scaleItem;
                }

                const std::string Scales = "scales";
                const std::string Converter = "converter";
                valueScaleItem[ Scales ] = arr;
                valueScaleItem[ Converter ] = valueType.getConverter() != nullptr;

                if( getDefaultScale( typeName ) ) {
                    valueScaleItem[ "defaultScale" ] = true;
                }

                valueScales[ typeName ] = valueScaleItem;
            }
        }

        if( m_valueTypes ) {
            for( const auto& typeName : valueTypesArr ) {
                valueTypes << typeName;
            }
        }

        if( m_scalableValueTypes ) {
            std::sort( scalableValueTypesArr.begin(), scalableValueTypesArr.end() );
            for( const auto& typeName : scalableValueTypesArr ) {
                scalableValueTypes << typeName;
            }
        }
    }
}

JsonObject ScenesDataListResponseBuilder::getTypeFamilyTypes() {
    JsonObject families;
    for( const auto& family : CoreD::Model::valueTypeFamilies() ) {
        JsonArrayBuilder arr;

        for( const auto& item : family.second ) {
            arr << item;
        }

        families[ family.first ] = arr;
    }
    return families;
}

JsonObject ScenesDataListResponseBuilder::getDataWithFilter() {
    JsonObject res = JSON_EMPTY_OBJ;

    using namespace CoreD::Model;
    std::vector< std::string > allTypes;
    std::vector< std::string > scalableTypes;

    JsonArrayBuilder valueTypes;
    JsonArrayBuilder scalableValueTypes;
    Json valueScales;

    JsonArrayBuilder valueTypeFamilies;

    getValueTypeData( valueTypes, scalableValueTypes, valueScales );

    const std::string List = "list";
    if( m_valueTypes ) {
        res[ "valueTypes" ] = JSON( List << valueTypes );
    }

    if( m_scalableValueTypes ) {
        res[ "scalableValueTypes" ] = JSON( List << scalableValueTypes );
    }

    if( m_valueScales ) {
        res[ "valueScales" ] = valueScales;
    }

    if( m_sceneValueTypes ) {
        JsonArrayBuilder scenesValueTypesArr;
        for( const auto& type : SceneEngine::Model::ScenesValueTypes ) {
            scenesValueTypesArr << type;
        }

        res[ "scenesValueTypes" ] = JSON( List << scenesValueTypesArr );
    }

    if( m_valueTypeFamilies ) {
        res[ "valueTypeFamilies" ] = getTypeFamilyTypes();
    }

    if( m_comparisonOperators ) {
        res[ "comparisonOperators" ] = getComparisonOperators();
    }

    if( m_comparisonMethods ) {
        res[ "comparisonMethods" ] = getComparisonMethods();
    }

    if( m_actionMethods ) {
        auto configPath = Environment::getConfigsFolder() + "/scenes_action_metadata.cfg";
        if( filesystem::isFileExist( configPath ) ) {
            std::ifstream file( configPath );
            JsonObject content;
            file >> content;
            res[ "actionMethods" ] = content;
            LOGGER_DEBUG( cc::debug( "Action methods file has been read" ) + cc::j( content ) );
        }
        else {
            LOGGER_ERROR( cc::error( "Can't open action methods file" ) );
        }
    }

    return res;
}

JsonArrayBuilder& fillOperatorsArr( JsonArrayBuilder& operatorsArr, const OperatorInfo& operatorInfo ) {
    const std::string Method = "method";
    const std::string Name = "name";
    const std::string Operator = "op";
    for( const auto& operatorInfoItem : operatorInfo.op ) {
        if( !operatorInfoItem.op.empty() && !operatorInfoItem.name.empty() ) {
            operatorsArr << JSON( Operator << operatorInfoItem.op << Name << operatorInfoItem.name << Method
                                           << operatorInfo.method );
        }
    }
    return operatorsArr;
}

JsonObject getFamilies( OperatorsMapper::EventNameEnumOperatorInfoMap& operatorsInfoMap,
                        const std::pair< const std::string, std::vector< std::string > >& family,
                        JsonObject& familyParam ) {
    const std::string Family = "family";
    const std::string Methods = "methods";

    JsonArrayBuilder operatorsArr;
    for( const auto& item : operatorsInfoMap ) {
        const SceneEngine::Model::OperatorsMapper::OperatorInfoList& operatorInfoList = item.second;

        for( const auto& operatorInfo : operatorInfoList ) {
            if( operatorInfo.family == family.first ) {
                if( familyParam.empty() ) {
                    familyParam = JSON( Family << family.first );
                }

                operatorsArr = fillOperatorsArr( operatorsArr, operatorInfo );
                familyParam[ Methods ] = operatorsArr;
            }
        }
    }

    return familyParam;
}

Json ScenesDataListResponseBuilder::getComparisonOperators() const {
    Json res;
    JsonArrayBuilder families;
    auto operatorsInfoMap = OperatorsMapper::operatorInfoMap();
    for( auto const& family : CoreD::Model::valueTypeFamilies() ) {
        JsonObject familyParam;

        familyParam = getFamilies( operatorsInfoMap, family, familyParam );
        if( !familyParam.empty() ) {
            families << familyParam;
        }
    }

    res = JSON( "families" << families );
    return res;
}

JsonArrayBuilder getDataSource( std::vector< SceneEngine::Model::MethodInfo >& methodDataSourceInfos ) {
    JsonArrayBuilder dataSource;
    for( const auto& info : methodDataSourceInfos ) {
        JsonArrayBuilder typesArr;
        for( const auto& type : info.types ) {
            typesArr << type;
        }

        auto dataSourceItem = JSON( "index" << info.index << "types" << typesArr );
        if( !info.field.empty() ) {
            dataSourceItem[ "field" ] = info.field;
        }

        dataSource << dataSourceItem;
    }
    return dataSource;
}

JsonArrayBuilder getOperatorOptions( std::vector< SceneEngine::Model::Operator >& operators ) {
    JsonArrayBuilder options;
    for( const auto& operatorItem : operators ) {
        if( !operatorItem.op.empty() )
            options << operatorItem.op;
    }
    return options;
}

JsonObject ScenesDataListResponseBuilder::getComparisonMethods() {
    auto methodInfoMap = OperatorsMapper::methodInfoMap();
    auto operatorsInfoMap = OperatorsMapper::operatorInfoMap();

    std::string version = SceneEngine::Model::ComparisonMethodsVersion;
    JsonObject methodInfo;
    methodInfo[ "info" ] = JSON( "version" << version );
    for( const auto& item : methodInfoMap ) {
        auto itemInfoKey = item.first;
        std::vector< SceneEngine::Model::MethodInfo > methodDataSourceInfos = item.second;
        auto methodOperatorInfoList = operatorsInfoMap[ itemInfoKey ];
        auto methodName = OperatorsMapper::getMethodNameFromEnum( itemInfoKey );

        JsonArrayBuilder dataSource;
        JsonObject comparator;
        JsonArrayBuilder types;

        dataSource = getDataSource( methodDataSourceInfos );

        if( methodOperatorInfoList.empty() ) {
            // exception for isItemState: no families, no comparators, only name and dataSource
            auto methodInfoItem = JSON( "dataSource" << dataSource );
            methodInfo[ methodName ] = methodInfoItem;
            continue;
        }
        for( const auto& methodOperatorInfo : methodOperatorInfoList ) {
            auto type = methodOperatorInfo.type;
            auto family = methodOperatorInfo.family;
            auto operators = methodOperatorInfo.op;
            // TODO: EFWR-3337: this field should be passed outside, all methods: comparator, but for stringOperation:
            // operation
            auto field = "comparator";

            JsonArrayBuilder options = getOperatorOptions( operators );

            comparator = JSON( "family" << family << "type" << type << "options" << options << "field" << field );
            auto methodInfoItem = JSON( "dataSource" << dataSource );

            if( !family.empty() ) {
                methodInfoItem[ "comparator" ] = comparator;
            }

            methodInfo[ methodName ] = methodInfoItem;
        }
    }

    return methodInfo;
}

}  // namespace ScenesManager
}  // namespace HubPlatform
