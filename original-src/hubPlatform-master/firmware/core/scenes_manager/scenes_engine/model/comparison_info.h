#pragma once

#include "event_names.h"
#include <map>
#include <vector>

namespace SceneEngine {
namespace Model {

struct Operator {
    std::string op;
    std::string name;
};

struct OperatorInfo {
    std::vector< Operator > op;
    std::string method;
    std::string family;
    std::string type;
};

struct MethodInfo {
    int index;
    std::string field;
    std::vector< std::string > types;
};

extern const std::string ComparisonMethodsVersion;

class OperatorsMapper {
public:
    using EventNameEnum = EventName::Enum;
    using OperatorInfoList = std::vector< OperatorInfo >;
    using EventNameEnumOperatorInfoMap = std::map< EventNameEnum, OperatorInfoList >;

public:
    static const EventNameEnumOperatorInfoMap& operatorInfoMap();

    static const std::map< EventNameEnum, std::vector< MethodInfo > >& methodInfoMap();

    static const std::map< std::string, EventNameEnum >& enumFromMethodNameMap();

    static EventNameEnum getEnumFromMethodName( const std::string& methodName );

    static std::string getMethodNameFromEnum( EventNameEnum enumValue );
};

}  // namespace Model
}  // namespace SceneEngine